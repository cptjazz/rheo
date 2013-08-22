#!/usr/bin/ruby
require './opt_args.rb'

raise "No input file specified" if ARGV[0] == nil

file = ARGV[0][0..-2] + "bc"
more_opt_args = ""
more_opt_args = ARGV[1..ARGV.length].join " " if ARGV.length > 1

puts "\n"
puts file
puts "###########################################"
`opt -load Debug+Asserts/lib/dataflow.so -o /dev/null #{@opt_args} #{more_opt_args} -debug-pass=Structure -debug-only=dataflow -dataflow < #{file}`

Dir.glob("*.dot") do |dot|
  `dot -Tpdf #{dot} > #{dot}.pdf`
end
