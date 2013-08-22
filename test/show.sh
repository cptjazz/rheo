#!/bin/bash

opt -strip-debug-declare -instcombine -constmerge -constprop -tailcallelim -licm -sink -adce -dse -mem2reg -scalarrepl -globalopt -globaldce -die -instnamer -simplifycfg $1 | llvm-dis | less
