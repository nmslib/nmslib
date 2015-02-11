import logging, bz2, random

from gensim import interfaces, matutils
from gensim.corpora import IndexedCorpus

inpObj = matutils.MmReader(bz2.BZ2File('sparse_wiki_tfidf.mm.bz2'))

outObj1 = matutils.MmWriter('sparse_wiki_tfidf_part1.mm')
outObj2 = matutils.MmWriter('sparse_wiki_tfidf_part2.mm')
docNo = 1

prob = 0.5

for d in inpObj:
  if random.random() > prob :
    outObj1.write_vector(docNo, d)
  else:
    outObj2.write_vector(docNo, d)
  docNo = docNo + 1

outObj1.close()
outObj2.close()

