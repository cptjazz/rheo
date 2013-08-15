require 'fileutils'
require 'rainbow'
require 'rake/clean'
require 'pty'

@opt_args = "-strip-debug-declare -globalopt -globaldce -instcombine -tailcallelim -licm -sink -adce -dse -mem2reg -scalarrepl -dataflow"

desc "Run all tests, but only show failed ones"
namespace :test do
  task :failed_only, [:pattern] do |t, args|
    run_tests(:failed_only, args)
  end
end

desc "Run all tests"
task :test, [:pattern] do |t, args|
  run_tests(:all, args)
end

task :analyse, [:file] do |t, args|
  analyse(args)
end

def analyse(args)
  puts " * Compiling RHEO"
  `make ENABLE_OPTIMIZED=1`

  file = args.file || "/tmp/taint-flow.bc"
  file = File.absolute_path(file)

  FileUtils.rm_rf("output")
  sleep 0.1
  FileUtils.mkdir("output")

  if Dir.exist?("taintlib")
    `cp -R taintlib output/` 
  end

  if File.exist?("requests.list")
    `cp requests.list output/`
  end

  if File.exist?("exclude.list")
    `cp exclude.list output/`
  end

  FileUtils.cd("output") do
    log_file = File.open("analysis.log", "w")
    puts " * Preprocessing BC file"

    opt_cmd = "opt -load ../Release+Asserts/lib/dataflow.so #{@opt_args} < #{file} -o /dev/null 2>&1"
    begin
      PTY.spawn(opt_cmd) do |r, w, pid|
        begin
          externals = []
          function_count = i_th_func = 1
          arg_count = 0
          arg_no = 0

          r.each do |line|
            log_file.puts line
            log_file.flush

            if line =~ /__info:arg_count:(.*)/
              arg_count = $1.to_i
              print "%0#{arg_count.to_s.length}d/%d" % [0, arg_count]
            end

            if line =~ /__info:arg_no:(.*)/
              arg_no = $1.to_i
              print "\b" * (arg_count.to_s.length*2 + 1)
              print "%0#{arg_count.to_s.length}d/%d" % [arg_no, arg_count]
            end

            if line =~ /__log:start:(.*)/
              print " (%0#{function_count.to_s.length}d/%d) ".color("#888888") % [i_th_func, function_count] + $1.strip.bright + " ... "
            end
            
            if line =~ /__taints:(.*)\((.*)\)/
              taints = ($2 || "").strip
              taints = taints[0..40] + "..." + " [and many more]".color(:magenta) if taints.length > 50
              taints = " " * 10 if taints.length == 0
              print "\b" * (arg_count.to_s.length*2 + 1)
              puts "done".color(:green) +  "  #{taints}".color("#aaaaaa")
              i_th_func += 1
            end

            if line =~ /__defer:(.*):(.*)/
              dependency = ($2 || "").strip
              dependency_string = dependency ? " (waiting for `#{dependency}` to complete)" : ""
              print "\b" * (arg_count.to_s.length*2 + 1)
              puts "deferred".color(:yellow) + dependency_string.color("#333333")
            end

            if line =~ /__error:(.*)/
              print "\b" * (arg_count.to_s.length*2 + 1)
              puts "error: #{$1.strip}".color(:red)
            end

            if line =~ /__enqueue:start/
              puts " * Inspecting Call Graph"
            end
            
            if line =~ /__enqueue:end/
              puts " * External functions that will be handled by heuristic: " + externals.join(", ").color(:blue)
              puts
            end

            if line =~ /__enqueue:count:(.*)/
              puts " # Functions to analyse: #{$1.strip}".bright
              puts
              function_count = $1.to_i
            end

            if line =~ /__external:(.*)/
              externals << $1.strip
            end
          end
        rescue Errno::EIO  
        end  
      end
    rescue PTY::ChildExited => e  
    end  

    log_file.close
  end

end

def run_tests(show_only, args)
  `make ENABLE_OPTIMIZED=1`

  FileUtils.cd("test") do
    overall_result = true
    overall_passed = 0
    overall_failed = 0

    Dir.glob("*.c").sort.each do |file|
      next if (args.pattern != nil and not file.match(args.pattern))

      file = File.basename(file, ".*")

      exp_file = File.readlines("#{file}.c").join

      exp_map = {}
      def_map = {}

      exp_file.scan(/__expected:(.+)\((.*)\)/) { |m| exp_map[m[0]] = m[1].split(', ') }
      exp_file.scan(/__define:(.+)\((.*)\)/) { |m| def_map[m[0]] = m[1].split(', ') }

      `clang -emit-llvm -g -c #{file}.c -o #{file}.bc`
      `rm *.taints 2>/dev/null`
      `rm *.taints.temp 2>/dev/null`

      create_taint_file(def_map)
      out_map = {}

      opt_out = `opt -load ../Release+Asserts/lib/dataflow.so #{@opt_args} < #{file}.bc -o /dev/null 2>&1`

      opt_out.scan(/__taints:(.+)\((.*)\)/) { |m| out_map[m[0]] = m[1].split(', ') }

      File.open(file + ".log", "w") do |logfile|
        logfile.puts opt_out
      end

      (test_result, passed_count, failed_count) = test(file, exp_map, out_map, show_only)

      overall_passed += passed_count
      overall_failed += failed_count
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

def test(file, exp_map, out_map, show_only)
  test_result = true
  failed_count = 0
  passed_count = 0

  unless exp_map.length == out_map.length
    print_failed(file, "", "Function count mismatch. Expected #{exp_map.length} but was #{out_map.length}")
    puts exp_map.keys
    puts " --- But was ---".color(:yellow)
    puts out_map.keys
    test_result &&= false
  end
  
  exp_map.sort.each do |function, taints|
    unless out_map.has_key? function
      print_failed(file, function, "function `#{function}` not found")
      test_result &&= false
      failed_count += 1
      next
    end

    out_taints = out_map[function]
    unless taints.length == out_taints.length
      print_failed(file, function, "Taint count mismatch. Expected #{taints.length} but was #{out_taints.length}")
      puts " --- Expected ---".color(:yellow)
      puts taints.sort
      puts " --- But was ---".color(:yellow)
      puts out_taints.sort
      test_result &&= false
      failed_count += 1
      next
    end

    single_taint_check_result = true
    taints.each do |taint|
      unless out_taints.include? taint
        print_failed(file, function, "Expected taint missing: " + taint.color(:cyan))
        test_result &&= false
        failed_count += 1
        single_taint_check_result = false
      end
    end
  
    next unless single_taint_check_result

    print_passed(file, function) if show_only == :all
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

CLEAN.include('test/*.taints')
