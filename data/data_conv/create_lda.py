#!/usr/bin/env python
import logging, gensim, bz2
logging.basicConfig(format='%(asctime)s : %(levelname)s : %(message)s', level=logging.INFO)

# load id->word mapping (the dictionary), one of the results of step 2 above
id2word = gensim.corpora.Dictionary.load_from_text('sparse_wiki_wordids.txt')
# load corpus iterator
mm = gensim.corpora.MmCorpus(bz2.BZ2File('sparse_wiki_tfidf.mm.bz2')) 

print mm

ntop=128

lda = gensim.models.ldamodel.LdaModel(corpus=mm, id2word=id2word, num_topics= ntop, update_every=0, passes=20)

lda_file = 'LDA/lda'+str(ntop)

lda.save(lda_file)

out_vect = 'LDA/wikipedia_lda'+str(ntop)+'.txt'
gensim.corpora.MmCorpus.serialize(out_vect, (gensim.matutils.unitvec(vec) for vec in lda[mm]))

