/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2015
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef GIZA_TRAN_TABLE_READER_H
#define GIZA_TRAN_TABLE_READER_H

#include <string>
#include <memory>
#include <vector>
#include <functional>

#include "logging.h"
#include "giza_vocab_reader.h"
#include "space/qa/docentry.h"

#ifdef USE_HASH_BASED_TRAN_TABLE

// SIMPLE_KEY works much better
#define SIMPLE_KEY
#ifdef SIMPLE_KEY
typedef uint64_t HashKeyType;
typedef std::hash<uint64_t> PairHashFunc;
#else
typedef std::pair<similarity::WORD_ID_TYPE, similarity::WORD_ID_TYPE> HashKeyType;

struct PairHashFunc {
  size_t operator()(HashKeyType key) const {
    const size_t prime = 31;
    size_t res = 1;
    res = prime * res + hasher(key.first);
    res = prime * res + hasher(key.second);
    return res;
  }
  std::hash<similarity::WORD_ID_TYPE> hasher;
};

#endif


#ifdef USE_GOOGLE_SPARSE_HASH_FOR_TRAN_TABLE
#include <google/dense_hash_map>
typedef google::dense_hash_map<HashKeyType, float, PairHashFunc> TRAN_TABLE_HASH_TYPE   ;
#else
#include <unordered_map>
typedef std::unordered_map<HashKeyType, float, PairHashFunc> TRAN_TABLE_HASH_TYPE   ;
#endif

#endif




namespace similarity {

/*
 * A helper intermediate class to store partial (no source ID) translation
 * entries.
 * 
 */
struct TranRecNoSrcId {
  WORD_ID_TYPE  mDstId;
  float         mProb;

  TranRecNoSrcId(WORD_ID_TYPE dstId, float prob) : mDstId(dstId), mProb(prob) {;
  }

  bool operator<(const TranRecNoSrcId& o) const {
    return mDstId < o.mDstId;
  }
};


/*
 * Translation probabilities for one source word.
 * 
 */
struct GizaOneWordTranRecs {
  /*
   * Constructor.
   * 
   * sortedEntries are partial (no source ID) translation entries. 
   *               They !!MUST!! be sorted in the order of ascending mDstIds.
   */
  GizaOneWordTranRecs(vector<TranRecNoSrcId> sortedEntries) {
    mQty = sortedEntries.size();

    mpProbs   = new float[mQty];
    mpDstIds  = new WORD_ID_TYPE[mQty];

    for (size_t i = 0; i < mQty; ++i) {
      mpProbs[i] = sortedEntries[i].mProb;
      mpDstIds[i] = sortedEntries[i].mDstId;
    }
  }

  ~GizaOneWordTranRecs() {
    delete [] mpProbs;
    delete [] mpDstIds;
  }
  
  size_t          mQty;
  float*          mpProbs;
  WORD_ID_TYPE*   mpDstIds;
};

typedef const GizaOneWordTranRecs* GizaOneWordTranRecsConstPtr;

/*
 * 
 * A helper class to read  translation table files produced by Giza or Giza++.
 * After reading, the table is flipped, i.e., source and target change places.
 *
 * 
 */
class GizaTranTableReaderAndRecoder {
public:
  GizaTranTableReaderAndRecoder(
       // input file name
       string fileName, 
       // used for filtering and recoding of string IDs.
       const VocabularyFilterAndRecoder& filterAndRecoder,
       // processed source vocabulary
       const GizaVocabularyReader& vocSrc,
       // processed target vocabulary
       const GizaVocabularyReader& vocDst,
       float probSelfTran,
       // for rescaling purposes: a probability of translating a word into itself.
       float tranProbThreshold
       // a threshold for the translation probability: records with values
       // below the threshold are discarded.  
  );
  const GizaOneWordTranRecs* getTranProbsOrig(WORD_ID_TYPE wordId) const {
     if (wordId < 0 || static_cast<size_t>(wordId) > mMaxWordId) return NULL;
     return mTranProbOrig[wordId].get();
  }
  const GizaOneWordTranRecs* getTranProbsFlipped(WORD_ID_TYPE wordId) const {
    if (wordId < 0 || static_cast<size_t>(wordId) > mMaxWordId) return NULL;
    return mTranProbFlipped[wordId].get();
  }
#ifdef USE_HASH_BASED_TRAN_TABLE
#ifdef SIMPLE_KEY
  uint64_t makeKey(WORD_ID_TYPE id1, WORD_ID_TYPE id2) const {
    uint64_t res = id1;
    //res <<= 32;
    res <<= mMaxWordIdShift;
    return (res | id2);
  }
#else
  std::pair<WORD_ID_TYPE, WORD_ID_TYPE> makeKey(WORD_ID_TYPE id1, WORD_ID_TYPE id2) const {
    return std::make_pair(id1, id2);
  }
#endif

  bool getTranProbHash(WORD_ID_TYPE srcWordId, WORD_ID_TYPE dstWordId, float& prob) const {
    if (dstWordId < 0 || static_cast<size_t>(dstWordId) > mMaxWordId) return false;
    if (srcWordId < 0 || static_cast<size_t>(srcWordId) > mMaxWordId) return false;
    const auto it = mTranHashProb->find(makeKey(srcWordId, dstWordId));
    if (it == mTranHashProb->end()) return false;
    prob = it->second;
    return true;
  }
#endif
  void procOneWord(WORD_ID_TYPE prevSrcId, vector<TranRecNoSrcId>& tranRecs);    
  void flipTranTable();
 
  // Note that by design the vector always contains at least one element
  // see the constructor of this class 
  const float* getSrcWordProbTable() const { return &mSrcWordProb[0]; }
private:
  const VocabularyFilterAndRecoder&   mFilterAndRecoder;

  float         mProbSelfTran = 0;
  const size_t  mMaxWordId;
  size_t        mMaxWordIdShift;

  vector<unique_ptr<GizaOneWordTranRecs>>               mTranProbOrig;
  vector<unique_ptr<GizaOneWordTranRecs>>               mTranProbFlipped;
#ifdef USE_HASH_BASED_TRAN_TABLE
  unique_ptr<TRAN_TABLE_HASH_TYPE>                      mTranHashProb;
#endif

  vector<float>                           mSrcWordProb;
};

};

#endif
