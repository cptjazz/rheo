#!/usr/bin/ruby
file = ARGV[0]
puts "\n"
puts file
puts "###########################################"
`opt -load Debug+Asserts/lib/dataflow.so -o /dev/null -loopanalyser -dataflow < #{file}`
