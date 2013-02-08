#!/bin/bash
make
cd test/
./compile.rb
cd ..
./exec_opt.rb
