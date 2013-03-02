require 'fileutils'
require 'rainbow'
require 'rake/clean'


task :test, [:pattern] do |t, args|
  `make`

  FileUtils.cd("test") do
    overall_result = true
    overall_passed = 0
    overall_failed = 0

    `rm *.taints`
    `cp ../taintlib/*.taints .`

    Dir.glob("*.c") do |file|
      next if (args.pattern != nil and not file.match(args.pattern))

      file = File.basename(file, ".*")

      exp_file = File.readlines("#{file}.c").join

      exp_map = {}
      out_map = {}
      def_map = {}

      exp_file.scan(/__expected:(.+)\((.*)\)/) { |m| exp_map[m[0]] = m[1].split(', ') }
      exp_file.scan(/__define:(.+)\((.*)\)/) { |m| def_map[m[0]] = m[1].split(', ') }

      create_taint_file(def_map)

       `clang -emit-llvm -c #{file}.c -o #{file}.bc`
      ["", "-mem2reg", "-reg2mem", "-mem2reg -reg2mem", "-adce", "-mem2reg -adce"].each do |opt|
        opt_out = `opt -load ../Debug+Asserts/lib/dataflow.so #{opt} -instnamer -dataflow < #{file}.bc -o /dev/null 2>&1`
        opt_out.scan(/__taints:(.+)\((.*)\)/) { |m| out_map[m[0]] = m[1].split(', ') }

        File.open(file + "#{opt}.log", "w") do |logfile|
          logfile.puts opt_out
        end

        (test_result, passed_count, failed_count) = test(file, exp_map, out_map, opt)

        overall_passed += passed_count
        overall_failed += failed_count
        overall_result &&= test_result
      end
    end

    puts
    puts "------------------------"
    print " Overall result:  ".bright
    print "#{overall_passed} PASSED  ".color(:green).bright
    print "#{overall_failed} FAILED".color(:red).bright
    puts
  end
end

def test(file, exp_map, out_map, opts)
  test_result = true
  failed_count = 0
  passed_count = 0

  unless exp_map.length == out_map.length
    print_failed(file, "", "Function count mismatch. Expected #{exp_map.length} but was #{out_map.length}", opts)
    puts exp_map.keys
    puts " --- But was ---".color(:yellow)
    puts out_map.keys
    test_result &&= false
  end
  
  exp_map.each do |function, taints|
    unless out_map.has_key? function
      print_failed(file, function, "function `#{function}` not found", opts)
      test_result &&= false
      failed_count += 1
      next
    end

    out_taints = out_map[function]
    unless taints.length == out_taints.length
      print_failed(file, function, "Taint count mismatch. Expected #{taints.length} but was #{out_taints.length}", opts)
      puts " --- Expected ---".color(:yellow)
      puts taints
      puts " --- But was ---".color(:yellow)
      puts out_taints
      test_result &&= false
      failed_count += 1
      next
    end

    single_taint_check_result = true
    taints.each do |taint|
      unless out_taints.include? taint
        print_failed(file, function, "Expected taint missing: " + taint.color(:cyan), opts)
        test_result &&= false
        failed_count += 1
        single_taint_check_result = false
      end
    end
  
    next unless single_taint_check_result

    print_passed(file, function, opts)
    passed_count += 1
  end

  [test_result, passed_count, failed_count]
end

def create_taint_file(def_map)
  def_map.each do |function, taints|
    File.open(function + ".taints", "w") do |f|
      f.puts taints.join("\n")
    end
  end
end

def print_failed(file, function, reason, opts)
  print " * " + "FAILED".bright.color(:red)
  print " [#{opts}]".color("#AAAAAA")
  print " : " + file.color(:blue) 
  print "." + function.italic if function.length > 0
  print " -- " + reason.color(:yellow)
  print "\n"
end

def print_passed(file, function, opts)
  print " * " + "PASSED".bright.color(:green)
  print " [#{opts}]".color("#AAAAAA")
  print " : " + file.color(:blue) 
  print "." + function.italic
  print "\n"
end

CLEAN.include('test/*.taints')
