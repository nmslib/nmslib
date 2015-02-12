#!/usr/bin/env python
import logging, gensim, bz2, sys
logging.basicConfig(format='%(asctime)s : %(levelname)s : %(message)s', level=logging.INFO)

# load id->word mapping (the dictionary), one of the results of step 2 above
id2word = gensim.corpora.Dictionary.load_from_text('sparse_wiki_wordids.txt')
# load corpus iterator
mmTrain = gensim.corpora.MmCorpus(bz2.BZ2File('sparse_wiki_tfidf_part1.mm.bz2')) 
mmTest = gensim.corpora.MmCorpus(bz2.BZ2File('sparse_wiki_tfidf_part2.mm.bz2')) 

print mmTrain
print mmTest

if len(sys.argv) != 3:
  raise Exception("Usage: <number of topics> <number of cores>")

ntop=int(sys.argv[1])
ncores=int(sys.argv[2])
print "Using " + str(ntop) + " topics and " + str(ncores) + " cores"

lda = gensim.models.LdaMulticore(corpus=mmTrain, id2word=id2word, num_topics= ntop, workers=ncores)

lda_file = 'LDA/lda'+str(ntop)

lda.save(lda_file)

out_vect = 'LDA/wikipedia_lda'+str(ntop)+'.txt'
#gensim.corpora.MmCorpus.serialize(out_vect, (gensim.matutils.unitvec(vec) for vec in lda[mmTest]))
gensim.corpora.MmCorpus.serialize(out_vect, lda[mmTest])

