#!/usr/bin/env python
#
# This script is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
# It is used to convert sparse documents available in
# Metric Spaces Library (http://www.sisap.org/sisap2/SISAP/Metric_Space_Library.html)
# to a format that our software can understand
#
#

import os
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-d', '--inpd', type=str, required=True)
parser.add_argument('-f', '--outf', type=str, required=True)

args = parser.parse_args()

outf = open(args.outf, "w")
listDir = os.listdir(args.inpd)
listDir.sort(key = lambda x: int(x))

for filename in listDir:
  fullpath = os.path.join(args.inpd, filename)
  if os.path.isfile(fullpath):
    filecontent = open(fullpath, "r").read().strip().split("\n")
    #line = [filename]
    line = [] # Don't output the file name (at least for now)
    line.extend([w.strip() for w in filecontent if len(w.strip()) > 0])
    outf.write("\t".join(w for w in line) + "\n")

outf.close()

