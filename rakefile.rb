require 'fileutils'
require 'rainbow'

task :test do
  FileUtils.cd("test") do
    overall_result = true

    Dir.glob("*.c") do |file|
      file = File.basename(file, ".*")

      `clang -emit-llvm -c #{file}.c -o #{file}.bc`
      opt_out = `opt -load ../Debug+Asserts/lib/dataflow.so -dataflow < #{file}.bc -o /dev/null 2>&1`

      exp_file = File.readlines("#{file}.c").join

      exp_map = {}
      out_map = {}

      exp_file.scan(/__expected:(.+)\((.+)\)/) { |m| exp_map[m[0]] = m[1].split(', ') }
      opt_out.scan(/__taints:(.+)\((.+)\)/) { |m| out_map[m[0]] = m[1].split(', ') }

      test_result = test(exp_map, out_map)
      puts " * " + file + " -- " + (test_result  ? "PASSED".color(:green) : "FAILED".color(:red) )
      if (!test_result)
        puts " --- Expected: ---"
        puts exp_map
        puts
        puts " --- But Was: ---"
        puts out_map
        puts
      end
      overall_result &&= test_result
    end

    puts
    puts "------------------------"
    puts " Overall result: " + (overall_result ? "PASSED".color(:green) : "FAILED".color(:red))
    puts
  end
end

def test(exp_map, out_map)
  return false unless exp_map.length == out_map.length

  exp_map.each do |function, taints|
    return false unless out_map.has_key? function
    out_taints = out_map[function]

    taints.each do |taint|
      return false unless out_taints.include? taint
    end
  end

  true
end
