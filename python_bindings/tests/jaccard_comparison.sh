#!/bin/bash

for space in bit_jaccard jaccard_sparse; do
  for num_elems in 30000  100000 300000 1000000 3000000 10000000 30000000; do
    for nbits in 512 20148; do
      python jaccard_comparison.py $space $num_elems $nbits
    done
  done
done
