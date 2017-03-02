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
#ifndef DOCENTRY_H
#define DOCENTRY_H

#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_map>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

//#define DEBUG_CHECK

#define USE_GOOGLE_SPARSE_HASH_FOR_PAYLOADS
#define PRECOMPUTE_TRAN_TABLES

/*
 * Tran table is experimental only and it's up to
 * an order of magnitude slower. Anyways, if you use
 * it to test things, don't use it with Google sparse hash,
 * which surprisingly fails miserably on this task (by being
 * 100x slower than the standard C++ unordered_map).
 * Google sparse hash is activated by defining the flag:
 *   USE_GOOGLE_SPARSE_HASH_FOR_TRAN_TABLE
 */
//#define USE_HASH_BASED_TRAN_TABLE
//#define HASH_TRAN_TABLE_ESTIM_SIZE  50000000
//#define USE_GOOGLE_SPARSE_HASH_FOR_TRAN_TABLE

//#define USE_NON_IDF_AVG_EMBED

namespace similarity {

/* 
  Let's stick to 4-byte types.
  WordID must be a signed type.
*/
typedef int32_t   WORD_ID_TYPE;
typedef float     IDF_TYPE;
typedef uint32_t  QTY_TYPE;
typedef uint32_t  FIELD_QTY_TYPE;

using std::string;
using std::unordered_map;
using std::vector;
using std::max;

typedef unordered_map<string, WORD_ID_TYPE> Str2WordId;
typedef unordered_map<size_t, size_t>       Size2Size;

struct OneTranEntry {
  WORD_ID_TYPE  mWordId;
  WORD_ID_TYPE  mIq;        // the zero-based number of a word in question array
  float         mTranProb;

  OneTranEntry(WORD_ID_TYPE mWordId, WORD_ID_TYPE mIq, float mTranProb) : mWordId(mWordId), mIq(mIq),
                                                                          mTranProb(mTranProb) { }
  bool operator<(const OneTranEntry& e) const {
    if (mWordId != e.mWordId) return mWordId < e.mWordId;
    return mIq < e.mIq;
  }
};

struct OneTranEntryShort {
  WORD_ID_TYPE  mIq;        // the zero-based number of a word in question array
  float         mTranProb;
  OneTranEntryShort(WORD_ID_TYPE iq, float tranProb) : mIq(iq), mTranProb(tranProb) {}
};

struct DocEntryPtr {
  QTY_TYPE       mWordIdsQty = 0;      //  a number of elements for mpWordIds, mpQtys, mpBM25IDF, mpLuceneIDF
  QTY_TYPE       mWordIdsTotalQty = 0; //  a total number of unique words in a document
  QTY_TYPE       mWordIdSeqQty = 0; // a number of elements for mpWordIdSeq, it could be zero when mWordIdsTotalQty > 0,
                                    // b/c we do not necessarily store 
  QTY_TYPE       mWordEmbedDim = 0; ; // a dimensionality of averaged word embeddings

  const WORD_ID_TYPE*  mpWordIds = NULL;   // unique word ids
  const IDF_TYPE*      mpBM25IDF = NULL;
  const IDF_TYPE*      mpLuceneIDF = NULL;
  const QTY_TYPE*      mpQtys = NULL;      // # of word occurrences corresponding to memorized ids
  const WORD_ID_TYPE*  mpWordIdSeq = NULL; // a sequence of word IDs (can contain repeats)
#ifdef PRECOMPUTE_TRAN_TABLES
  int32_t                   mTranEntryQty = -1;
  const WORD_ID_TYPE*       mpTranWordIds = NULL; // wordIds associated with mpWordIds via translation tables
  const OneTranEntryShort*  mpTranEntries = NULL;
#endif
#ifdef USE_NON_IDF_AVG_EMBED
  const float*         mpRegAvgWordEmbed = NULL;
#endif
  const float*         mpIDFWeightAvgWordEmbed = NULL;
};

struct DocEntryHeader {
  const QTY_TYPE    mWordIdsQty;
  const QTY_TYPE    mWordIdsTotalQty;
  const QTY_TYPE    mWordIdSeqQty;
  const QTY_TYPE    mWordEmbedDim;
#ifdef PRECOMPUTE_TRAN_TABLES
  const int32_t     mTranRecQty; // negative value means that entries weren't precomputed
#endif
  DocEntryHeader(unsigned wordIdsQty,
                 unsigned wordIdsTotalQty,
                 unsigned wordIdSeqQty,
                 unsigned wordEmbedDim
#ifdef PRECOMPUTE_TRAN_TABLES
      , int tranReqQty
#endif
  ) : mWordIdsQty(wordIdsQty), mWordIdsTotalQty(wordIdsTotalQty), mWordIdSeqQty(wordIdSeqQty), mWordEmbedDim(wordEmbedDim)
#ifdef PRECOMPUTE_TRAN_TABLES
      ,mTranRecQty(tranReqQty)
#endif
  {}

  /*
   * Returns the total number of bytes necessary to store both the header and the contents.
   */
  inline size_t getTotalSize() const {
    return sizeof(*this) +
           mWordIdsQty * (sizeof(WORD_ID_TYPE) + sizeof(QTY_TYPE) + 2*sizeof(IDF_TYPE)) +
           mWordIdSeqQty * sizeof(WORD_ID_TYPE) +
           sizeof(float) * mWordEmbedDim * 
#ifdef USE_NON_IDF_AVG_EMBED
            2 /* 2 because weighted + non-weighted embeddings */
#else
            1 /* in this case only non-weighted */
#endif
           #ifdef PRECOMPUTE_TRAN_TABLES
           + max<int32_t>(0, mTranRecQty) * (sizeof(OneTranEntryShort) + sizeof(WORD_ID_TYPE));
#endif
    ;
  }
};

class InMemFwdIndexReader;

/*
 * One document entry from a forward file.
 */
struct DocEntryParser {
  DocEntryParser(const InMemFwdIndexReader& indxReader, size_t fieldId, const string& docStr);

  vector<WORD_ID_TYPE>  mvWordIds;   // unique word ids
  vector<IDF_TYPE>      mvBM25IDF;
  vector<IDF_TYPE>      mvLuceneIDF;
  vector<QTY_TYPE>      mvQtys;      // # of word occurrences corresponding to memorized ids
  unsigned              mWordIdsTotalQty = 0;
  vector<WORD_ID_TYPE>  mvWordIdSeq; // a sequence of word IDs (can contain repeats)
};


}

#endif

