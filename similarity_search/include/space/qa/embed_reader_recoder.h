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
#ifndef EMBED_READER_RECODER_H_
#define EMBED_READER_RECODER_H_

#include "logging.h"
#include "distcomp.h"


#include <unordered_map>
#include <string>

#include "space/qa/docentry.h"
#include "space/qa/inmemfwd_indxread.h"

namespace similarity {

using std::unordered_map;
using std::string;

/**
 *
 *
 */
class EmbeddingReaderAndRecoder {
public:
  /*
   * The class reads word embeddings, if the respective word is present in the index, the embedding
   * is memorized. We keep only mapping from the word ID to the vector, but discard the word itself.
   *
   * Important note: we don't do case transformation here.
   */
  EmbeddingReaderAndRecoder(const string& fileName, const InMemFwdIndexReader& index, size_t fieldId);
  ~EmbeddingReaderAndRecoder() {
    delete [] mpZeroVect;
    for (auto e : mhInt2Vect) delete [] e.second;
  }

  size_t getDim() const { return mDim; }
  void normalizeL2(float *vec) const {
    CHECK(mpZeroVect != NULL);

    float norm = L2NormSIMD(vec, mpZeroVect, mDim);
    if (norm >= 2*numeric_limits<float>::min()) {
      // Division is much slower, so let's multiply by the inverse norm
      norm = 1.0f/norm;
      for (size_t i = 0; i < mDim; ++i) vec[i] *= norm;

#ifdef DEBUG_CHECK
      float sum = L2NormSIMD(vec, mpZeroVect, mDim);
      CHECK_MSG(ApproxEqual(sum, 1.0f, 8), "Normalization failed, the resulting norm is: " + ConvertToString(sum));
#endif
    }
    else fill(vec,vec+mDim, 0);
  }
  /*
   * Computed two averaged word embeddings: one average is simple and the other one is weighted by IDF.
   * Note that resulting embedding are L2-normalized.
   * The float buffers must point to memory regions large enough to hold mDim floats each.
   */
  void getDocAverages(
      const DocEntryParser& docEntry,
#ifdef USE_NON_IDF_AVG_EMBED
      float* pRegAvg,
#endif
      float* pIDFWeightAvg) const;
private:
  const size_t  REPORT_INTERVAL_QTY = 50000;
  size_t        mDim = 0;

  float*                                  mpZeroVect = NULL;
  unordered_map<WORD_ID_TYPE, float*>     mhInt2Vect;
};

#endif

}
