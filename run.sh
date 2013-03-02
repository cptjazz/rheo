#!/bin/bash
make
cd test/
./compile.rb
cd ..
./exec_opt.rb $1 $2 $3 $4 $5
