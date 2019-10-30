#!/usr/bin/env python
import sys,os
from random import randint

sys.path.append(os.path.relpath("scripts/data"))
from sparse_hlp import str2SparseVect,sparseVect2Str

if len(sys.argv) != 3:
  print "Usage: <input sparse file> <output sparse file with randomly flipped signs>"
  sys.exit(1)

fin = open(sys.argv[1])
fout = open(sys.argv[2], 'w')

for line in fin:
  x=str2SparseVect(line)
  cols=x.nonzero()[1]
  for ci in cols:
    if randint(0,1)==0: x[0,ci]=-x[0,ci]
  fout.write(sparseVect2Str(x)+'\n')

fout.close()

