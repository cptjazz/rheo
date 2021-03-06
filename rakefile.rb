require 'fileutils'
require 'rainbow'
require 'rake/clean'
require 'pty'
require './opt_args.rb'

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

def run_tests(show_only, args)
  `make -j5 ENABLE_OPTIMIZED=1`

  @opt_args << " -dataflow"

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
