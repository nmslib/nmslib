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

#include <cmath>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <limits>
#include <cmath>

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>

#include "space/qa/space_qa1.h"
#include "logging.h"
#include "utils.h"
#include "flexbuff.h"
#include "distcomp.h"

#include "immintrin.h"


#define USE_EXPONENTIAL_SEARCH
//#define USE_SEQ_FINDER

//#define DEBUG_CHECK
//#define DEBUG_PRINT
//#define DEBUG_PRINT_ADD

namespace similarity {

inline float safe_log(float v) {
  // ::min() returns a normalized value
  return std::max(numeric_limits<float>::min(), v);
}

using namespace std;

using boost::lexical_cast;
using boost::trim;
using boost::bad_lexical_cast;

// This should be more than enough to keep features on the stack
// in most cases
const size_t MAX_ON_STACK_FEATURE_QTY = 512;
// Again more than enough for answers
const size_t MAX_ON_STACK_ANSWER_VAL_QTY = 512;

const string QUERY_FILE_FLAG = "isQueryFile=";
const string WEIGHT_FILE = "weightFile=";
const string SIMPLE_INDEX_TYPE="simpleIndexType=";
const string USE_HASH_BASED_PAYLOADS_OPT = "useHashBasedPayloads=";
const string FIELD_INDEX_FILE = "fieldIndexFile=";
const string FIELD_FEATURES = "fieldFeatures=";
const string FIELD_TRAN_TABLE_PREFIX = "fieldTranTablePrefix=";
const string GIZA_ITER_QTY = "gizaIterQty=";
const string MIN_TRAN_PROB = "minTranProb=";
const string PROB_SELF_TRAN = "probSelfTran=";
const string LAMBDA_MODEL1 = "lambdaModel1=";
const string FIELD_EMBED_FILE = "embedFile=";

// The number of LCS features for a given field
const size_t LCS_FIELD_FEATURE_QTY = 1; /* only normalized */
// The number of Model 1 features for a given field
const size_t MODEL1_FIELD_FEATURE_QTY = 1;
// The number of simple translation features for a given field
const size_t SIMPLE_TRAN_FIELD_FEATURE_QTY = 1;
// The number of overall-match features for a given field */
const size_t OVERAL_MATCH_FIELD_FEATURE_QTY = 1;
// The number of averaged word embeddings features
#ifdef USE_NON_IDF_AVG_EMBED
const size_t AVG_EMBED_FIELD_FEATURE_QTY = 2;
#else
const size_t AVG_EMBED_FIELD_FEATURE_QTY = 1;
#endif

const char *const BM25_FEATURE_NAME = "bm25";
const char *const COSINE_FEATURE_NAME = "cosine";
const vector<string> FEATURE_TYPES = {BM25_FEATURE_NAME,
                                      "tfidf",
                                      "model1",
                                      "simple_tran",
                                      "overall_match",
                                      "lcs",
                                      COSINE_FEATURE_NAME,
                                      "avg_embed"};
// Must be all 2^i with different i
enum FeatureTypeFlags {
  BM25_FLAG = 1,
  TFIDF_LUCENE_FLAG = 2,
  MODEL1_FLAG = 4,
  SIMPLE_TRAN_FLAG = 8,
  OVERALL_MATCH_FLAG = 16,
  LCS_FLAG = 32,
  COSINE_FLAG = 64,
  AVG_EMBED_FLAG = 128
};

const vector<uint64_t> FEATURE_TYPE_MASKS = {BM25_FLAG,
                                             TFIDF_LUCENE_FLAG,
                                             MODEL1_FLAG,
                                             SIMPLE_TRAN_FLAG,
                                             OVERALL_MATCH_FLAG,
                                             LCS_FLAG,
                                             COSINE_FLAG,
                                             AVG_EMBED_FLAG};

static bool checkPrefixGetRest(const string &prefix, const string &line, string &rest) {
  rest.clear();
  size_t len = prefix.size();
  if (line.size() < len) return false;
  if (prefix == line.substr(0, len)) {
    rest = line.substr(len);
    return true;
  }

  return false;
}

static void readFeatureWeights(const string &fname, vector<float> &weights) {
  weights.clear();

  ifstream istrm(fname);
  CHECK_MSG(istrm, "Cannot open file '" + fname + "' for reading");
  istrm.exceptions(ios::badbit);


  string line;
  for (size_t lineNum = 1; getline(istrm, line); ++lineNum) {
    trim(line);
    if (!line.empty() && line[0] != '#') {
      stringstream str(line);
      string s;
      for (unsigned fn = 1; str >> s; ++fn) {
        vector<string> tmp;
        boost::split(tmp, s, boost::is_any_of(":"));
        CHECK_MSG(tmp.size() == 2, "Expected colon separated fields in line " +
                                   lexical_cast<string>(lineNum) + " field # " + lexical_cast<string>(fn) +
                                   " weight file '" + fname + "'");
        try {
          unsigned n = lexical_cast<unsigned>(tmp[0]);
          CHECK_MSG(n == fn, "Wrong value of the feature ID in line " +
                             lexical_cast<string>(lineNum) + " field # " + lexical_cast<string>(fn) +
                             " weight file '" + fname + "' expected: " + lexical_cast<string>(fn) +
                             " but got: " + tmp[0]);
        } catch (const bad_lexical_cast &) {
          CHECK_MSG(tmp.size() == 2, "Wrong format of the feature ID in line " +
                                     lexical_cast<string>(lineNum) + " field # " + lexical_cast<string>(fn) +
                                     " weight file '" + fname + "'");
        }
        try {
          float v = lexical_cast<float>(tmp[1]);
          weights.push_back(v);
        } catch (const bad_lexical_cast &) {
          CHECK_MSG(tmp.size() == 2, "Wrong format of the feature weight in line " +
                                     lexical_cast<string>(lineNum) + " field # " + lexical_cast<string>(fn) +
                                     " weight file '" + fname + "'");
        }
      }
      break;
    }
  }
  if (weights.empty()) {
    throw runtime_error("No weights found in '" + fname + "'");
  }
}

/*
 * This function works only with a flipped translation table.
 */
static void computeTranFeaturesFromFlippedTable(
    size_t fieldId,
    const SpaceParamQA1& params,
    const DocEntryPtr& queryObj, const DocEntryPtr& docObj,
    float& model1prob, float& simpleTran1, float& simpleTran2) {
  CHECK(fieldId < params.mTranTables.size());
  CHECK(params.mTranTables[fieldId].get() != NULL);
  CHECK(params.mVocSrc[fieldId].get() != NULL);

  const GizaTranTableReaderAndRecoder& answToQuestTran = *params.mTranTables[fieldId];
#ifdef DEBUG_CHECK
  const float probSelfTran      = params.mProbSelfTran[fieldId];
#endif
  const float minTranProb       = params.mMinTranProb[fieldId];
  const float lambda            = params.mLambdaModel1[fieldId];
  const float *pFieldProbTable  = answToQuestTran.getSrcWordProbTable();

  QTY_TYPE queryWordQty  = queryObj.mWordIdsQty;
  QTY_TYPE answerWordQty = docObj.mWordIdsQty;

  DECLARE_FLEXIBLE_BUFF(float, aSourceWordProb, answerWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(aSourceWordProb, aSourceWordProb + answerWordQty, 0.0);
  
  {
    float sum = 0;
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      sum += docObj.mpQtys[ia];
    }
    float invSum = 1.0f/max(1.0f, sum);
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      aSourceWordProb[ia] = invSum * float(docObj.mpQtys[ia]);
    }
  }
  
  model1prob = 0;
  simpleTran1 = simpleTran2 = 0;

  size_t shareTranPairQty = 0;
  float  logScore = 0;

  for (size_t iqOuterLoop = 0; iqOuterLoop < queryWordQty; ++iqOuterLoop) {
    WORD_ID_TYPE  queryWordId = queryObj.mpWordIds[iqOuterLoop];
    QTY_TYPE      queryRepQty = queryObj.mpQtys[iqOuterLoop];

    const GizaOneWordTranRecs* pTranRecs = queryWordId >= 0 ? answToQuestTran.getTranProbsFlipped(queryWordId) : NULL;

    float totTranProb=0;
    if (queryWordId >= 0 && NULL != pTranRecs) {
#ifdef DEBUG_CHECK
#pragma message "Using DEBUG_CHECK for additional checking, switch this off when compiling a production version!"
      bool found = false;
      for (size_t k = 0; k < pTranRecs->mQty; ++k) {
        if (pTranRecs->mpDstIds[k] == queryWordId) {
          if (probSelfTran - pTranRecs->mpProbs[k] > numeric_limits<float>::min()) {
            PREPARE_RUNTIME_ERR(err) << "Self-tranlation probability for: id=" << queryWordId << " is too small: " << pTranRecs->mpProbs[k];
            THROW_RUNTIME_ERR(err);
          }
          found = true; break;
        }
      }
      if (!found) {
        PREPARE_RUNTIME_ERR(err) << "No self-tranlation probability for: id=" << queryWordId;
        THROW_RUNTIME_ERR(err);
      }
#endif

      const WORD_ID_TYPE* const pTranWordIds = pTranRecs->mpDstIds;
      size_t                    tranRecsQty  = pTranRecs->mQty;

      const WORD_ID_TYPE* pTranWordIdsCurr = pTranRecs->mpDstIds;
      const WORD_ID_TYPE* pTranWordIdsEnd  = pTranRecs->mpDstIds + tranRecsQty;

      totTranProb = 0;

      for (size_t ia = 0; ia < answerWordQty; ++ia) {
        WORD_ID_TYPE answWordId = docObj.mpWordIds[ia];
        QTY_TYPE     answRepQty = docObj.mpQtys[ia];
        // Let's check if we have a translation entry (using the exponential search)
#ifdef USE_SEQ_FINDER
        #pragma message "Dont use this codepath unless you do some speed comparisons!"
          // Sequential search *IS* the slowest and can slow down the overall search process by 30%
          const WORD_ID_TYPE* p = pTranWordIdsCurr;
          while (p < pTranWordIdsEnd && *p < answWordId) ++p;
#else
        const WORD_ID_TYPE* pSearchEndIterator = pTranWordIdsEnd;
#ifdef USE_EXPONENTIAL_SEARCH
        // Exponential search seems to be the best
        for (ssize_t delta = 1; delta < pTranWordIdsEnd - pTranWordIdsCurr; delta *= 2) {
          if (pTranWordIdsCurr[delta] > answWordId) {
            pSearchEndIterator = pTranWordIdsCurr + delta;
            break;
          }
        }
#else
#pragma message "Not using faster version: exponential search!"
#endif
        const WORD_ID_TYPE* p = lower_bound(pTranWordIdsCurr, pSearchEndIterator, answWordId);
#endif
        if (p < pTranWordIdsEnd) {
          if (*p == answWordId) {
            float oneTranProb = pTranRecs->mpProbs[p - pTranWordIds];

            if (oneTranProb >= minTranProb) {
              totTranProb += oneTranProb * aSourceWordProb[ia];
              shareTranPairQty += answRepQty * queryRepQty;
            }
            /*
             * *p == (answWordId = docObj.mpWordIds[ia])
             * for ia + 1,
             * docObj.mpWordIds[ia + 1] > (answWordId = docObj.mpWordIds[ia]) == *p
             * Hence, following answer ids cannot match *p, but only following elements
             * (translation tables are sorted by word ids)
             */

            pTranWordIdsCurr = p + 1;
          } else {
            /*
             *  *p > (answWordId = docObj.mpWordIds[ia])
             * However, for ia + 1, it's possible:
             *  *p <= (answWordId = docObj.mpWordIds[ia+1])
             * Hence, we don't make pTranWordIdsCurr equal p + 1, but rather
             * keep it equal to p.
             */
            pTranWordIdsCurr = p;
          }
        } else break; // reached the end of the array
      }
    }
    float collectProb = queryWordId >= 0 ? max(OOV_PROB, pFieldProbTable[queryWordId]) : OOV_PROB;
    logScore += log((1-lambda)*totTranProb +lambda*collectProb);
  }

  model1prob = logScore;
  simpleTran1 = shareTranPairQty;
  simpleTran2 = shareTranPairQty / (float)
      max(1.0f, (float)queryObj.mWordIdSeqQty * docObj.mWordIdSeqQty);

}

#ifdef PRECOMPUTE_TRAN_TABLES
/*
 * These two functions work only with a flipped translation table.
 * and it requires that a query object to hold a precomputed
 * and presorted set of translation entries.
 */


/*
 * The function expects a payload with a non-null hash table.
 */
static void computeTranFeaturesFromFlippedTableHashPayload(
    size_t fieldId,
    const SpaceParamQA1& params,
    const ObjectPayload& payload,
    const DocEntryPtr& queryObj, const DocEntryPtr& docObj,
    float& model1prob, float& simpleTran1, float& simpleTran2) {
  CHECK(fieldId < params.mTranTables.size());
  CHECK(params.mTranTables[fieldId].get() != NULL);
  CHECK(params.mVocSrc[fieldId].get() != NULL);
  CHECK(queryObj.mTranEntryQty >= 0);

  const GizaTranTableReaderAndRecoder &answToQuestTran = *params.mTranTables[fieldId];
  const float minTranProb       = params.mMinTranProb[fieldId];
  const float lambda = params.mLambdaModel1[fieldId];
  const float *pFieldProbTable = answToQuestTran.getSrcWordProbTable();

  QTY_TYPE queryWordQty = queryObj.mWordIdsQty;
  QTY_TYPE answerWordQty = docObj.mWordIdsQty; 

  DECLARE_FLEXIBLE_BUFF(float, aSourceWordProb, answerWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(aSourceWordProb, aSourceWordProb + answerWordQty, 0.0);
  
  {
    float sum = 0;
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      sum += docObj.mpQtys[ia];
    }
    float invSum = 1.0f/max(1.0f, sum);
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      aSourceWordProb[ia] = invSum * float(docObj.mpQtys[ia]);
    }
  }
 

  model1prob = 0;
  simpleTran1 = simpleTran2 = 0;
  float shareTranPairQty = 0;

  float logScore = 0;

  DECLARE_FLEXIBLE_BUFF(float, queryTranProb, queryWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(queryTranProb, queryTranProb + queryWordQty, 0.0);
  DECLARE_FLEXIBLE_BUFF(float, queryShareTranPairQty, queryWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(queryShareTranPairQty, queryShareTranPairQty + queryWordQty, 0.0);

  CHECK(queryObj.mTranEntryQty >= 0);
  const OneTranEntryShort *pTranEntries = queryObj.mpTranEntries;

  const HashTablePayload &payloadType = dynamic_cast<const HashTablePayload &>(payload);
  const TABLE_HASH_TYPE *pHT = payloadType.vTranEntriesHash_[fieldId];

  CHECK_MSG(pHT != nullptr, "Expecting a non-nul hash table in this payload");
  for (size_t ia = 0; ia < answerWordQty; ++ia) {
    WORD_ID_TYPE answWordId = docObj.mpWordIds[ia];
    QTY_TYPE answRepQty = docObj.mpQtys[ia];

    auto it = pHT->find(answWordId);
    if (it != pHT->end()) {
      TranRecEntryInfo e = it->second;
      for (size_t tid = e.startIndex_; tid < e.startIndex_ + e.qty_; ++tid) {
        const OneTranEntryShort &e = pTranEntries[tid];
        float oneTranProb = e.mTranProb;

        if (oneTranProb >= minTranProb) {
          queryTranProb[e.mIq] += oneTranProb * aSourceWordProb[ia];
          queryShareTranPairQty[e.mIq] += answRepQty;
        }
      }
    }
  }

  for (size_t iq = 0; iq < queryWordQty; ++iq) {
    WORD_ID_TYPE  queryWordId = queryObj.mpWordIds[iq];
    QTY_TYPE      queryRepQty = queryObj.mpQtys[iq];

    shareTranPairQty += queryShareTranPairQty[iq] * queryRepQty;

    float collectProb = queryWordId >= 0 ?
                        max(OOV_PROB, pFieldProbTable[queryWordId]) : OOV_PROB;

    logScore += log((1-lambda)*queryTranProb[iq] +lambda*collectProb);
  }

  model1prob = logScore;
  simpleTran1 = shareTranPairQty;
  simpleTran2 = shareTranPairQty / (float)
      max(1.0f, (float)queryObj.mWordIdSeqQty * docObj.mWordIdSeqQty);

}


static void computeTranFeaturesFromFlippedTableUsingPrecompTable(
    size_t fieldId,
    const SpaceParamQA1& params,
    const DocEntryPtr& queryObj, const DocEntryPtr& docObj,
    float& model1prob, float& simpleTran1, float& simpleTran2) {
  CHECK(fieldId < params.mTranTables.size());
  CHECK(params.mTranTables[fieldId].get() != NULL);
  CHECK(params.mVocSrc[fieldId].get() != NULL);
  CHECK(queryObj.mTranEntryQty >= 0);

  const GizaTranTableReaderAndRecoder& answToQuestTran = *params.mTranTables[fieldId];
  const float lambda            = params.mLambdaModel1[fieldId];
  const float minTranProb       = params.mMinTranProb[fieldId];
  const float *pFieldProbTable  = answToQuestTran.getSrcWordProbTable();

  QTY_TYPE queryWordQty  = queryObj.mWordIdsQty;
  QTY_TYPE answerWordQty = docObj.mWordIdsQty;

  DECLARE_FLEXIBLE_BUFF(float, aSourceWordProb, answerWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(aSourceWordProb, aSourceWordProb + answerWordQty, 0.0);
  
  {
    float sum = 0;
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      sum += docObj.mpQtys[ia];
    }
    float invSum = 1.0f/max(1.0f, sum);
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      aSourceWordProb[ia] = invSum * float(docObj.mpQtys[ia]);
    }
  }


  model1prob = 0;
  simpleTran1 = simpleTran2 = 0;
  float shareTranPairQty = 0;

  float  logScore = 0;

  DECLARE_FLEXIBLE_BUFF(float, queryTranProb, queryWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(queryTranProb, queryTranProb + queryWordQty, 0.0);
  DECLARE_FLEXIBLE_BUFF(float, queryShareTranPairQty, queryWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(queryShareTranPairQty, queryShareTranPairQty + queryWordQty, 0.0);

  const WORD_ID_TYPE* const pTranWordIds = queryObj.mpTranWordIds;
  CHECK(queryObj.mTranEntryQty >= 0);
  size_t                    tranRecsQty  = queryObj.mTranEntryQty;
  const OneTranEntryShort*  pTranEntries = queryObj.mpTranEntries;

  const WORD_ID_TYPE* pTranWordIdsCurr = pTranWordIds;
  const WORD_ID_TYPE* pTranWordIdsEnd  = pTranWordIds + tranRecsQty;

  for (size_t ia = 0;
       ia < answerWordQty && pTranWordIdsCurr < pTranWordIdsEnd ;
       ++ia) {
    WORD_ID_TYPE answWordId = docObj.mpWordIds[ia];
    QTY_TYPE answRepQty = docObj.mpQtys[ia];

    // Note that pTranWordIdsCurr < pTranWordIdsEnd, hence *pTranWordIdsCurr can always be dereferenced
    if (answWordId < *pTranWordIdsCurr) continue;

    const WORD_ID_TYPE *pSearchEndIterator = pTranWordIdsEnd;
    // Exponential search
    for (ssize_t delta = 1; delta < pTranWordIdsEnd - pTranWordIdsCurr; delta <<=1) {
      if (pTranWordIdsCurr[delta] > answWordId) {
        pSearchEndIterator = pTranWordIdsCurr + delta;
        break;
      }
    }

    pTranWordIdsCurr = lower_bound(pTranWordIdsCurr, pSearchEndIterator, answWordId);

    if (pTranWordIdsCurr < pTranWordIdsEnd) {
      if (*pTranWordIdsCurr == answWordId) {
        for (;
            pTranWordIdsCurr < pTranWordIdsEnd && *pTranWordIdsCurr == answWordId;
            pTranWordIdsCurr++) {
          const OneTranEntryShort &e = pTranEntries[pTranWordIdsCurr - pTranWordIds];
          float oneTranProb = e.mTranProb;

          if (oneTranProb >= minTranProb) {
            queryTranProb[e.mIq] += oneTranProb * aSourceWordProb[ia];
            queryShareTranPairQty[e.mIq] += answRepQty;
          }
        }
        /*
         * 1) If pTranWordIdsCurr becomes >= pTranWordIdsEnd, this means one of the following
         * 1.1) The current answWordId is > than all wordIds in translation records.
         *    Because answWordIds are sorted in strictly ascending order, all the following
         *    answWordIds will be > than all wordIds in translation records as well.
         *    So, there will be no further common entries and we must stop.
         * 1.2) The lower_bound function found wordIds equal to answWordIds. However, when
         *    we iterated past these wordIds, we reached the end of translation records.
         *    Hence, all records equal to answWordIds represented the last entries in the
         *    array with translation records. Again, because answWordIds are sorted in ascending
         *    orders, all following answWordIds will be larger than all the entries in the
         *    translation record array. Again, there will be no further common entries and we must stop.
         *
         *  2) If pTranWordIdsCurr is < pTranWordIdsEnd, it points to the first wordId larger than answWordIds.
         *    This will be the next starting point, because the next answWordId will be surely larger than wordIds found
         *    by the lower_bound in this iteration (equal to the current answWordId).
         */
      } else {
        /*
         *  *pTranWordIdsCurr > (answWordId = docObj.mpWordIds[ia])
         *  However, for ia + 1, it's possible:
         *  *pTranWordIdsCurr <= (answWordId = docObj.mpWordIds[ia+1])
         *  Hence, we don't increase pTranWordIdsCurr here.
         */
        //++pTranWordIdsCurr;
      }
    }
  }

  for (size_t iq = 0; iq < queryWordQty; ++iq) {
    WORD_ID_TYPE  queryWordId = queryObj.mpWordIds[iq];
    QTY_TYPE      queryRepQty = queryObj.mpQtys[iq];

    shareTranPairQty += queryShareTranPairQty[iq] * queryRepQty;

    float collectProb = queryWordId >= 0 ?
                        max(OOV_PROB, pFieldProbTable[queryWordId]) : OOV_PROB;

    logScore += log((1-lambda)*queryTranProb[iq] +lambda*collectProb);
  }

  model1prob = logScore;
  simpleTran1 = shareTranPairQty;
  simpleTran2 = shareTranPairQty / (float)
      max(1.0f, (float)queryObj.mWordIdSeqQty * docObj.mWordIdSeqQty);
}
#endif

#ifdef USE_HASH_BASED_TRAN_TABLE
static void computeTranFeaturesFromTranBasedHash(
    size_t fieldId,
    const SpaceParamQA1& params,
    const DocEntryPtr& queryObj, const DocEntryPtr& docObj,
    float& model1prob, float& simpleTran1, float& simpleTran2) {
  CHECK(fieldId < params.mTranTables.size());
  CHECK(params.mTranTables[fieldId].get() != NULL);
  CHECK(params.mVocSrc[fieldId].get() != NULL);
  CHECK(queryObj.mTranEntryQty >= 0);

  const GizaTranTableReaderAndRecoder &answToQuestTran = *params.mTranTables[fieldId];
  const float minTranProb       = params.mMinTranProb[fieldId];
  const float lambda = params.mLambdaModel1[fieldId];
  const float *pFieldProbTable = answToQuestTran.getSrcWordProbTable();

  QTY_TYPE queryWordQty = queryObj.mWordIdsQty;
  QTY_TYPE answerWordQty = docObj.mWordIdsQty;

  DECLARE_FLEXIBLE_BUFF(float, aSourceWordProb, answerWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(aSourceWordProb, aSourceWordProb + answerWordQty, 0.0);
  
  {
    float sum = 0;
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      sum += docObj.mpQtys[ia];
    }
    float invSum = 1.0f/max(1.0f, sum);
    for (size_t ia = 0; ia < answerWordQty; ++ia) {
      aSourceWordProb[ia] = invSum * float(docObj.mpQtys[ia]);
    }
  }

  model1prob = 0;
  simpleTran1 = simpleTran2 = 0;
  float shareTranPairQty = 0;

  float logScore = 0;

  DECLARE_FLEXIBLE_BUFF(float, queryTranProb, queryWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(queryTranProb, queryTranProb + queryWordQty, 0.0);
  DECLARE_FLEXIBLE_BUFF(float, queryShareTranPairQty, queryWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
  fill(queryShareTranPairQty, queryShareTranPairQty + queryWordQty, 0.0);

  for (size_t ia = 0; ia < answerWordQty; ++ia) {
    for (size_t iq = 0; iq < queryWordQty; ++iq) {
      WORD_ID_TYPE answWordId = docObj.mpWordIds[ia], queryWordId = queryObj.mpWordIds[iq];
      QTY_TYPE answRepQty = docObj.mpQtys[ia];

      float oneTranProb;
      if (answToQuestTran.getTranProbHash(answWordId, queryWordId, oneTranProb)) {
        if (oneTranProb >= minTranProb) {
          queryTranProb[iq] += oneTranProb * aSourceWordProb[ia];
          queryShareTranPairQty[iq] += answRepQty;
        }
      }
    }
  }

  for (size_t iq = 0; iq < queryWordQty; ++iq) {
    WORD_ID_TYPE  queryWordId = queryObj.mpWordIds[iq];
    QTY_TYPE      queryRepQty = queryObj.mpQtys[iq];

    shareTranPairQty += queryShareTranPairQty[iq] * queryRepQty;

    float collectProb = queryWordId >= 0 ?
                        max(OOV_PROB, pFieldProbTable[queryWordId]) : OOV_PROB;

    logScore += log((1-lambda)*queryTranProb[iq] +lambda*collectProb);
  }

  model1prob = logScore;
  simpleTran1 = shareTranPairQty;
  simpleTran2 = shareTranPairQty / (float)
      max(1.0f, (float)queryObj.mWordIdSeqQty * docObj.mWordIdSeqQty);

}
#endif

void DataFileInputStateQA1::logFeatureMasks(const string& msg, const vector<uint64_t>& featureMasks) const {
  LOG(LIB_INFO) << "Feature masks (" << msg << ") ";
  for (size_t fieldId = 0; fieldId < featureMasks.size(); ++fieldId) {
    stringstream ss;

    uint64_t mask = featureMasks[fieldId];
    for (size_t i = 0; i < FEATURE_TYPES.size(); ++i)
    if (mask & FEATURE_TYPE_MASKS[i]){
      ss << FEATURE_TYPES[i] << " ";
    }

    LOG(LIB_INFO) << "Field id: " << fieldId << " " << ss.str();
  }
}
void DataFileInputStateQA1::logFeatureWeights(const string& msg,  const vector<float>& featureWeights) const {
  stringstream ss;
  for (auto v : featureWeights) ss << v << " ";
  LOG(LIB_INFO) << "Feature weights (" << msg << ") " << ss.str();
}

DataFileInputStateQA1::DataFileInputStateQA1(const string& headerFileName) {
  bool              useHashBasedPayloads;
  string            weightFileName;
  vector<float>     featureWeights;
  vector<float>     featureWeightsPivots;
  vector<string>    indexFileNames;
  vector<size_t>    gizaIterQty;
  vector<string>    tranTablePrefix;
  vector<float>     minTranProb;
  vector<float>     lambdaModel1;
  vector<float>     probSelfTran;
  vector<string>    embedFileNames;
  vector<uint64_t>  featureMasks;
  vector<uint64_t>  featureMasksPivots;

  mIsQueryFile = false;
  mLineNum = 1;

  mHeadStrm.reset(new ifstream(headerFileName));

  ifstream& istrm = *mHeadStrm.get();
  CHECK_MSG(istrm, "Cannot open file '" + headerFileName + "' for reading");
  istrm.exceptions(std::ios::badbit);

  string   line;

  /*
     There are two types of input files:
     1) A query file that contains all the objects
     2) A header file that provides information about everything:
        data, weights, etc... Additional information may be read from
        separate files.
   */
  {
    string s;
    CHECK_MSG(getline(istrm, line),
          "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
    CHECK_MSG(checkPrefixGetRest(QUERY_FILE_FLAG, line, s),
          "Expected prefix '" + QUERY_FILE_FLAG + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
    ++mLineNum;

    stringstream str(s);

    if (!(str >> mIsQueryFile) || !str.eof()) {
      throw runtime_error("Error parsing of the boolean/numeric flag in the first line (use 1 or 0 to indicate value) instead of " + s);
    }
  }

  if (mIsQueryFile) {
    LOG(LIB_INFO) << "Parsing a query-type file: '" << headerFileName << "'";

    // Don't close the file, because all the data is stored in this file
    //istrm.close();
  } else {
    LOG(LIB_INFO) << "Parsing a data file";
    string s;

    CHECK_MSG(getline(istrm, line),
              "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
    CHECK_MSG(checkPrefixGetRest(USE_HASH_BASED_PAYLOADS_OPT, line, s),
              "Expected prefix '" + USE_HASH_BASED_PAYLOADS_OPT + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
    useHashBasedPayloads = lexical_cast<bool>(s);
    ++mLineNum;
    CHECK_MSG(getline(istrm, line),
          "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
    CHECK_MSG(checkPrefixGetRest(WEIGHT_FILE, line, weightFileName),
          "Expected prefix '" + WEIGHT_FILE + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
    ++mLineNum;

    readFeatureWeights(weightFileName, featureWeights);

    CHECK_MSG(getline(istrm, line),
          "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");

    string simpleIndexType;
    if (checkPrefixGetRest(SIMPLE_INDEX_TYPE, line, simpleIndexType)) {
      ToLower(simpleIndexType);
      CHECK_MSG(simpleIndexType == BM25_FEATURE_NAME || simpleIndexType == COSINE_FEATURE_NAME,
               "The simple index type is " + string(BM25_FEATURE_NAME) + " or " + string(COSINE_FEATURE_NAME));
      CHECK_MSG(getline(istrm, line),
                "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");

    }

    CHECK_MSG(line.empty(),
          "Expected to read an empty line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
    ++mLineNum;

    while (true) {
      if (!getline(istrm, line)) break;

      indexFileNames.push_back("");
      tranTablePrefix.push_back("");
      gizaIterQty.push_back(0);
      minTranProb.push_back(0);
      lambdaModel1.push_back(0);
      probSelfTran.push_back(0);
      embedFileNames.push_back("");
      featureMasks.push_back(0);

      size_t curr = indexFileNames.size() - 1;

      CHECK_MSG(checkPrefixGetRest(FIELD_INDEX_FILE, line, indexFileNames[curr]),
                "Expected prefix '" + FIELD_INDEX_FILE + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
      CHECK_MSG(!indexFileNames[curr].empty(),
                "Expected a non-empty name of the (forward) index file in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");

      ++mLineNum;
      CHECK_MSG(getline(istrm, line),
          "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
      string maskDesc, s;
      CHECK_MSG(checkPrefixGetRest(FIELD_FEATURES, line, maskDesc),
                "Expected prefix '" + FIELD_FEATURES + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
      stringstream str(maskDesc);
      while (str >> s) {
        bool f = false;
        for (size_t i = 0; i < FEATURE_TYPES.size(); ++i) {
          if (s == FEATURE_TYPES[i]) {
            f = true;
            featureMasks[curr] |= FEATURE_TYPE_MASKS[i];
            break;
          }
        }
        CHECK_MSG(f, "Unknown feature type: '" + s + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
      }
      ++mLineNum;

      if (featureMasks[curr] & (MODEL1_FLAG | SIMPLE_TRAN_FLAG)) {
        CHECK_MSG(getline(istrm, line), "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
        CHECK_MSG(checkPrefixGetRest(FIELD_TRAN_TABLE_PREFIX, line, tranTablePrefix[curr]),
                "Expected prefix '" + FIELD_TRAN_TABLE_PREFIX + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
        ++mLineNum;

        CHECK_MSG(getline(istrm, line) , "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
        CHECK_MSG(checkPrefixGetRest(GIZA_ITER_QTY, line, s),
                "Expected prefix '" + GIZA_ITER_QTY + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");

        try {
          gizaIterQty[curr] = lexical_cast<size_t>(s);
        } catch (const bad_lexical_cast& e){
          throw runtime_error("Non-integer parameter: '" + GIZA_ITER_QTY +
                              "' (value=" +s + ") in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");

        }
        ++mLineNum;

        CHECK_MSG(getline(istrm, line), "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
        CHECK_MSG(checkPrefixGetRest(PROB_SELF_TRAN, line, s),
              "Expected prefix '" + PROB_SELF_TRAN + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
        probSelfTran[curr] = lexical_cast<float>(s);
        ++mLineNum;

        if (featureMasks[curr] & (MODEL1_FLAG | SIMPLE_TRAN_FLAG)) {
          CHECK_MSG(getline(istrm, line),
                    "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
          CHECK_MSG(checkPrefixGetRest(MIN_TRAN_PROB, line, s),
                    "Expected prefix '" + MIN_TRAN_PROB + "' in line " + lexical_cast<string>(mLineNum) +
                    " of the file '" + headerFileName + "'");
          minTranProb[curr] = lexical_cast<float>(s);
          ++mLineNum;

        }
        if (featureMasks[curr] & MODEL1_FLAG) {
          CHECK_MSG(getline(istrm, line), "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
          CHECK_MSG(checkPrefixGetRest(LAMBDA_MODEL1, line, s),
                "Expected prefix '" + LAMBDA_MODEL1 + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
          lambdaModel1[curr] = lexical_cast<float>(s);
          ++mLineNum;
        }
      }

      if (featureMasks[curr] & AVG_EMBED_FLAG) {
        CHECK_MSG(getline(istrm, line), "Cannot read line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
        CHECK_MSG(checkPrefixGetRest(FIELD_EMBED_FILE, line, embedFileNames[curr]),
                  "Expected prefix '" + FIELD_EMBED_FILE + "' in line " + lexical_cast<string>(mLineNum) + " of the file '" + headerFileName + "'");
        ++mLineNum;
      }

      if (!getline(istrm, line)) break;
      CHECK_MSG(line.empty(),
          "Expected to read an empty line " + lexical_cast<string>(mLineNum) + " from '" + headerFileName + "'");
      ++mLineNum;
    }

    size_t featureQty = 0;

    unsigned fieldId=0;
    for (uint64_t mask : featureMasks) {
      if (mask & BM25_FLAG) {
        ++featureQty;
        mask ^= BM25_FLAG;
      }
      if (mask & TFIDF_LUCENE_FLAG) {
        ++featureQty;
        mask ^= TFIDF_LUCENE_FLAG;
      }
      if (mask & MODEL1_FLAG) {
        featureQty += MODEL1_FIELD_FEATURE_QTY;
        mask ^= MODEL1_FLAG;
      }
      if (mask & SIMPLE_TRAN_FLAG) {
        featureQty += SIMPLE_TRAN_FIELD_FEATURE_QTY;
        mask ^= SIMPLE_TRAN_FLAG;
      }
      if (mask & OVERALL_MATCH_FLAG) {
        featureQty += OVERAL_MATCH_FIELD_FEATURE_QTY;
        mask ^= OVERALL_MATCH_FLAG;
      }
      if (mask & LCS_FLAG) {
        featureQty += LCS_FIELD_FEATURE_QTY;
        mask ^=  LCS_FLAG;
      }
      if (mask & COSINE_FLAG) {
        ++featureQty;
        mask ^= COSINE_FLAG;
      }
      if (mask & AVG_EMBED_FLAG) {
        featureQty += AVG_EMBED_FIELD_FEATURE_QTY;
        mask ^= AVG_EMBED_FLAG;
      }

      CHECK_MSG(0 == mask,
                "Bug: it looks like we didn't take into account all the features for field "  + lexical_cast<string>(fieldId));
      ++fieldId;
    }

    CHECK_MSG(featureQty == featureWeights.size(),
              "Bug: the expected number of features " + lexical_cast<string>(featureQty) + " isn't equal to the number of feature weights " +
              lexical_cast<string>(featureWeights.size()) + " in the file '" + weightFileName + "'");

    CHECK_MSG(!weightFileName.empty(), "The name of the weight file shouldn't be empty!");
    CHECK_MSG(!indexFileNames.empty(), "No field features defined in file '" + headerFileName + "'");

    featureWeightsPivots = featureWeights;
    featureMasksPivots   = featureMasks;
    /*
     * Simple index can use only simple text features. We wont load, e.g.,
     * translation dictionaries unless they are loaded for main similarity.
     *
     * Also note that the simple index similarity will use the first field only!
     */
    if (!simpleIndexType.empty()) {
      CHECK_MSG(simpleIndexType == BM25_FEATURE_NAME || simpleIndexType == COSINE_FEATURE_NAME,
                "The simple index type is " + string(BM25_FEATURE_NAME) + " or " + string(COSINE_FEATURE_NAME));
      featureWeightsPivots.resize(1);
      featureWeightsPivots[0] = 1;
      for (auto & v: featureMasksPivots) {
        v = 0;
      }
      featureMasksPivots[0] = (simpleIndexType == BM25_FEATURE_NAME ? BM25_FLAG : COSINE_FLAG);
    }

    logFeatureMasks("regular", featureMasks);
    logFeatureMasks("pivot", featureMasksPivots);

    logFeatureWeights("regular", featureWeights);
    logFeatureWeights("pivot", featureWeightsPivots);

    mSpaceParams.reset(new SpaceParamQA1(useHashBasedPayloads,
                                        featureWeights,
                                        featureWeightsPivots,
                                        indexFileNames,
                                        tranTablePrefix,
                                        gizaIterQty,
                                        minTranProb,
                                        lambdaModel1,
                                        probSelfTran,
                                        embedFileNames,
                                        featureMasks,
                                        featureMasksPivots));
    // Close the file, if the file is only a header
    istrm.close();
  }
};

float SpaceQA1::ProxyDistance(const Object* pObjData, const Object* pObjQuery) const {
  CHECK_MSG(mSpaceParams.get(), "Bug: Expecting a non-NULL parameter pointer in the distance function");
  const SpaceParamQA1& params = *mSpaceParams.get();

  return DistanceInternal(pObjData, pObjQuery, params.mFeatureWeightsPivots, params.mFeatureMasksPivots);
}

float SpaceQA1::DistanceInternal(const Object* pObjData, const Object* pObjQuery,
                       const vector<float>& featureWeights,
                       const vector<uint64_t>& featureMasks) const {
  CHECK_MSG(mSpaceParams.get(), "Bug: Expecting a non-NULL parameter pointer in the distance function");

  const SpaceParamQA1& params = *mSpaceParams.get();
  bool  useHashBasedPayloads = params.mUseHashBasedPayloads;

  const char* pBuffPtr1 = NULL;
  const char* pBuffPtr2 = NULL;

  const FIELD_QTY_TYPE fieldQty1 = getFieldQty(pObjData, pBuffPtr1);
  const FIELD_QTY_TYPE fieldQty2 = getFieldQty(pObjQuery, pBuffPtr2);

  CHECK_MSG(fieldQty1 == params.getFieldQty(),
            "Bug: the number of fields (" + lexical_cast<string>(fieldQty1) + ") in the first, i.e., data object != " +
            " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));
  CHECK_MSG(fieldQty2 == params.getFieldQty(),
            "Bug: the number of fields (" + lexical_cast<string>(fieldQty2) + ") in the second, i.e., query object != " +
            " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));
  CHECK(fieldQty1 == featureMasks.size());

  size_t featureQty = featureWeights.size();

  DECLARE_FLEXIBLE_BUFF(float, featVals, featureQty + 10 /* buffer zone */, MAX_ON_STACK_FEATURE_QTY);
  fill(featVals, featVals + featureQty, 0);

  // Ok let's do some computations
  size_t featId = 0;

  DocEntryPtr objData, objQuery;

  /*
   * We want our features to behave in a similar way:
   * the larger is the value the more similar they are.
   * In the end, to make it a distance-like, we negate the result.
   */

  for (size_t fieldId = 0; fieldId < fieldQty1; ++fieldId) {
    uint64_t featMask = featureMasks[fieldId];

    getNextDocEntryPtr(pBuffPtr1, objData);
    getNextDocEntryPtr(pBuffPtr2, objQuery);

    float scoreBM25 = 0, scoreTFIDFLucene = 0, scoreCosine = 0, scoreOverallMatch = 0;
    float queryNorm = 1.0f / max(1.0f, (float) objQuery.mWordIdsQty);

    if (featMask & (BM25_FLAG | OVERALL_MATCH_FLAG)) {
      // BM25 and the overall match flag will be computed mostly together
      SimilarityFunctions::computeSimilBm25(true /* normalize all scores */,
                                            objQuery, objData,
                                            params.mIndxReader.getInvAvgDocLen(fieldId),
                                            scoreBM25, scoreOverallMatch);
    }
    if (featMask & COSINE_FLAG) {
      SimilarityFunctions::computeCosine(objQuery, objData, scoreCosine);
    }

    if (featMask & TFIDF_LUCENE_FLAG) {
      SimilarityFunctions::computeSimilTFIDFLucene(true /* normalize all scores */,
                                                   objQuery, objData,
                                                   scoreTFIDFLucene);
    }
    if (featMask & BM25_FLAG) {
      featVals[featId] = scoreBM25;
      ++featId;
    }
    if (featMask & TFIDF_LUCENE_FLAG) {
      featVals[featId] = scoreTFIDFLucene;
      ++featId;
    }

    if (featMask & COSINE_FLAG) {
      featVals[featId] = scoreCosine;
      ++featId;
    }

    if (featMask & OVERALL_MATCH_FLAG) {
      featVals[featId] = scoreOverallMatch;
      ++featId;
    }

    if (featMask & LCS_FLAG) {
      float val = SimilarityFunctions::computeLCS(objData.mpWordIdSeq, objData.mWordIdSeqQty,
                                                  objQuery.mpWordIdSeq, objQuery.mWordIdSeqQty);
      featVals[featId] = val * queryNorm;
      ++featId;
    }

    if (featMask & (MODEL1_FLAG | SIMPLE_TRAN_FLAG)) {
      float model1 = 0, simpleTran1 = 0, simpleTran2 = 0;

#ifdef USE_HASH_BASED_TRAN_TABLE
      #pragma message WARN("Using a hash-based translation-table (a keys is a pair of (source word ID, target word ID)!")
      computeTranFeaturesFromTranBasedHash(fieldId, params,
                                          objQuery, objData,
                                          model1, simpleTran1, simpleTran2);
#else
#ifdef PRECOMPUTE_TRAN_TABLES
      /*
       * We precompute tables only for queries and pivots, but not for the documents.
       * In such a case, mTranEntryQty < 0.
       */
      if (objQuery.mTranEntryQty < 0) {

        computeTranFeaturesFromFlippedTable(fieldId, params,
                                            objQuery, objData,
                                            model1, simpleTran1, simpleTran2);
      } else {
        if (useHashBasedPayloads) {
          CHECK_MSG(pObjQuery->GetPayload(), "Bug: expecting a non-null query payload!");

          computeTranFeaturesFromFlippedTableHashPayload(fieldId, params, *pObjQuery->GetPayload(),
                                                         objQuery, objData,
                                                         model1, simpleTran1, simpleTran2);
        } else {
          computeTranFeaturesFromFlippedTableUsingPrecompTable(fieldId, params,
                                                               objQuery, objData,
                                                               model1, simpleTran1, simpleTran2);
        }

      }
#else
      #pragma message WARN("Not using translation-table precomputation for query-like objects!")
      computeTranFeaturesFromFlippedTable(fieldId, params,
                                          objQuery, objData,
                                          model1, simpleTran1, simpleTran2);
#endif
#endif

      model1      *= queryNorm;
      simpleTran1 *= queryNorm;

      if (featMask & MODEL1_FLAG) {
        featVals[featId] = model1;
        ++featId;
      }
      if (featMask & SIMPLE_TRAN_FLAG) {
        featVals[featId] = simpleTran1; ++featId;
        // simpleTran2 isn't used!
        //featVals[featId] = simpleTran2; ++featId;
      }
    }

    if (featMask & AVG_EMBED_FLAG) {
      /*
       * We want our features to behave in a similar way:
       * the larger is the value the more similar they are.
       * In the end, to make it a distance-like, we negate the result.
       */
#ifdef USE_NON_IDF_AVG_EMBED
      featVals[featId] = 1 + ScalarProductSIMD(objData.mpRegAvgWordEmbed, objQuery.mpRegAvgWordEmbed, objData.mWordEmbedDim);
      ++featId;
#endif
      featVals[featId] = 1 + ScalarProductSIMD(objData.mpIDFWeightAvgWordEmbed, objQuery.mpIDFWeightAvgWordEmbed, objData.mWordEmbedDim);
      ++featId;
    }

    CHECK(featId <= featureQty);
  }

  CHECK(featId == featureQty);

  size_t readSize1 = pBuffPtr1 - pObjData->data();

  CHECK_MSG(readSize1 == pObjData->datalength(),
            "Bug: the read size of the first (data) object " + lexical_cast<string>(readSize1) +
            " != total size " + lexical_cast<string>(pObjData->datalength()));

  size_t readSize2 = pBuffPtr2 - pObjQuery->data();

  CHECK_MSG(readSize2 == pObjQuery->datalength(),
            "Bug: the read size of the second (query) object " + lexical_cast<string>(readSize2) +
            " != total size " + lexical_cast<string>(pObjQuery->datalength()));

  CHECK_MSG(featId == featureQty, "Bug: expected to compute " + ConvertToString(featureQty) + " features, but "
      " computed  " + ConvertToString(featId) + " features.");

  float dist = 0;

  for (size_t i = 0; i < featureQty; ++i) {
    dist += featVals[i] * featureWeights[i];
  }

#ifdef DEBUG_PRINT_ADD
  {
    stringstream str;
    for (size_t i = 0; i < featureQty; ++i) {
      if (i) str << " ";
      str << (i+1) << ":" << featVals[i];
    }

    LOG(LIB_INFO) << "Features:" << endl << str.str() << endl;
  }
#endif

  CHECK_MSG(!isnan(dist), "Bug: the computed distance value is NAN!");
  CHECK_MSG(!isinf(dist), "Bug: the computed distance value is infinite!");

  return -dist;
}

float SpaceQA1::HiddenDistance(const Object* pObjData, const Object* pObjQuery) const {
  CHECK_MSG(mSpaceParams.get(), "Bug: Expecting a non-NULL parameter pointer in the distance function");
  const SpaceParamQA1& params = *mSpaceParams.get();

  return DistanceInternal(pObjData, pObjQuery, params.mFeatureWeights, params.mFeatureMasks);
}

vector<unique_ptr<SimpleInvIndex>>*
SpaceQA1::computeModel1PivotIndex(const ObjectVector& pivots) const {
  unique_ptr<vector<unique_ptr<SimpleInvIndex>>>   holder(new vector<unique_ptr<SimpleInvIndex>>());

  const SpaceParamQA1& params = *mSpaceParams.get();

  size_t fieldQty = params.getFieldQty();
  holder->resize(fieldQty);

  for (size_t i = 0; i < fieldQty; ++i)
    (*holder)[i].reset(new SimpleInvIndex);

  for (size_t pivotId = 0; pivotId < pivots.size(); ++pivotId) {
    const char* pBuffPtr = NULL;

    const FIELD_QTY_TYPE pivotFieldQty = getFieldQty(pivots[pivotId], pBuffPtr);

    CHECK_MSG(params.getFieldQty() == pivotFieldQty,
              "Bug: the number of fields (" + lexical_cast<string>(pivotFieldQty) + ") in the pivot != " +
              " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));

    DocEntryPtr objPivot;

    for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {
      getNextDocEntryPtr(pBuffPtr, objPivot);

        
      if ((params.mFeatureMasksPivots[fieldId] & (MODEL1_FLAG | SIMPLE_TRAN_FLAG))==0) continue;

      QTY_TYPE pivotWordQty = objPivot.mWordIdsQty;

      DECLARE_FLEXIBLE_BUFF(float, aPivotWordProb, pivotWordQty + 10 /* buffer zone */, MAX_ON_STACK_ANSWER_VAL_QTY);
      fill(aPivotWordProb, aPivotWordProb + pivotWordQty, 0.0);
  
      {
        float sum = 0;
        for (size_t ip = 0; ip < pivotWordQty; ++ip) {
          sum += objPivot.mpQtys[ip];
        }
        float invSum = 1.0f/max(1.0f, sum);
        for (size_t ip = 0; ip < pivotWordQty; ++ip) {
          aPivotWordProb[ip] = invSum * float(objPivot.mpQtys[ip]);
        }
      }
 

      const GizaTranTableReaderAndRecoder& answToQuestTran = *params.mTranTables[fieldId];
      const float                          minTranProb       = params.mMinTranProb[fieldId];

      SimpleInvIndex&                      invIndex = *(*holder)[fieldId];


      for (IdTypeUnsign ia = 0; ia < objPivot.mWordIdsQty; ++ia) {
        WORD_ID_TYPE answWordId  = objPivot.mpWordIds[ia];
        QTY_TYPE     answRepQty = objPivot.mpQtys[ia];
        float        answWordMult = aPivotWordProb[ia];

        CHECK(answWordId >= 0);

        const GizaOneWordTranRecs* pTranRecs = answToQuestTran.getTranProbsOrig(answWordId);
        if (pTranRecs != nullptr) {
          for (IdTypeUnsign  it = 0 ; it < pTranRecs->mQty; ++it) {
            float oneProb = pTranRecs->mpProbs[it];
            WORD_ID_TYPE dstWordId = pTranRecs->mpDstIds[it];
            if (oneProb >= minTranProb) {
              invIndex.addEntry(dstWordId, SimpleInvEntry(pivotId, oneProb * answWordMult, answRepQty));
            }
          }
        }
      }
    }
  }

  for (size_t i = 0; i < fieldQty; ++i) {
    SimpleInvIndex&                      invIndex = *(*holder)[i];
    invIndex.sort();
  }

  return holder.release();
}


vector<unique_ptr<SimpleInvIndex>>*
SpaceQA1::computeBM25PivotIndex(const ObjectVector& pivots) const {
  unique_ptr<vector<unique_ptr<SimpleInvIndex>>>   holder(new vector<unique_ptr<SimpleInvIndex>>());

  const SpaceParamQA1& params = *mSpaceParams.get();

  size_t fieldQty = params.getFieldQty();
  holder->resize(fieldQty);

  for (size_t i = 0; i < fieldQty; ++i)
    (*holder)[i].reset(new SimpleInvIndex);

  for (size_t pivotId = 0; pivotId < pivots.size(); ++pivotId) {
    const char* pBuffPtr = NULL;

    const FIELD_QTY_TYPE pivotFieldQty = getFieldQty(pivots[pivotId], pBuffPtr);

    CHECK_MSG(params.getFieldQty() == pivotFieldQty,
              "Bug: the number of fields (" + lexical_cast<string>(pivotFieldQty) + ") in the pivot != " +
              " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));

    DocEntryPtr objPivot;

    for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {
      getNextDocEntryPtr(pBuffPtr, objPivot);

      if ((params.mFeatureMasksPivots[fieldId] & (BM25_FLAG | OVERALL_MATCH_FLAG)) == 0) continue;

      SimpleInvIndex&     invIndex = *(*holder)[fieldId];
      float               invAvgDocLen = params.mIndxReader.getInvAvgDocLen(fieldId);
      float               docLen = objPivot.mWordIdSeqQty;


      for (IdTypeUnsign ia = 0; ia < objPivot.mWordIdsQty; ++ia) {
        WORD_ID_TYPE answWordId  = objPivot.mpWordIds[ia];
        float        tf = objPivot.mpQtys[ia];

        CHECK(answWordId >= 0);

        float idf = objPivot.mpBM25IDF[ia];
        float normTF = (tf * (BM25_K1 + 1)) / (tf + BM25_K1 * (1 - BM25_B + BM25_B * docLen * invAvgDocLen));
        invIndex.addEntry(answWordId, SimpleInvEntry(pivotId, normTF*idf));
      }
    }
  }

  return holder.release();
}

vector<unique_ptr<SimpleInvIndex>>*
SpaceQA1::computeCosinePivotIndex(const ObjectVector& pivots) const {
  unique_ptr<vector<unique_ptr<SimpleInvIndex>>>   holder(new vector<unique_ptr<SimpleInvIndex>>());

  const SpaceParamQA1& params = *mSpaceParams.get();

  size_t fieldQty = params.getFieldQty();
  holder->resize(fieldQty);

  for (size_t i = 0; i < fieldQty; ++i)
    (*holder)[i].reset(new SimpleInvIndex);

  for (size_t pivotId = 0; pivotId < pivots.size(); ++pivotId) {
    const char* pBuffPtr = NULL;

    const FIELD_QTY_TYPE pivotFieldQty = getFieldQty(pivots[pivotId], pBuffPtr);

    CHECK_MSG(params.getFieldQty() == pivotFieldQty,
              "Bug: the number of fields (" + lexical_cast<string>(pivotFieldQty) + ") in the pivot != " +
              " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));

    DocEntryPtr objPivot;

    for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {
      getNextDocEntryPtr(pBuffPtr, objPivot);

      if ((params.mFeatureMasksPivots[fieldId] & (COSINE_FLAG)) == 0) continue;

      SimpleInvIndex&     invIndex = *(*holder)[fieldId];
      float               norm = 0;

      for (IdTypeUnsign ia = 0; ia < objPivot.mWordIdsQty; ++ia) {
        float idf = objPivot.mpBM25IDF[ia];
        float w = idf * objPivot.mpQtys[ia];
        norm += w * w;
      }

      float invNorm = 1.0f / std::max<float>(1e-6f, sqrt(norm));

      for (IdTypeUnsign ia = 0; ia < objPivot.mWordIdsQty; ++ia) {
        WORD_ID_TYPE answWordId  = objPivot.mpWordIds[ia];
        float        tf = objPivot.mpQtys[ia];

        CHECK(answWordId >= 0);

        float idf = objPivot.mpBM25IDF[ia];
        float w = idf * tf;

        invIndex.addEntry(answWordId, SimpleInvEntry(pivotId, w * invNorm));
      }
    }
  }

  return holder.release();
}



void
SpaceQA1::computePivotDistances(const Object* pQuery, const PivotInvIndexHolder& pivotInfo, vector<float>& vDist) const {
  // clear() + resize() will set pivotQty_ elements to zero
  size_t pivotQty = pivotInfo.pivotQty_;
  vDist.clear();
  vDist.resize(pivotQty);

  CHECK_MSG(mSpaceParams.get(), "Bug: Expecting a non-NULL parameter pointer in the distance function");


  const SpaceParamQA1& params = *mSpaceParams.get();
  size_t featureQty = params.mFeatureWeightsPivots.size();

  const char* pBuffPtr = NULL;

  const FIELD_QTY_TYPE  fieldQty = getFieldQty(pQuery, pBuffPtr);

  uint64_t allowedFeatures = (BM25_FLAG | OVERALL_MATCH_FLAG | MODEL1_FLAG | SIMPLE_TRAN_FLAG | COSINE_FLAG ) ;

  CHECK_MSG(fieldQty == params.getFieldQty(),
            "Bug: the number of fields (" + lexical_cast<string>(fieldQty) + ") in the query object != " +
            " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));

  size_t              featId = 0;

  DocEntryPtr objQuery;

  for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {
    uint64_t featMask = params.mFeatureMasksPivots[fieldId];

    CHECK_MSG((featMask & allowedFeatures) == featMask,
              "Found unsupported features in the bulk distance-computation function, supported features are: bm25, overall_match, model1, simple_tran");

    getNextDocEntryPtr(pBuffPtr, objQuery);

    float queryNormCoeff = 1.0f / max(1.0f, (float) objQuery.mWordIdsQty);

    size_t queryWordQty = objQuery.mWordIdsQty;

    if (featMask & (BM25_FLAG | OVERALL_MATCH_FLAG)) {
      float sumBM25 = 0;

      const SimpleInvIndex *pInvIndex = (*pivotInfo.bm25index_)[fieldId].get();
      CHECK_MSG(pInvIndex != nullptr, "Bug: null BM25 inverted index pointer, fieldId: " + ConvertToString(fieldId));

      // We don't care about allocation cost here,
      // because it's done once for many (potentially thousands) of pivot-query distance computations
      vector<float> accumBM25(pivotQty),
                    accumOverallMatch(pivotQty);

      for (size_t iq = 0; iq < queryWordQty; ++iq) {
        WORD_ID_TYPE queryWordId = objQuery.mpWordIds[iq];
        const QTY_TYPE queryRepQty = objQuery.mpQtys[iq];
        const IDF_TYPE queryWordIDF = objQuery.mpBM25IDF[iq];
        sumBM25 += queryWordIDF;

        const vector<SimpleInvEntry> *pEntries = pInvIndex->getDict(queryWordId);
        if (pEntries != nullptr) {
          for (SimpleInvEntry e : *pEntries) {
            accumBM25[e.id_] += queryRepQty * e.weight_;
            accumOverallMatch[e.id_] += queryRepQty;
          }
        }
      }

      float queryNormBM25Coeff = 1.0f / max(1.0f, sumBM25);

      if (featMask & BM25_FLAG) {
        CHECK_MSG(featId < featureQty,
                  "Bug: BM25 feature ID (" + ConvertToString(featId) + ") fieldId=" + ConvertToString(fieldId) +
                  " >= # of features (" + ConvertToString(featureQty));
        for (size_t pivotId = 0; pivotId < pivotQty; ++pivotId) {
          vDist[pivotId] += params.mFeatureWeightsPivots[featId] * accumBM25[pivotId] * queryNormBM25Coeff;
        }
        featId++;
      }

      if (featMask & OVERALL_MATCH_FLAG) {
        CHECK_MSG(featId < featureQty,
                  "Bug: overall match feature ID (" + ConvertToString(featId) + ") fieldId=" + ConvertToString(fieldId) +
                  " >= # of features (" + ConvertToString(featureQty));
        for (size_t pivotId = 0; pivotId < pivotQty; ++pivotId) {
          vDist[pivotId] += params.mFeatureWeightsPivots[featId] * accumOverallMatch[pivotId] * queryNormCoeff;
        }
        featId++;
      }
    }

    if (featMask & (COSINE_FLAG)) {
      float sumCosine = 0;

      const SimpleInvIndex *pInvIndex = (*pivotInfo.cosineIndex_)[fieldId].get();
      CHECK_MSG(pInvIndex != nullptr, "Bug: null BM25 inverted index pointer, fieldId: " + ConvertToString(fieldId));

      // We don't care about allocation cost here,
      // because it's done once for many (potentially thousands) of pivot-query distance computations
      vector<float> accumCosineDistance(pivotQty);

      for (size_t iq = 0; iq < queryWordQty; ++iq) {
        WORD_ID_TYPE queryWordId = objQuery.mpWordIds[iq];
        const QTY_TYPE queryRepQty = objQuery.mpQtys[iq];
        const IDF_TYPE queryWordIDF = objQuery.mpBM25IDF[iq];

        float w = queryRepQty * queryWordIDF;
        sumCosine += w * w;
        const vector<SimpleInvEntry> *pEntries = pInvIndex->getDict(queryWordId);
        if (pEntries != nullptr) {
          for (SimpleInvEntry e : *pEntries) {
            accumCosineDistance[e.id_] += w * e.weight_;
          }
        }
      }

      float queryNormCosine = 1.0f / max(1e-6f, sqrt(sumCosine));

      CHECK_MSG(featId < featureQty,
                "Bug: COSINE feature ID (" + ConvertToString(featId) + ") fieldId=" + ConvertToString(fieldId) +
                " >= # of features (" + ConvertToString(featureQty));
      for (size_t pivotId = 0; pivotId < pivotQty; ++pivotId) {
        vDist[pivotId] += params.mFeatureWeightsPivots[featId] * accumCosineDistance[pivotId] * queryNormCosine;
      }
      featId++;
    }


    if (featMask & (MODEL1_FLAG | SIMPLE_TRAN_FLAG)) {
      const GizaTranTableReaderAndRecoder &answToQuestTran = *params.mTranTables[fieldId];
      const float *pFieldProbTable = answToQuestTran.getSrcWordProbTable();
      const float lambda = params.mLambdaModel1[fieldId];

      const SimpleInvIndex *pInvIndex = (*pivotInfo.model1index_)[fieldId].get();
      CHECK_MSG(pInvIndex != nullptr, "Bug: null Model1 inverted index pointer, fieldId: " + ConvertToString(fieldId));

      vector<float> accumModel1(pivotQty * queryWordQty),
          accumSimpleTran(pivotQty);

      for (size_t iq = 0; iq < queryWordQty; ++iq) {
        WORD_ID_TYPE queryWordId = objQuery.mpWordIds[iq];
        QTY_TYPE queryRepQty = objQuery.mpQtys[iq];

        // if queryWordId <0, the entry is OOV and getDict() will return NULL
        const vector<SimpleInvEntry> *pEntries = pInvIndex->getDict(queryWordId);
        if (pEntries != nullptr) {
          for (SimpleInvEntry e : *pEntries) {
            accumModel1[iq * pivotQty + e.id_] += e.weight_;
            accumSimpleTran[e.id_] += e.answRepQty_ * queryRepQty;
          }
        }
      }

      if (featMask & MODEL1_FLAG) {
        CHECK_MSG(featId < featureQty,
                  "Bug: model1 feature ID (" + ConvertToString(featId) + ") fieldId=" + ConvertToString(fieldId) +
                  " >= # of features (" + ConvertToString(featureQty));

        for (size_t pivotId = 0; pivotId < pivotQty; ++pivotId) {
          float model1 = 0;
          for (size_t iq = 0; iq < queryWordQty; ++iq) {
            WORD_ID_TYPE queryWordId = objQuery.mpWordIds[iq];

            float collectProb = queryWordId >= 0 ?
                                max(OOV_PROB, pFieldProbTable[queryWordId]) : OOV_PROB;
            float queryTranProb = accumModel1[iq * pivotQty + pivotId];
            model1 += log((1 - lambda) * queryTranProb + lambda * collectProb);

          }
          vDist[pivotId] += params.mFeatureWeightsPivots[featId] * model1 * queryNormCoeff;

        }
        featId++;
      }


      if (featMask & SIMPLE_TRAN_FLAG) {
        for (size_t pivotId = 0; pivotId < pivotQty; ++pivotId) {

          CHECK_MSG(featId < featureQty,
                    "Bug: simple_tran feature ID (" + ConvertToString(featId) + ") fieldId=" +
                    ConvertToString(fieldId) +
                    " >= # of features (" + ConvertToString(params.mFeatureWeightsPivots.size()));
          vDist[pivotId] += params.mFeatureWeightsPivots[featId] * accumSimpleTran[pivotId] * queryNormCoeff;

        }
        featId++;
      }
    }
  }
  CHECK_MSG(featId == params.mFeatureWeightsPivots.size(),
            "Bug: expected to compute " + ConvertToString(featureQty) + " features, but "
                " computed  " + ConvertToString(featId) + " features.");
  for (size_t pivotId = 0; pivotId < pivotQty; ++pivotId) {
    float dist = vDist[pivotId];
    CHECK_MSG(!isnan(dist), "Bug: the computed distance value for pivotId = " + ConvertToString(pivotId) + " is NAN!");
    CHECK_MSG(!isinf(dist), "Bug: the computed distance value is infinite!");
    vDist[pivotId] = -dist;
  }
}

void
SpaceQA1::getObjStat(const Object* pObjData /* or pivot */,
                     const Object* pObjQuery, IdTypeUnsign fieldId,
                     IdTypeUnsign& docWordQty, IdTypeUnsign& queryWordQty, IdTypeUnsign& intersectSize,
                     IdTypeUnsign& queryTranRecsQty, IdTypeUnsign& queryTranObjIntersectSize) const {
  const SpaceParamQA1& params = *mSpaceParams.get();
#ifndef PRECOMPUTE_TRAN_TABLES
  throw runtime_error("getObjStat works only if PRECOMPUTE_TRAN_TABLES is defined!");
#endif

  const char* pBuffPtr1 = NULL;
  const char* pBuffPtr2 = NULL;

  const FIELD_QTY_TYPE fieldQty1 = getFieldQty(pObjData, pBuffPtr1);
  const FIELD_QTY_TYPE fieldQty2 = getFieldQty(pObjQuery, pBuffPtr2);

  CHECK_MSG(fieldQty1 == params.getFieldQty(),
            "Bug: the number of fields (" + lexical_cast<string>(fieldQty1) + ") in the first, i.e., data object != " +
            " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));
  CHECK_MSG(fieldQty2 == params.getFieldQty(),
            "Bug: the number of fields (" + lexical_cast<string>(fieldQty2) + ") in the second, i.e., query object != " +
            " the actual number of fields " + lexical_cast<string>(params.getFieldQty()));
  CHECK_MSG(fieldId < fieldQty1, "The requested field ID: " + ConvertToString(fieldId) + " is too large, # of fields is only: " + ConvertToString(fieldQty1));

  size_t featureQty = params.mFeatureWeights.size();

  DECLARE_FLEXIBLE_BUFF(float, featVals, featureQty + 10 /* buffer zone */, MAX_ON_STACK_FEATURE_QTY);
  fill(featVals, featVals + featureQty, 0);

  queryWordQty = 0; docWordQty = 0; intersectSize = 0;
  queryTranRecsQty = 0; queryTranObjIntersectSize = 0;

  DocEntryPtr objData, objQuery;

  /*
   * We want our features to behave in a similar way:
   * the larger is the value the more similar they are.
   * In the end, to make it a distance-like, we negate the result.
   */

  for (size_t fid = 0; fid < fieldQty1; ++fid)
  if (fieldId == fid) {
    getNextDocEntryPtr(pBuffPtr1, objData);
    getNextDocEntryPtr(pBuffPtr2, objQuery);

    docWordQty = objData.mWordIdsQty;
    queryWordQty = objQuery.mWordIdsQty;

    vector<WORD_ID_TYPE> result(max(queryWordQty, docWordQty));
    intersectSize = set_intersection(objData.mpWordIds, objData.mpWordIds + objData.mWordIdsQty,
                     objQuery.mpWordIds, objQuery.mpWordIds + objQuery.mWordIdsQty,
                     result.begin()) - result.begin();

#ifdef PRECOMPUTE_TRAN_TABLES
    CHECK_MSG(objQuery.mTranEntryQty >= 0,
              "The number of precomputed translation enteries for fieldId" + ConvertToString(fid) + " should be non-negative!");

    queryTranRecsQty  = objQuery.mTranEntryQty;
    const WORD_ID_TYPE* const pTranWordIds = objQuery.mpTranWordIds;
    //const OneTranEntryShort*  pTranEntries = objQuery.mpTranEntries;

    const WORD_ID_TYPE* pTranWordIdsCurr = pTranWordIds;
    const WORD_ID_TYPE* pTranWordIdsEnd  = pTranWordIds + queryTranRecsQty;

    const WORD_ID_TYPE* pAnswWordIds = objData.mpWordIds;
    const WORD_ID_TYPE* pAnswWordIdsEnd = objData.mpWordIds + docWordQty;

    while (pAnswWordIds < pAnswWordIdsEnd && pTranWordIdsCurr < pTranWordIdsEnd) {
      // Note that pTranWordIdsCurr < pTranWordIdsEnd, hence *pTranWordIdsCurr can always be dereferenced
      while (*pAnswWordIds < *pTranWordIdsCurr) {
        if (++pAnswWordIds >= pAnswWordIdsEnd) goto exit_lab;
      }
      while (*pAnswWordIds > *pTranWordIdsCurr) {
        if (++pTranWordIdsCurr >= pTranWordIdsEnd) goto exit_lab;
      }
      if (*pAnswWordIds == *pTranWordIdsCurr) {
        while (pTranWordIdsCurr < pTranWordIdsEnd && *pAnswWordIds == *pTranWordIdsCurr) {
          //const OneTranEntryShort &e = pTranEntries[pTranWordIdsCurr - pTranWordIds];
          //float oneTranProb = e.mTranProb;
          //if (oneTranProb >= min(params.mMinProbModel1[fid], params.mMinProbSimpleTran[fid])) {
            ++queryTranObjIntersectSize;
          //}
          ++pTranWordIdsCurr;
        }
        ++pAnswWordIds;
      }
    }
#endif
    exit_lab: ;

    break;
  }

}

/** Sample standard functions to read/write/create objects */

unique_ptr<DataFileInputState> SpaceQA1::OpenReadFileHeader(const string& inpFileName) const {
  return unique_ptr<DataFileInputState>(new DataFileInputStateQA1(inpFileName));
}

unique_ptr<DataFileOutputState> SpaceQA1::OpenWriteFileHeader(const ObjectVector& dataset,
                                                                        const string& outFileName) const {
  unique_ptr<DataFileOutputState> res(new DataFileOutputState(outFileName));

  LOG(LIB_INFO) << "Read all header/param information";

  return res;
}

unique_ptr<Object>
SpaceQA1::CreateObjFromStr(IdType id, LabelType label, const string& objStr,
                                            DataFileInputState* pInpStateBase) const {
  DataFileInputStateQA1* pInpState = NULL;
  const SpaceParamQA1 *pSpaceParams = NULL;
  if (pInpStateBase != NULL) {
     pInpState = dynamic_cast<DataFileInputStateQA1*>(pInpStateBase);
    CHECK_MSG(pInpState != NULL, "Bug: unexpected pointer type");
    pSpaceParams = pInpState->mSpaceParams.get();
  }

  if (NULL == pSpaceParams) { // Input state doesn't have params if we deal with the query file
    pSpaceParams = mSpaceParams.get();
    CHECK_MSG(pSpaceParams != NULL, "Bug: neither input state nor the space object contains space parameters");
  }

  try {
    bool                          useHashBasedPayloads = pSpaceParams->mUseHashBasedPayloads;
    vector<DocEntryParser*>       docEntries;
    AutoVectDel<DocEntryParser>   docEntriesDel(docEntries);
    vector<DocEntryHeader>        docEntryHeaders;

    stringstream strm(objStr);


    FIELD_QTY_TYPE fieldQty = pSpaceParams->getFieldQty();
    size_t totalSize = sizeof(FIELD_QTY_TYPE); // Used to keep the number of fields

#ifdef PRECOMPUTE_TRAN_TABLES
    vector<unique_ptr<vector<OneTranEntry>>>  vTranEntries(fieldQty);
#endif

    unique_ptr<HashTablePayload>  ht;
    if (useHashBasedPayloads) ht.reset(new HashTablePayload(fieldQty));

    for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {

      string line1, line2;
      CHECK_MSG(getline(strm, line1),
                "Failed to obtain the first line, field ID: " + lexical_cast<string>(fieldId));
      CHECK_MSG(getline(strm, line2),
                "Failed to obtain the second line, field ID: " + lexical_cast<string>(fieldId));
      string oneFieldStr = line1 + "\n" + line2 + "\n";
      try {
        docEntries.push_back(new DocEntryParser(pSpaceParams->mIndxReader, fieldId, oneFieldStr));
      } catch (const exception& e) {
        LOG(LIB_ERROR) << "Failed to parse document entry:" << oneFieldStr;
        throw;
      }

      const DocEntryParser& de = *docEntries.back();

    #ifdef PRECOMPUTE_TRAN_TABLES
      int32_t tranRecQty = -1;
      uint64_t featMask = pSpaceParams->mFeatureMasks[fieldId];

      /*
       * Don't precompute these values for non-data entries. They will occupy to much space!
       * Also, in most interesting cases, we need to precompute only for query-type entries:
       * external pivots and real queries.
       */
      if ((pInpState == NULL || pInpState->mIsQueryFile) &&
          !mDontPrecomputeFlag &&
          (featMask & (MODEL1_FLAG | SIMPLE_TRAN_FLAG))
          ) {
        const GizaTranTableReaderAndRecoder& answToQuestTran = *pSpaceParams->mTranTables[fieldId];
        const float minTranProb = pSpaceParams->mMinTranProb[fieldId];

        size_t estimQty = 0;
        for (size_t iq = 0; iq < de.mvWordIds.size(); ++iq) {
          WORD_ID_TYPE queryWordId = de.mvWordIds[iq];
          const GizaOneWordTranRecs *pTranRecs = answToQuestTran.getTranProbsFlipped(queryWordId);
          if (nullptr != pTranRecs) estimQty += pTranRecs->mQty;
        }

        unique_ptr<vector<OneTranEntry>>  tranEntries(new vector<OneTranEntry>());
        tranEntries->reserve(estimQty);

        for (size_t iq = 0; iq < de.mvWordIds.size(); ++iq) {
          WORD_ID_TYPE  queryWordId = de.mvWordIds[iq];
          const GizaOneWordTranRecs* pTranRecs = answToQuestTran.getTranProbsFlipped(queryWordId);
          if (NULL == pTranRecs) continue;
          const WORD_ID_TYPE* const pTranWordIds = pTranRecs->mpDstIds;
          size_t                    tranRecsQty  = pTranRecs->mQty;
          for (size_t i = 0; i < tranRecsQty; ++i) {
            float prob = pTranRecs->mpProbs[i];
            if (prob >= minTranProb)
              tranEntries->emplace_back(OneTranEntry(pTranWordIds[i], iq, prob));
          }
        }
        sort(tranEntries->begin(), tranEntries->end());
        tranRecQty = tranEntries->size();
        vTranEntries[fieldId].reset(tranEntries.release());
      }
    #endif

      DocEntryHeader        dhead(de.mvWordIds.size(), de.mvWordIdSeq.size(),
                                  pSpaceParams->mWordEmbeddings[fieldId] != NULL ? pSpaceParams->mWordEmbeddings[fieldId]->getDim() : 0
                                  #ifdef PRECOMPUTE_TRAN_TABLES
                                        ,tranRecQty
                                  #endif
                                        );
      totalSize += dhead.getTotalSize();
      docEntryHeaders.push_back(dhead);
    }

    string line;

    while (getline(strm, line)) {
      trim(line);
      if (!line.empty()) {
        throw runtime_error("More than " + lexical_cast<string>(pSpaceParams->getFieldQty()) +
                            " fields encountered in the string representation of an object");
      }
    }
    unique_ptr<Object> obj(new Object(id, label, totalSize, NULL));
    char *pBuf = obj->data();

    memcpy(pBuf, &fieldQty, sizeof fieldQty);
    pBuf += sizeof fieldQty;

    CHECK(docEntryHeaders.size() == fieldQty);
    CHECK(docEntries.size() == fieldQty);

    for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {
      memcpy(pBuf, &docEntryHeaders[fieldId], sizeof(docEntryHeaders[fieldId]));
      pBuf += sizeof(docEntryHeaders[fieldId]);
      const DocEntryParser& de = *docEntries[fieldId];
      pBuf = reinterpret_cast<char *>(
                  copy(de.mvWordIds.begin(), de.mvWordIds.end(),
                      reinterpret_cast<WORD_ID_TYPE*>(pBuf)));
      pBuf = reinterpret_cast<char *>(
                  copy(de.mvBM25IDF.begin(), de.mvBM25IDF.end(),
                      reinterpret_cast<IDF_TYPE*>(pBuf)));
      pBuf = reinterpret_cast<char *>(
                  copy(de.mvLuceneIDF.begin(), de.mvLuceneIDF.end(),
                      reinterpret_cast<IDF_TYPE*>(pBuf)));
      pBuf = reinterpret_cast<char *>(
                  copy(de.mvQtys.begin(), de.mvQtys.end(),
                      reinterpret_cast<QTY_TYPE*>(pBuf)));
      pBuf = reinterpret_cast<char *>(
                  copy(de.mvWordIdSeq.begin(), de.mvWordIdSeq.end(),
                      reinterpret_cast<WORD_ID_TYPE*>(pBuf)));
      const EmbeddingReaderAndRecoder* pEmbed = pSpaceParams->mWordEmbeddings[fieldId].get();

      if (pEmbed != NULL) {
#ifdef USE_NON_IDF_AVG_EMBED
        float* pRegAvgWordEmbed = reinterpret_cast<float*>(pBuf);
        pBuf += sizeof(float) * pEmbed->getDim();
#endif
        float* pIDFWeightAvgWordEmbed = reinterpret_cast<float*>(pBuf);
        pBuf += sizeof(float) * pEmbed->getDim();
#ifdef USE_NON_IDF_AVG_EMBED
        pEmbed->getDocAverages(de, pRegAvgWordEmbed, pIDFWeightAvgWordEmbed);
#else
        pEmbed->getDocAverages(de, pIDFWeightAvgWordEmbed);
#endif
#if 0
        for (int i = 0; i < pEmbed->getDim(); ++i) {
          if (i) cout << " ";
          cout << pIDFWeightAvgWordEmbed[i];
        }
        cout << endl;
#endif
      }

    #ifdef PRECOMPUTE_TRAN_TABLES
      int32_t tranRecQty = docEntryHeaders[fieldId].mTranRecQty;
      if (tranRecQty >=0) {
        vector<WORD_ID_TYPE>        vTranWordIds(tranRecQty);
        vector<OneTranEntryShort>   vTranEntryShort;

        vTranEntryShort.reserve(tranRecQty);

        CHECK_MSG(vTranEntries[fieldId].get() != NULL,
                  "Bug: non-zero tranRecQty, but the vTranEntries pointer is NULL for fieldId=" + ConvertToString(fieldId));
        const vector<OneTranEntry>& tranEntries = *vTranEntries[fieldId];
        for (int32_t i = 0; i < tranRecQty; ++i) {
          vTranWordIds[i] = tranEntries[i].mWordId;
          vTranEntryShort.emplace_back(OneTranEntryShort(tranEntries[i].mIq, tranEntries[i].mTranProb));
        }
        pBuf = reinterpret_cast<char *>(
            copy(vTranWordIds.begin(), vTranWordIds.end(),
                 reinterpret_cast<WORD_ID_TYPE*>(pBuf)));
        pBuf = reinterpret_cast<char *>(
            copy(vTranEntryShort.begin(), vTranEntryShort.end(),
                 reinterpret_cast<OneTranEntryShort*>(pBuf)));
        if (useHashBasedPayloads) {
          IdType ri1 = 0, ri2;
          TABLE_HASH_TYPE* h = new TABLE_HASH_TYPE(tranRecQty);
#ifdef USE_GOOGLE_SPARSE_HASH_FOR_PAYLOADS
          h->set_empty_key(-1); // IdType is a signed type
#endif
          h->max_load_factor(0.5);
          ht->vTranEntriesHash_[fieldId] = h;

          while (ri1 < tranRecQty) {
            for (ri2 = ri1 + 1; ri2 < tranRecQty && vTranWordIds[ri1] == vTranWordIds[ri2]; ++ri2);

            ht->vTranEntriesHash_[fieldId]->insert(make_pair(vTranWordIds[ri1], TranRecEntryInfo(ri1, ri2-ri1)));
            ri1 = ri2;
          }
        }
      }
    #endif
    }

    size_t copiedSize = pBuf - obj->data();

    CHECK_MSG(copiedSize == totalSize,
              "Bug: the copied size " + lexical_cast<string>(copiedSize) + " != calculated total size " + lexical_cast<string>(totalSize));
#ifdef DEBUG_CHECK
#pragma message "Using DEBUG_CHECK for additional checking, switch this off when compiling a production version!"
    {
      const char *ptr;
      unsigned qty = getFieldQty(obj.get(), ptr);

      CHECK_MSG(qty == fieldQty,
               "Bug: the number of read fields " + lexical_cast<string>(qty) + " != the actual number of fields " + lexical_cast<string>(fieldQty));

      DocEntryPtr docEntry;
      for (unsigned fieldId = 0; fieldId < fieldQty; ++fieldId) {
        const char *ptr0 = ptr;
        getNextDocEntryPtr(ptr, docEntry);
        CHECK_MSG(docEntry.mWordIdsQty == docEntryHeaders[fieldId].mWordIdsQty,
                "Bug: docEntry.mWordIdsQty (" + lexical_cast<string>(docEntry.mWordIdsQty) + ") " +
                " != docEntryHeaders[fieldId].mWordIdsQty (" + lexical_cast<string>(docEntryHeaders[fieldId].mWordIdsQty) + ") " +
                " fieldId = " + lexical_cast<string>(fieldId));
        CHECK_MSG(docEntry.mWordIdSeqQty == docEntryHeaders[fieldId].mWordIdSeqQty,
                "Bug: docEntry.mWordIdSeqQty (" + lexical_cast<string>(docEntry.mWordIdSeqQty) + ") " +
                " != docEntryHeaders[fieldId].mWordIdSeqQty (" + lexical_cast<string>(docEntryHeaders[fieldId].mWordIdSeqQty) + ") " +
                " fieldId = " + lexical_cast<string>(fieldId));
        CHECK_MSG(static_cast<size_t>(ptr - ptr0) == docEntryHeaders[fieldId].getTotalSize(),
                  "Bug: the actual number of read bytes " + lexical_cast<string>(ptr - ptr0) +
                  " for field " + lexical_cast<string>(fieldId) + " != calculated size: " + lexical_cast<string>(docEntryHeaders[fieldId].getTotalSize()));

        for (unsigned i = 0; i < docEntry.mWordIdsQty; ++i) {
          WORD_ID_TYPE    wordId = docEntry.mpWordIds[i];
          IDF_TYPE        idfBM25 = 0, idfLucene = 0;

          if (wordId >= 0) {
            const WordRec* pWordRec = pSpaceParams->mIndxReader.getWordRec(fieldId, wordId);

            CHECK_MSG(NULL != pWordRec,
                    "Bug: Cannot obtain word info for wordId=" + boost::lexical_cast<string>(wordId) +
                    " fieldId=" + boost::lexical_cast<string>(fieldId));

            idfBM25 = SimilarityFunctions::computeBM25IDF(pSpaceParams->mIndxReader.getDocQty(fieldId), pWordRec->mFreq);

            CHECK_MSG(similarity::ApproxEqual(idfBM25, docEntry.mpBM25IDF[i]) == true,
                   "Bug: recomputed BM25 IDF " + lexical_cast<string>(idfBM25) + " doesn't match the IDF in the object: "
                                               + lexical_cast<string>(docEntry.mpBM25IDF[i]));
            idfLucene = SimilarityFunctions::computeLuceneIDF(pSpaceParams->mIndxReader.getDocQty(fieldId), pWordRec->mFreq);
            CHECK_MSG(similarity::ApproxEqual(idfLucene, docEntry.mpLuceneIDF[i]) == true,
                   "Bug: recomputed Lucene IDF " + lexical_cast<string>(idfLucene) + " doesn't match the IDF in the object: "
                                               + lexical_cast<string>(docEntry.mpLuceneIDF[i]));
          }

          /*
           * Ok, now can be sure that all those IDFs are stored correctly, later on we'll check that all word IDs and qtys
           * are stored properly as well.
           */

        }
      }

      size_t readSize = ptr - obj->data();

      CHECK_MSG(readSize == obj->datalength(),
              "Bug: the read size " + lexical_cast<string>(readSize) + " != total size " + lexical_cast<string>(obj->datalength()));

    }
    {
      string objStr1 = objStr;
      // Replace '\t' with ' '
      for (char& c : objStr1) {
        if ('\t' == c) c = ' ';
      }
      string objStr2 = CreateStrFromObj(obj.get(), "");
      for (char& c : objStr2) {
        if ('\t' == c) c = ' ';
      }
      CHECK_MSG(objStr1 == objStr2,
                "Bug: The recreated object string is different from the original, recreated string:\n" + objStr2 +
                "\nOriginal string: " + objStr1);
      // Ok now I am convinced that IDs AND qtys are stored properly + I can recreate the original string representation of the Object
    }

#endif

    if (useHashBasedPayloads) 
      obj->SetPayload(ht.release());

    return obj;
  } catch (...) {
    LOG(LIB_ERROR) << "Object string that caused an exception:";
    LOG(LIB_ERROR) << endl << objStr;
    throw;
  }

}

string SpaceQA1::CreateStrFromObj(const Object* pObj, const string& externId /* ignored */) const {
  stringstream str;

  const char *ptr;
  QTY_TYPE fieldQty = getFieldQty(pObj, ptr);

  DocEntryPtr docEntry;

  for (unsigned fieldId = 0; fieldId < fieldQty; ++fieldId) {
    getNextDocEntryPtr(ptr, docEntry);

    for (QTY_TYPE i = 0; i < docEntry.mWordIdsQty; ++i) {
      if (i) str << '\t';
      str << docEntry.mpWordIds[i] << ':' << docEntry.mpQtys[i];
    }
    str << endl;
    for (QTY_TYPE i = 0; i < docEntry.mWordIdSeqQty; ++i) {
      if (i) str << " ";
      str << docEntry.mpWordIdSeq[i];
    }
    str << endl;
  }

  size_t readSize = ptr - pObj->data();

  CHECK_MSG(readSize == pObj->datalength(),
            "Bug: the read size " + lexical_cast<string>(readSize) + " != total size " + lexical_cast<string>(pObj->datalength()));

  return str.str();
}

bool SpaceQA1::ReadNextObjStr(DataFileInputState &inpStateBase, string& strObj, LabelType& label, string& externId) const {
  externId.clear();
  DataFileInputStateQA1* pInpState = dynamic_cast<DataFileInputStateQA1*>(&inpStateBase);
  CHECK_MSG(pInpState != NULL, "Bug: unexpected pointer type");
  label = -1;

  bool retFlag = false;

  if (!pInpState->mIsQueryFile) {
    retFlag = pInpState->mSpaceParams->mIndxReader.readNextObjectStr(externId, strObj);
  } else {
    strObj.clear();
    CHECK_MSG(mSpaceParams.get(), "Bug: Expecting a non-NULL parameter pointer in the function ReadNextObjStr when reading a query file");

    const SpaceParamQA1& params = *mSpaceParams.get();

    string line;

    if (!getline(*pInpState->mHeadStrm, line)) return false; // this just the end of the file

    CHECK_MSG(line.empty(),  "Expecting an empty line in the input query file, line: " + lexical_cast<string>(pInpState->mLineNum));
    pInpState->mLineNum++;

    for (size_t fieldId = 0; fieldId < params.getFieldQty(); ++fieldId) {
      if (!getline(*pInpState->mHeadStrm, line)) {
        if (!fieldId) return false; // this just the end of the file
        throw runtime_error("Expecting a non-empty line in the input query file, line: " + lexical_cast<string>(pInpState->mLineNum));
      }
      strObj.append(line); strObj.append("\n");
      pInpState->mLineNum++;
      CHECK_MSG(getline(*pInpState->mHeadStrm, line),
                "Expecting to read a line in the input query file, line: " + lexical_cast<string>(pInpState->mLineNum));
      pInpState->mLineNum++;
      strObj.append(line); strObj.append("\n");
    }

    retFlag = true;
  }

#ifdef DEBUG_PRINT
  if (retFlag) {
    cout << "#########################################" << endl;
    cout << "ID: " << externId << endl;
    cout << "#########################################" << endl;
    cout << strObj;
    cout << "#########################################" << endl;
  }
#endif

  return retFlag;
}

void SpaceQA1::WriteNextObj(const Object& obj, const string& externId, DataFileOutputState &outState) const {
  /*
   * We don't implement writing of objects, because we cannot save them in the original format.
   * Namely, the source fields are separated into fields and the current API doesn't support such
   * separation.
   */
  throw runtime_error("WriteNextObj is not implemented (and isn't planned to be implemented)!");
}
/** End of standard functions to read/write/create objects */

}  // namespace similarity
