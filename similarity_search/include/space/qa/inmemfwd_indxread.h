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
#ifndef INMEMFWD_INDEX_READER_H
#define INMEMFWD_INDEX_READER_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include "logging.h"
#include "simil.h"

namespace similarity {

using std::vector;
using std::ifstream;
using std::string;
using std::unique_ptr;
using std::unordered_map;

struct WordRec {
  WordRec() : mFreq(0), mBM25IDF(0), mLuceneIDF(0) {}
  WordRec(size_t docQty, unsigned freq) 
  : mFreq(freq), 
  mBM25IDF(SimilarityFunctions::computeBM25IDF(docQty, freq)),
  mLuceneIDF(SimilarityFunctions::computeLuceneIDF(docQty, freq))
  {
  }
  const unsigned  mFreq; // The number of documents containing this word
  const float     mBM25IDF;
  const float     mLuceneIDF;
};


typedef unordered_map<int, WordRec> Int2WordRec;
typedef unordered_map<string, int>  Str2Int;

/*
 * A helper class that reads forward index previously created
 * by another (Java) pipeline. The forward index is split into fields.
 * For each field, this class reads the field-specific dictionary (but doesn't keep the words, only IDs)
 * as well as the field-specific document statistics. However, unlike the code
 * in the other (Java) pipeline, it merges document entries from several fields.
 */
class InMemFwdIndexReader {
public:
  InMemFwdIndexReader(const vector<string>& fileNames) : mvFileNames(fileNames) {
    resize(fileNames.size());
  
    for (size_t i = 0; i < fileNames.size(); ++i) {
      mvInpFiles[i] = unique_ptr<ifstream>(new ifstream(fileNames[i]));
      ifstream& strm = *mvInpFiles[i];
      if (!strm) {
        PREPARE_RUNTIME_ERR(err) << "Cannot open: '" << fileNames[i] << "' for reading";
        THROW_RUNTIME_ERR(err);
      }
      strm.exceptions(std::ios::badbit);
    }
  }

  // Read vocabularies and document statistics
  void readVocsAndDocStat();
  // Read a string representation of the next object, return false if the EOF is reached.
  bool readNextObjectStr(string& extId, string& strObj);

  void close() {
    for (size_t i = 0; i < mvInpFiles.size(); ++i) mvInpFiles[i]->close();
  }

  const WordRec* getWordRec(size_t fieldId, WORD_ID_TYPE wordId) const {
    if (fieldId > mvWordId2WordRec.size()) {
      stringstream str;
      str << "Bug: the field ID: " << fieldId << " is too large";
      LOG(LIB_ERROR) << str.str();
      throw runtime_error(str.str());
    }
    const Int2WordRec& dict = mvWordId2WordRec[fieldId];
    Int2WordRec::const_iterator it = dict.find(wordId);
    if (dict.end() != it) {
      return &it->second;
    }
    return NULL;
  }

  WORD_ID_TYPE getWordId(size_t fieldId, const string& word) const {
    if (fieldId > mvStr2WordId.size()) {
      stringstream str;
      str << "Bug: the field ID: " << fieldId << " is too large";
      LOG(LIB_ERROR) << str.str();
      throw runtime_error(str.str());
    }
    Str2Int::const_iterator it = mvStr2WordId[fieldId].find(word);
    return it == mvStr2WordId[fieldId].end() ? -1 : it->second;
  }

  size_t getFieldQty() const { return mvMaxWordId.size(); }
  size_t getMaxWordId(size_t fieldIndex) const { return mvMaxWordId[fieldIndex]; }
  size_t getDocQty(size_t fieldIndex) const { return mvDocQty[fieldIndex]; }
  float  getAvgDocLen(size_t fieldIndex) const { return mvAvgDocLen[fieldIndex]; }
  float  getInvAvgDocLen(size_t fieldIndex) const { return mvInvAvgDocLen[fieldIndex]; }

private:
  vector<string>                  mvFileNames;
  vector<unique_ptr<ifstream>>    mvInpFiles;
  vector<Int2WordRec>             mvWordId2WordRec;
  vector<Str2Int>                 mvStr2WordId;
  vector<uint32_t>                mvDocQty;
  vector<uint64_t>                mvTotalWordQty;
  vector<float>                   mvAvgDocLen;
  vector<float>                   mvInvAvgDocLen;
  vector<size_t>                  mvLineNum;
  vector<size_t>                  mvMaxWordId;

  void resize(size_t qty) {
    mvInpFiles.resize(qty);
    mvWordId2WordRec.resize(qty);
    mvStr2WordId.resize(qty);
    mvDocQty.resize(qty);
    mvTotalWordQty.resize(qty);
    mvAvgDocLen.resize(qty);
    mvInvAvgDocLen.resize(qty);
    mvLineNum.resize(qty);
    mvMaxWordId.resize(qty);
  }
};

}  // namespace similarity

#endif
