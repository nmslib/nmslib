#!/bin/bash
for fn in *.ipynb ; do 
  echo "Running $fn"
  `which ipython` $fn
  echo "===================="
done
