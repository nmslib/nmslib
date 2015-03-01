#!/usr/bin/env python
#
# This script is released under the
# Apache License Version 2.0 http://www.apache.org/licenses/.
#
# It is used to convert sparse documents produced by
# the gensim library (http://radimrehurek.com/gensim/wiki.html)
# to a format that our software can understand
#

import os
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('-i', '--inpf', type=str, required=True)
parser.add_argument('-o', '--outf', type=str, required=True)

args = parser.parse_args()

inf  = open(args.inpf, "r")
outf = open(args.outf, "w")

currDoc = []

prevDocId = -1
lineNum = 0
docNo = 0
for line in inf:
  lineNum = lineNum+1
  if lineNum < 3: continue
  (docId, termId, tfidf) = line.strip().split() 
  if docId != prevDocId:
    if len(currDoc)>0: 
      outf.write("\t".join(currDoc)+"\n")
      docNo = docNo + 1
      if docNo % 1000 == 0: print("Processed: " + str(docNo))
    outf.flush()
    prevDocId = docId
    currDoc = []
  currDoc.extend([termId, tfidf])

# The last doc
if len(currDoc)>0: outf.write("\t".join(currDoc)+"\n")
print("Processed: " + str(docNo))

outf.close()

