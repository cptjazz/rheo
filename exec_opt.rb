#!/usr/bin/ruby
#Dir.glob("test/*.bc") do |file|
file = "test/out_params.bc"
  puts "\n"
  puts file
  puts "###########################################"
  `opt -load Debug+Asserts/lib/dataflow.so -loopanalyser -dataflow < #{file} > /dev/null`
#end
