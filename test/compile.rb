#!/usr/bin/ruby

Dir.glob("*.c") do |file|
  `clang -emit-llvm -c #{file} -o #{file[0..-2]}bc`
end