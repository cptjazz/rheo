#!/usr/bin/ruby
file = ARGV[0][0..-2] + "bc"
puts "\n"
puts file
puts "###########################################"
`opt -load Debug+Asserts/lib/dataflow.so -o /dev/null -domtree -dataflow < #{file}`
