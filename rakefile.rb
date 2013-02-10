require 'fileutils'
require 'rainbow'

task :test do
  `make`

  FileUtils.cd("test") do
    overall_result = true
    overall_passed = 0
    overall_failed = 0

    Dir.glob("*.c") do |file|
      file = File.basename(file, ".*")

      `clang -emit-llvm -c #{file}.c -o #{file}.bc`
      opt_out = `opt -load ../Debug+Asserts/lib/dataflow.so -dataflow < #{file}.bc -o /dev/null 2>&1`

      exp_file = File.readlines("#{file}.c").join

      exp_map = {}
      out_map = {}

      exp_file.scan(/__expected:(.+)\((.*)\)/) { |m| exp_map[m[0]] = m[1].split(', ') }
      opt_out.scan(/__taints:(.+)\((.*)\)/) { |m| out_map[m[0]] = m[1].split(', ') }

      test_result = test(file, exp_map, out_map)

      overall_passed += 1 if test_result
      overall_failed += 1 unless test_result
      overall_result &&= test_result
    end

    puts
    puts "------------------------"
    print " Overall result:  ".bright
    print "#{overall_passed} PASSED  ".color(:green).bright
    print "#{overall_failed} FAILED".color(:red).bright
    puts
  end
end

def test(file, exp_map, out_map)
  test_result = true
  unless exp_map.length == out_map.length
    print_failed(file, "", "Function count mismatch. Expected #{exp_map.length} but was #{out_map.length}")
    puts exp_map
    puts " ----- but was"
    puts out_map
    test_result &&= false
  end
  
  exp_map.each do |function, taints|
    unless out_map.has_key? function
      print_failed(file, function, "function `#{function}` not found")
      puts " --- Expected ---".color(:yellow)
      puts exp_map
      puts " --- But was ---".color(:yellow)
      puts out_map
      test_result &&= false
      next
    end

    out_taints = out_map[function]
    unless taints.length == out_taints.length
      print_failed(file, function, "Taint count mismatch. Expected #{taints.length} but was #{out_taints.length}")
      puts " --- Expected ---".color(:yellow)
      puts taints
      puts " --- But was ---".color(:yellow)
      puts out_taints
      test_result &&= false
    end

    taints.each do |taint|
      unless out_taints.include? taint
        print_failed(file, function, "Expected taint missing: " + taint.color(:cyan))
        test_result &&= false
        next
      end
    end
  
    print_passed(file, function)
  end

  test_result
end

def print_failed(file, function, reason)
  print " * " + "FAILED".bright.color(:red)
  print " : " + file.color(:blue) 
  print "." + function.italic if function.length > 0
  print " -- " + reason.color(:yellow)
  print "\n"
end

def print_passed(file, function)
  print " * " + "PASSED".bright.color(:green)
  print " : " + file.color(:blue) 
  print "." + function.italic
  print "\n"
end
