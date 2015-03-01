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
parser.add_argument('-e', '--eps', type=float, default=0.0, help='Use this value instead of 0.0')
parser.add_argument('-n', '--norm', action='store_true', help='Do we normalize vectors?')

args = parser.parse_args()

inf  = open(args.inpf, "r")
outf = open(args.outf, "w")
eps  = args.eps
doNorm = args.norm

print "Normalization flag: {} eps {}".format(doNorm, eps)

currDoc = []

nNormalQty = 0

def writeDoc(docId, outf, currDoc, lineNum, nc, eps, doNorm):
  norm = 0
  tfs = []

  prevId = 0;

  vals = dict()
  for (i, tf) in currDoc: 
    vals[int(i)] = float(tf)

  for i in range(1, nc + 1):
    if vals.has_key(i):
      tfs.append(vals[i])
    else:
      tfs.append(eps)

  if len(tfs) != nc: 
    print tfs
    raise Exception("bug wrong number of elements!")
  #print len(tfs)

  if doNorm:
    vectSum = 0
    for i in range(0, nc):
      vectSum = vectSum + tfs[i]
    if abs(1.0 - vectSum) > eps: 
      #print "Normalizing, sum = {}".format(vectSum)
      norm=1
      for i in range(0, nc):
        tfs[i] = tfs[i] / vectSum
    #else:
      #print "No normalizing"

  for i in range(0, nc):
    tfs[i] = str(tfs[i])

  outf.write(" ".join(tfs)+"\n")
  outf.flush()
  return norm

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
      nNormalQty = nNormalQty + writeDoc(docId, outf, currDoc, lineNum, nc, eps, doNorm)
      docNo = docNo + 1
      if docNo % 100000 == 0: print("Processed: " + str(docNo))
    prevDocId = docId
    currDoc = []

  currDoc.append([termId, tfidf])


# The last doc
if len(currDoc)>0:
  nNormalQty = nNormalQty + writeDoc(docId, outf, currDoc, lineNum, nc, eps, doNorm)

print("Processed: " + str(docNo) + " normalized: " + str(nNormalQty))

outf.close()
