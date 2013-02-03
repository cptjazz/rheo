#!/bin/bash
make
cd test/
./compile.sh
cd ..
./run_opt.sh
