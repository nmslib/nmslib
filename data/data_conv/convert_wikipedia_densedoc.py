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

def writeDoc(docId, outf, currDoc, lineNum, nc):
  tfs = []

  prevId = 0;

  vals = dict()
  for (i, tf) in currDoc: 
    vals[int(i)] = tf

  for i in range(1, nc + 1):
    if vals.has_key(i):
      tfs.append(str(vals[i]))
    else:
      tfs.append('0')

  if len(tfs) != nc: 
    print tfs
    raise Exception("bug wrong number of elements!")
  #print len(tfs)

  outf.write(" ".join(tfs)+"\n")
  outf.flush()

prevDocId = -1
lineNum = 0
docNo = 0
nc = 0
for line in inf:
  lineNum = lineNum+1
  if lineNum == 2:
    nc = int((line.strip().split())[1])
  if lineNum < 3: continue
  (docId, termId, tfidf) = line.strip().split() 
  if docId != prevDocId:
    if len(currDoc)>0: 
      writeDoc(docId, outf, currDoc, lineNum, nc)
      docNo = docNo + 1
      if docNo % 1000 == 0: print("Processed: " + str(docNo))
    prevDocId = docId
    currDoc = []

  currDoc.append([termId, tfidf])


# The last doc
if len(currDoc)>0:
  writeDoc(docId, outf, currDoc, lineNum, nc)

print("Processed: " + str(docNo))

outf.close()

