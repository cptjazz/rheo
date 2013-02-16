#!/bin/bash
opt -instnamer -dot-cfg -dot-dom -dot-postdom $1

for i in $( ls *.dot ); do
  dot -Tpdf $i > $i.pdf
done
