#!/usr/bin/ruby
raise "No input file specified" if ARGV[0] == nil

file = ARGV[0][0..-2] + "bc"
opt_args = ""
opt_args = ARGV[1..ARGV.length].join " " if ARGV.length > 1

puts "\n"
puts file
puts "###########################################"
`opt -load Debug+Asserts/lib/dataflow.so -o /dev/null #{opt_args} -strip-debug-declare -globalopt -globaldce -tailcallelim -licm -sink -adce -dse -mem2reg -scalarrepl -instcombine -simplifycfg -debug-pass=Structure -debug-only=dataflow -instnamer -dataflow < #{file}`

Dir.glob("*.dot") do |dot|
  `dot -Tpdf #{dot} > #{dot}.pdf`
end
