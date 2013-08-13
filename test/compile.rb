#!/usr/bin/ruby

Dir.glob("*.c") do |file|
  `clang -m32 -g -emit-llvm -O0 -c #{file} -o #{file[0..-2]}bc`
end
