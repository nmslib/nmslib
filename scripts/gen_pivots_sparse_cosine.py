#!/usr/bin/env python
import sys
import random

if len(sys.argv) != 5:
  print "Usage: <number of pivots> <number of terms per pivot> <maximum word ID> <output file>"
  sys.exit(1)

pivotQty=int(sys.argv[1])
termNum=int(sys.argv[2])
maxID=int(sys.argv[3])
outFile=sys.argv[4]
print "Will generate %d pivots each having %d keyword IDs from 1 to %d, save to %s" % (pivotQty, termNum, maxID, outFile)

fout=open(outFile, 'w')
for i in range(0, pivotQty):
  pivArr=[]
  seen={}
  for k in range(0, termNum):
    id = random.randint(1, maxID)
    while id in seen:
      id = random.randint(1, maxID)
    seen[id]=1
    pivArr.append(id)
  pivArr.sort()
  pivArrMod =[str(id) + ':1' for id in pivArr]
  fout.write(' '.join(pivArrMod) + '\n');
  if (i+1) % 100 == 0: print "# of pivots %d" % (i+1)

fout.close()
