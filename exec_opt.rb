#!/usr/bin/ruby
raise "No input file specified" if ARGV[0] == nil
file = ARGV[0][0..-2] + "bc"
puts "\n"
puts file
puts "###########################################"
`opt -load Debug+Asserts/lib/dataflow.so -o /dev/null -instnamer -dataflow < #{file}`

Dir.glob("*.dot") do |dot|
  `dot -Tpdf #{dot} > #{dot}.pdf`
end
