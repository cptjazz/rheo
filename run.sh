#!/bin/bash
make -j5
rm *.taints
rm *.taints.temp
cd test/
./compile.rb
cd ..
./exec_opt.rb $1 $2 $3 $4 $5
