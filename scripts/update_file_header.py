#!/usr/bin/env python3
import sys
import os
import datetime

now = datetime.datetime.now()

def printErr(str):
  sys.stderr.write(str + '\n')

if len(sys.argv) != 3:
  printErr("PARAMETERS: <input file> <output file>") 
  sys.exit(1)

inFileName = sys.argv[1]
outFileName = sys.argv[2]
  
outFile = open(outFileName, 'w')
inFile = open(inFileName, 'r')

lines = inFile.readlines()

mainDevelList = 'Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak'

newHeader =  """/**
 * Non-metric Space Library
 *
 * Main developers: %s
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-%d
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
""" 

lenQty = len(lines)

headEnd = None
if lenQty > 2 and lines[0].strip() == '/**' and lines[1].strip() == '* Non-metric Space Library':
  for ln in range(lenQty):
    if lines[ln].strip() == '*/':
      headEnd = ln
      break
  if headEnd is None:
    printErr('Cannot find the end of the template header in the file %s' % inFileName)
    sys.exit(1)
else:
  print('WRANING Cannot find a template header in the file %s, IGNORING' % inFileName)
  sys.exit(1)

outFile.write(newHeader % (mainDevelList, now.year))

for ln in range(headEnd + 1, lenQty):
  outFile.write(lines[ln])

outFile.close()
