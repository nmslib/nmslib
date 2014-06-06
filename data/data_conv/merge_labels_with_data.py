#!/usr/bin/env python
# Merging labels with respective data files

import sys

labFileName = sys.argv[1]
dataFileName = sys.argv[2]

labs = open(labFileName, 'r')

data = open(dataFileName, 'r')

labLines = labs.readlines()
dataLines = data.readlines()

if len(labLines) != len(dataLines):
  raise Exception("Unequal # of lines in input files: {} vs {}".format(len(labLines),len(dataLines)))

for i in range(0, len(labLines)):
  print "label:{} {}".format(labLines[i].strip(), dataLines[i].strip())

