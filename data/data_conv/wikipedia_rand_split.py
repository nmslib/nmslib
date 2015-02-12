#!/usr/bin/env python
import logging, bz2, random

from gensim import interfaces, matutils
from gensim.corpora import IndexedCorpus

numDoc1 = 0
numRow1 = 0
numCol1 = 0
numDoc2 = 0
numRow2 = 0
numCol2 = 0

split = []

inpObj = matutils.MmReader(bz2.BZ2File('sparse_wiki_tfidf.mm.bz2'))

prob = 0.5

print "The first pass"

docNo = 1

for (id, vect) in inpObj:
  if len(vect) > 0:
    if random.random() > prob :
      numDoc1 = numDoc1 + 1
      numRow1 = numRow1 + len(vect)
      # This is the maximum word id among all the tuples and numCol1
      numCol1 = max(numCol1,max(vect)[0])
      split.append(0)
    else:
      numDoc2 = numDoc2 + 1
      numRow2 = numRow2 + len(vect)
      numCol2 = max(numCol2,max(vect)[0])
      split.append(1)
  if docNo % 1000 == 0:
    print docNo
  docNo = docNo + 1
  #if docNo > 100: break

# TODO I wonder if writing zeros in the headers is Ok
outObj1 = matutils.MmWriter('sparse_wiki_tfidf_part1.mm')
outObj1.write_headers(numDoc1, numCol1, numRow1)
outObj2 = matutils.MmWriter('sparse_wiki_tfidf_part2.mm')
outObj2.write_headers(numDoc2, numCol2, numRow2)

inpObj = matutils.MmReader(bz2.BZ2File('sparse_wiki_tfidf.mm.bz2'))
print "The second pass"

docNo = 1
numDoc1 = 0
numDoc2 = 0

for (id, vect) in inpObj:
  # see the commented out above, this is for debug purposes only,
  # to stop if we process incomplete collection
  if docNo > len(split): break
  if split[docNo - 1] == 0:
    outObj1.write_vector(numDoc1, vect)
    numDoc1 = numDoc1 + 1
  else:
    outObj2.write_vector(numDoc2, vect)
    numDoc2 = numDoc2 + 1
  if docNo % 1000 == 0:
    print docNo
  docNo = docNo + 1

outObj1.close()
outObj2.close()

