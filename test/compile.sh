#!/bin/bash
clang -emit-llvm -S 1.c -o 1.ll
clang -emit-llvm -c 1.c -o 1.bc
