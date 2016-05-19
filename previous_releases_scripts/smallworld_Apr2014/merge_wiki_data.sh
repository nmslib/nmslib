#!/bin/bash
f=1
for n in 125 250 500 1000 2000 4000 8000 16000 32000 ; do
  if [ "$f" = "1" ] ; then
    cat ResultsCosineSimil/wikipedia_${n}00/res_K=10.dat
  else
    cat ResultsCosineSimil/wikipedia_${n}00/res_K=10.dat|grep -v "^MethodName"
  fi
  f=0
done
