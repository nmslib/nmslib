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
#ifndef SIMIL_H
#define SIMIL_H

#include <cmath>
#include <algorithm>
#include <cstring>

#include "flexbuff.h"
#include "space/qa/docentry.h"

namespace similarity {

const float BM25_K1 = 1.2f, BM25_B = 0.75f;

class SimilarityFunctions {
public:
  static float computeBM25IDF(float docQty, float wordFreq) {
    // (float) is a paranoid way to ensure we use a faster version of the logarithm
    return std::log(float(
        1.0f + (docQty - wordFreq + 0.5f) / (wordFreq + 0.5f)
    ));
  }

  static float computeLuceneIDF(float docQty, float wordFreq) {
    // (float) is a paranoid way to ensure we use a faster version of the logarithm
    return std::log(float(docQty / (wordFreq + 1.0f))) + 1.0f;
  }

  /*
   * This is a classic cosine similarity.
   */
  static void computeCosine(const DocEntryPtr &query, const DocEntryPtr &doc,
                            float &scoreCosine) {
    scoreCosine = 0;

    float normQuery = 0;
    size_t queryTermQty = query.mWordIdsQty;

    for (size_t i = 0; i < queryTermQty; ++i) {
      float w = query.mpBM25IDF[i] * query.mpQtys[i];
      normQuery += w * w;
    }

    float normDoc = 0;
    size_t docTermQty = doc.mWordIdsQty;

    for (size_t i = 0; i < docTermQty; ++i) {
      float w = doc.mpBM25IDF[i] * doc.mpQtys[i];
      normDoc += w * w;
    }

    size_t iQuery = 0, iDoc = 0;

    while (iQuery < queryTermQty && iDoc < docTermQty) {
      WORD_ID_TYPE queryWordId = query.mpWordIds[iQuery];
      WORD_ID_TYPE docWordId = doc.mpWordIds[iDoc];

      if (queryWordId < docWordId) ++iQuery;
      else if (queryWordId > docWordId) ++iDoc;
      else {
        scoreCosine += query.mpBM25IDF[iQuery] * query.mpQtys[iQuery] *
                       doc.mpBM25IDF[iDoc] * doc.mpQtys[iDoc];
        ++iQuery;
        ++iDoc;
      }
    }

    scoreCosine /= sqrt(std::max<float>(1e-6, normQuery * normDoc));
  }

  /*
   * Computes an old Lucene TFIDF (minus approximation of the document length)
   */
  static void computeSimilTFIDFLucene(bool bNormByQueryLen,
                                      const DocEntryPtr &query, const DocEntryPtr &doc,
                                      float &scoreLucene) {
    scoreLucene = 0;

    size_t docTermQty = doc.mWordIdsQty;
    size_t queryTermQty = query.mWordIdsQty;

    float normLucene = 0;

    if (bNormByQueryLen) {
      for (size_t i = 0; i < queryTermQty; ++i) {
        float idfLucene = query.mpLuceneIDF[i];
        normLucene += idfLucene * idfLucene;
      }
    }

    float docLen = doc.mWordIdSeqQty;
    float lengthNorm = docLen > 0 ? ((float) (1.0 / sqrt((float) docLen))) : 0;

    size_t iQuery = 0, iDoc = 0;

    while (iQuery < queryTermQty && iDoc < docTermQty) {
      WORD_ID_TYPE queryWordId = query.mpWordIds[iQuery];
      WORD_ID_TYPE docWordId = doc.mpWordIds[iDoc];

      if (queryWordId < docWordId) ++iQuery;
      else if (queryWordId > docWordId) ++iDoc;
      else {

        {
          /* begin of Lucene-scoring-related calculations */
          float tf = sqrt(doc.mpQtys[iDoc]);

          float idf = query.mpLuceneIDF[iQuery];
          float idfSquared = idf * idf;

          scoreLucene += query.mpQtys[iQuery] * // query frequency
                         tf * idfSquared;
          /* end of Lucene-scoring-related calculations */
        }

        ++iQuery;
        ++iDoc;
      }
    }

    scoreLucene *= lengthNorm;

    if (bNormByQueryLen) {
      scoreLucene /= std::max(1e-6f, normLucene);
    }
  }

  /*
   * This function computes both the BM25 as implemented in Lucene (minus approximation of document length)
   * and the overall match (the number of shared terms).
   */
  static void computeSimilBm25(bool bNormByQueryLen,
                               const DocEntryPtr &query, const DocEntryPtr &doc,
                               const float invAvgDocLen,
                               float &scoreBM25, float &scoreOverallMatch) {
    scoreBM25 = scoreOverallMatch = 0;

    size_t docTermQty = doc.mWordIdsQty;
    size_t queryTermQty = query.mWordIdsQty;

    float normBM25 = 0;
    float docLen = doc.mWordIdSeqQty;

    size_t iQuery = 0, iDoc = 0;

    while (iQuery < queryTermQty && iDoc < docTermQty) {
      WORD_ID_TYPE queryWordId = query.mpWordIds[iQuery];
      WORD_ID_TYPE docWordId = doc.mpWordIds[iDoc];

      if (queryWordId < docWordId) {
        normBM25 += query.mpBM25IDF[iQuery];
        ++iQuery;
      }
      else if (queryWordId > docWordId) ++iDoc;
      else {
        QTY_TYPE qty = query.mpQtys[iQuery];
        scoreOverallMatch += qty;

        {
          /* begin BM25-related calculations */
          float tf = doc.mpQtys[iDoc];
          float idf = query.mpBM25IDF[iQuery];

          float normTf = (tf * (BM25_K1 + 1)) / (tf + BM25_K1 * (1 - BM25_B + BM25_B * docLen * invAvgDocLen));

          normBM25 += idf;

          scoreBM25 += idf *    // IDF
                       qty *   // query term frequency
                       normTf; // Normalized term frequency

          /* end BM25-related calculations */
        }

        ++iQuery;
        ++iDoc;
      }
    }

    if (bNormByQueryLen) {
      while (iQuery < queryTermQty) {
        normBM25 += query.mpBM25IDF[iQuery];
        ++iQuery;
      }
      scoreBM25 /= std::max(1e-6f, normBM25);
      scoreOverallMatch /= std::max<float>(1, queryTermQty);
    }
  }

  static QTY_TYPE computeLCS(const WORD_ID_TYPE *pSeq1, size_t len1,
                             const WORD_ID_TYPE *pSeq2, size_t len2) {
    if (len2 > len1) {
      std::swap(len1, len2);
      std::swap(pSeq1, pSeq2);
    }

    size_t buffQty = len2 + 1;
    DECLARE_FLEXIBLE_BUFF(QTY_TYPE, colCurr, buffQty, 1024);
    DECLARE_FLEXIBLE_BUFF(QTY_TYPE, colPrev, buffQty, 1024);

    memset(colPrev, 0, sizeof(*colPrev) * buffQty);
    memset(colCurr, 0, sizeof(*colCurr) * buffQty);

    for (size_t i1 = 0; i1 < len1; i1++) {
      for (size_t i2 = 0; i2 < len2; i2++) {
        if (pSeq1[i1] == pSeq2[i2]) {
          colCurr[i2 + 1] = colPrev[i2] + 1;
        } else {
          colCurr[i2 + 1] = std::max(colPrev[i2 + 1], colCurr[i2]);
        }
      }

      std::swap(colPrev, colCurr);
    }

    return colPrev[len2];
  }
};

}
#endif
