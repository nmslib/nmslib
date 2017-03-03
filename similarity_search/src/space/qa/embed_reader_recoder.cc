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

#include <algorithm>

#include <idtype.h>
#include <space/space_vector.h>
#include "logging.h"

#include "space/qa/docentry.h"
#include "space/qa/embed_reader_recoder.h"

namespace similarity {

using namespace std;

EmbeddingReaderAndRecoder::EmbeddingReaderAndRecoder(const string& inpFileName, const InMemFwdIndexReader& index, size_t fieldId) {

  ifstream inpFile(inpFileName);

  if (!inpFile) {
    PREPARE_RUNTIME_ERR(err) << "Cannot open file: " << inpFileName << " for reading";
    THROW_RUNTIME_ERR(err);
  }

  inpFile.exceptions(ios::badbit);

  string strObj;
  size_t lineNum = 0;

  while (getline(inpFile, strObj)) {
    ++lineNum;

    ssize_t pos = -1;
    for (size_t i = 0; i < strObj.size(); ++i)
      if (isspace(strObj[i])) { pos = i ; break; }

    if (-1 == pos) {
      PREPARE_RUNTIME_ERR(err) << "No white space in line #" << lineNum << " line: '" << strObj << "'";
      THROW_RUNTIME_ERR(err);
    }
    string word = strObj.substr(0, pos);

    if (word.empty()) continue;

    strObj = strObj.substr(pos + 1);

    WORD_ID_TYPE wordId = index.getWordId(fieldId, word);
    if (wordId >= 0) {
      std::vector<float>  vec;
      LabelType label;
      VectorSpace<float>::ReadVec(strObj, label, vec);
      if (0 == mDim) {
        mDim = vec.size();
        mpZeroVect = new float[mDim];
        fill(mpZeroVect, mpZeroVect + mDim, 0);
        CHECK_MSG(mDim, "Wrong format in line " + ConvertToString(lineNum) + ", no vector elements found");
      } else {
        CHECK_MSG(mDim == vec.size(),
                  "Wrong format in line " + ConvertToString(lineNum) + ", # of vector elements " +
                      ConvertToString(vec.size()) + " is different from # of vector elements in preceeding lines ("
                      + ConvertToString(mDim) + ")");
      }
      float* pVec = new float[vec.size()];
      mhInt2Vect.insert(make_pair(wordId, pVec));
      copy(vec.begin(), vec.end(), pVec);
      normalizeL2(pVec);
    }

    if (lineNum % REPORT_INTERVAL_QTY == 0)
      LOG(LIB_INFO) << "Loaded " << mhInt2Vect.size() << " word vectors out of " << lineNum << " from '" << inpFileName << "'";
  }

  LOG(LIB_INFO) << "Finished loading " <<  mhInt2Vect.size() << " word vectors (out of " << lineNum
                << ") from " << inpFileName << ", dimensionality: " << mDim;

  inpFile.close();
}

void EmbeddingReaderAndRecoder::getDocAverages(
    const DocEntryParser& docEntry,
#ifdef USE_NON_IDF_AVG_EMBED
    float* pRegAvg,
#endif
    float* pIDFWeightAvg) const {

#ifdef USE_NON_IDF_AVG_EMBED
  fill(pRegAvg,       pRegAvg + mDim,       0);
#endif
  fill(pIDFWeightAvg, pIDFWeightAvg + mDim, 0);

  for (size_t iw = 0; iw < docEntry.mvWordIds.size(); ++iw) {
    WORD_ID_TYPE  wordId = docEntry.mvWordIds[iw];
    if (wordId >= 0) {
      const auto it = mhInt2Vect.find(wordId);
      if (it != mhInt2Vect.end()) {
        const float* pVec = it->second;

        float wordIDF = docEntry.mvBM25IDF[iw];
        float wordQty = docEntry.mvQtys[iw];

        for (size_t k = 0; k < mDim; ++k) {
#ifdef USE_NON_IDF_AVG_EMBED
          pRegAvg[k]       += wordQty * pVec[k];
#endif
          pIDFWeightAvg[k] += wordQty * pVec[k] * wordIDF;
        }
      }
    }
  }
#ifdef USE_NON_IDF_AVG_EMBED
  normalizeL2(pRegAvg);
#endif
  normalizeL2(pIDFWeightAvg);
}

}
