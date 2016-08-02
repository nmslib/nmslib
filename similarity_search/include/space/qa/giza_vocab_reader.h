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

#ifndef GIZA_VOCAB_READER_H
#define GIZA_VOCAB_READER_H

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "space/qa/docentry.h"
#include "space/qa/inmemfwd_indxread.h"

#include "logging.h"

namespace similarity {

using std::unique_ptr;
using std::ifstream;

class VocabularyFilter {
public:
  virtual bool checkWord(string word) const = 0;
};

class VocabularyFilterAndRecoder : public VocabularyFilter {
public:
  virtual WORD_ID_TYPE getMaxWordId() const = 0;
  virtual bool checkWord(string word) const = 0;
  // Returns a negative value if the word isn't found
  virtual WORD_ID_TYPE getWordId(string word) const = 0;
};

class InMemForwardIndexFilterAndRecoder : public VocabularyFilterAndRecoder {
public:
  InMemForwardIndexFilterAndRecoder(const InMemFwdIndexReader& indx, size_t fieldId)  : 
                                    mIndx(indx), mFieldId(fieldId) {
    CHECK(fieldId < indx.getFieldQty());
  }
  virtual WORD_ID_TYPE getMaxWordId() const {
    return mIndx.getMaxWordId(mFieldId);
  }
  virtual bool checkWord(string word) const {
    return mIndx.getWordId(mFieldId, word) >= 0;
  }
  // Returns a negative value if the word isn't found
  virtual WORD_ID_TYPE getWordId(string word) const {
    return mIndx.getWordId(mFieldId, word);
  }
private:
  const InMemFwdIndexReader&  mIndx;
  const size_t                mFieldId;
};

struct GizaVocRec {
  string          mWord;
  WORD_ID_TYPE    mId;
  QTY_TYPE        mQty;
  
  GizaVocRec(string line);
  GizaVocRec(const string& word, WORD_ID_TYPE id, QTY_TYPE qty) : mWord(word), mId(id), mQty(qty) {}
  
};

class GizaVocabularyReader {
public:
  GizaVocabularyReader(string fileName, const VocabularyFilter* pFilter);
  ~GizaVocabularyReader() {
    delete [] mpProb;
    delete [] mpId; 
  }

  const string*  getWord(WORD_ID_TYPE wordId) const {
    Size2Size::const_iterator  it = mId2InternIdMap.find(wordId);
    if (mId2InternIdMap.end() == it) return NULL;
    return &mWords[it->second]; 
  }

  float getWordProb(const string& word) const {
    Str2WordId::const_iterator it = mWord2InternIdMap.find(word);
    if (mWord2InternIdMap.end() == it) return 0.0;
    return mpProb[it->second];
  }

  const vector<string>& getAllWords() const { return mWords; }

private:
  Str2WordId                mWord2InternIdMap;
  Size2Size                 mId2InternIdMap;
  vector<string>            mWords;
  float*                    mpProb = NULL;
  WORD_ID_TYPE*             mpId = NULL;
};


};


#endif
