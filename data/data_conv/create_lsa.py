#!/usr/bin/env python
import logging, gensim, bz2
logging.basicConfig(format='%(asctime)s : %(levelname)s : %(message)s', level=logging.INFO)

# load id->word mapping (the dictionary), one of the results of step 2 above
id2word = gensim.corpora.Dictionary.load_from_text('sparse_wiki_wordids.txt')
# load corpus iterator
mm = gensim.corpora.MmCorpus(bz2.BZ2File('sparse_wiki_tfidf.mm.bz2')) 

print mm

# In reality this would mean 128 LSI topics
ntop=128

lsi = gensim.models.lsimodel.LsiModel(corpus=mm, id2word=id2word, num_topics=ntop, chunksize=10000)

lsi_file = 'LSI/lsi'+str(ntop)

lsi.save(lsi_file)

out_vect = 'LSI/wikipedia_lsi'+str(ntop)+'.txt'
gensim.corpora.MmCorpus.serialize(out_vect, (gensim.matutils.unitvec(vec) for vec in lsi[mm]))


