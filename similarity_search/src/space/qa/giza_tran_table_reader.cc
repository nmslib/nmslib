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

#include <fstream>
#include <algorithm>
#include <iomanip>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "logging.h"
#include "space/qa/giza_vocab_reader.h"
#include "space/qa/giza_tran_table_reader.h"

using namespace std;

using boost::trim;
using boost::split;
using boost::lexical_cast;

namespace similarity {
  
const size_t REPORT_INTERVAL_QTY = 100000;

struct GizaTranRec {
  WORD_ID_TYPE     mSrcId;
  WORD_ID_TYPE     mDstId;
  float            mProb;

  GizaTranRec(string line) {
    vector<string> parts;

    split(parts, line, boost::is_any_of("\t "));

    CHECK_MSG(parts.size() == 3,
              "Wrong format of line '" + line + "', got " + lexical_cast<string>(parts.size()) +" fields instead of three.");
    
    try {
      mSrcId  = lexical_cast<WORD_ID_TYPE>(parts[0]);
      mDstId  = lexical_cast<WORD_ID_TYPE>(parts[1]);
      mProb   = lexical_cast<float>(parts[2]);
    } catch(const boost::bad_lexical_cast &) {
      PREPARE_RUNTIME_ERR(err) << "Wrong format of line '" << line << "', IDs aren't proper integers or the probability is not a proper float.";
      THROW_RUNTIME_ERR(err);
    }
  }

  bool operator<(const GizaTranRec& o) const {
    if (mSrcId != o.mSrcId) return mSrcId < o.mSrcId;
    return mDstId < o.mDstId;
  }
};

GizaTranTableReaderAndRecoder::GizaTranTableReaderAndRecoder(
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
) : mFilterAndRecoder(filterAndRecoder), 
    mProbSelfTran(probSelfTran),
    mMaxWordId(filterAndRecoder.getMaxWordId()),
    mTranProbOrig(mMaxWordId + 1),
    mTranProbFlipped(mMaxWordId + 1),
    mSrcWordProb(mMaxWordId + 1)
{
  mMaxWordIdShift = 0;
#ifdef USE_HASH_BASED_TRAN_TABLE 
  for (WORD_ID_TYPE tmp = mMaxWordId; tmp ; tmp >>= 1, mMaxWordIdShift++);
  LOG(LIB_INFO) << "Using a hash-based translation pair (a key is a pair of words) mMaxWordId=" << mMaxWordId << " mMaxWordIdShift=" << mMaxWordIdShift;

  mTranHashProb.reset(new TRAN_TABLE_HASH_TYPE(HASH_TRAN_TABLE_ESTIM_SIZE));
#ifdef USE_GOOGLE_SPARSE_HASH_FOR_TRAN_TABLE
#ifdef SIMPLE_KEY
  mTranHashProb->set_empty_key(numeric_limits<uint64_t>::max());
#else
  mTranHashProb->set_empty_key(make_pair(-1,-1));
#endif
#endif
  mTranHashProb->max_load_factor(0.5);
#endif

  LOG(LIB_INFO) << "Reading translation table from file: " << fileName << " probSelfTran=" << probSelfTran << " tranProbThreshold=" << tranProbThreshold;

  CHECK_MSG(mProbSelfTran >= 0 && mProbSelfTran < 1,
            "Self translation probability should be >=0 & <!");

  string line;
    
  WORD_ID_TYPE prevSrcId = -1;
  WORD_ID_TYPE recodedSrcId = -1;

  vector<TranRecNoSrcId>  tranRecs;

  size_t wordQty = 0;
  
  size_t addedQty = 0;
  size_t totalQty = 0;

  fstream fr(fileName);
  CHECK_MSG(fr, "Cannot open file '" + fileName + "' for reading");
  fr.exceptions(ios::badbit);

  for (totalQty = 0; getline(fr, line); ) { 
    ++totalQty;
    // Skip empty lines
    trim(line); if (line.empty()) continue;

    GizaTranRec rec(line);

    if (rec.mSrcId != prevSrcId) {
      if (rec.mSrcId < prevSrcId) {
        PREPARE_RUNTIME_ERR(err) << "Input file '" << fileName << "' isn't sorted, encountered ID " <<
                                  rec.mSrcId << " after " << prevSrcId;   
        THROW_RUNTIME_ERR(err);
      }
      
      if (recodedSrcId >= 0) { 
        procOneWord(recodedSrcId, tranRecs);
      }
      
      tranRecs.clear();
      ++wordQty;
    }
    
    if (totalQty % REPORT_INTERVAL_QTY == 0) {
      LOG(LIB_INFO) << "Processed " << totalQty << " lines (" << wordQty << " source word entries) from '" << fileName << "'"; 
    }

    if (rec.mSrcId != prevSrcId) {
      recodedSrcId = -1;
      if (0 == rec.mSrcId) {
        recodedSrcId = 0;
      } else {        
        const string* pWordSrc = vocSrc.getWord(rec.mSrcId);
        // wordSrc can be null, if vocSrc was also "filtered"
        if (pWordSrc != NULL) {
          WORD_ID_TYPE tmpId = mFilterAndRecoder.getWordId(*pWordSrc);
          if (tmpId >= 0) {
            recodedSrcId = tmpId;
            CHECK((size_t)recodedSrcId <= mMaxWordId);
          }
        }
      }
    }      
    prevSrcId = rec.mSrcId;

    if (recodedSrcId >=0 && 
        rec.mProb >= tranProbThreshold) {
      const string* pWordDst = vocDst.getWord(rec.mDstId);
      // wordDst can be null, if vocDst was "filtered"
      if (pWordDst != NULL) {
        WORD_ID_TYPE recodedDstId = mFilterAndRecoder.getWordId(*pWordDst);
        if (recodedDstId >=0) {
          CHECK((size_t)recodedDstId <= mMaxWordId);
          tranRecs.push_back(TranRecNoSrcId(recodedDstId, rec.mProb));
          addedQty++;
        }
      }
    }
  }

  if (recodedSrcId >= 0) { 
    procOneWord(recodedSrcId, tranRecs);
// Don't need to clear those any more
//      tranRecs.clear();      
    ++wordQty;
  }
  LOG(LIB_INFO) << "Processed " << wordQty << " source word entries from '" << fileName << "'";
  LOG(LIB_INFO) << "Loaded translation table from '" << fileName << "' " << addedQty << " records out of " << totalQty;

  // Let's create a probability table
  for (const string w: vocSrc.getAllWords()) {
    WORD_ID_TYPE tmpId = mFilterAndRecoder.getWordId(w);
    if (tmpId >= 0) {
      CHECK((size_t)tmpId <= mMaxWordId);
      float probSrc = (float)vocSrc.getWordProb(w);
      mSrcWordProb[tmpId] = probSrc;
    }
  }

  LOG(LIB_INFO) << "Created an easy-to-access source-word probability table";

  flipTranTable();
}


void GizaTranTableReaderAndRecoder::procOneWord(WORD_ID_TYPE prevSrcId, vector<TranRecNoSrcId>& tranRecs) {
    /*
     * The input entries are always sorted by the first ID,
     * but not necessarily by the second ID, e.g., in the
     * flipped file (created by resorting by the second ID and flipping
     * columns 1 and 2).
     */
  sort(tranRecs.begin(), tranRecs.end());

  WORD_ID_TYPE prevId = -1;

  for (size_t i = 0; i < tranRecs.size(); ++i) {
    const TranRecNoSrcId& rec = tranRecs[i];
    if (rec.mDstId < prevId) {
      PREPARE_RUNTIME_ERR(err) << "Bug: not sorted by the second id, prevSrcId=" << prevSrcId <<
                                  ", encountered " << rec.mDstId << " after " << prevId;
      THROW_RUNTIME_ERR(err);
    }
  }
  
  TranRecNoSrcId key(prevSrcId, 0);

  vector<TranRecNoSrcId>::iterator it = lower_bound(tranRecs.begin(), tranRecs.end(), key);
  if (tranRecs.end() == it || it->mDstId != prevSrcId) 
    tranRecs.insert(it, TranRecNoSrcId(prevSrcId, 0)); // to ensure existence of a self-translation record

    // Don't adjust in the case of spurious insertions (i.e., the source word ID is zero)
  float adjustMult = prevSrcId > 0 ? (1.0f - mProbSelfTran) : 1.0f;

  for (size_t i = 0; i < tranRecs.size(); ++i) {
    TranRecNoSrcId& rec = tranRecs[i];
    rec.mProb *= adjustMult;
    if (rec.mDstId == prevSrcId) rec.mProb += mProbSelfTran;
  }

  CHECK(prevSrcId>=0 && (size_t)prevSrcId <= mMaxWordId);
#ifdef USE_HASH_BASED_TRAN_TABLE
  for (TranRecNoSrcId e: tranRecs) {
    mTranHashProb->insert(make_pair(makeKey(prevSrcId, e.mDstId), e.mProb));
  }
#endif
  
  mTranProbOrig[prevSrcId].reset(new GizaOneWordTranRecs(tranRecs));
}

void GizaTranTableReaderAndRecoder::flipTranTable() {
  LOG(LIB_INFO) << "Flipping translation table started.";

  vector<vector<TranRecNoSrcId>> newTranProb(mMaxWordId + 1);

  for (size_t oldId = 0; oldId < mTranProbOrig.size(); ++oldId) {
    GizaOneWordTranRecs* pOldRec = mTranProbOrig[oldId].get();
    if (NULL != pOldRec) {
      for (size_t k = 0; k < pOldRec->mQty; ++k) {
        WORD_ID_TYPE newId = pOldRec->mpDstIds[k];

        vector<TranRecNoSrcId>& newRec = newTranProb[newId];
        newRec.push_back(TranRecNoSrcId(oldId, pOldRec->mpProbs[k]));
      }
    }
  }

  CHECK(mTranProbOrig.size() == newTranProb.size());

  for (size_t newId = 0; newId < mTranProbOrig.size(); ++newId) {
    vector<TranRecNoSrcId>& tranRecs = newTranProb[newId];
    if (!tranRecs.empty()) {
      sort(tranRecs.begin(), tranRecs.end());
      mTranProbFlipped[newId].reset(new GizaOneWordTranRecs(tranRecs));
    }
  }
  LOG(LIB_INFO) << "Flipping translation table ended.";
}

};
