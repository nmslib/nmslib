#!/bin/bash

set -e

for space in jaccard_sparse; do
  for num_elems in 30000  100000 300000 1000000 3000000 10000000 30000000; do
    for nbits in 512 2048; do
      python jaccard_comparison.py $space $num_elems $nbits
    done
  done
done
