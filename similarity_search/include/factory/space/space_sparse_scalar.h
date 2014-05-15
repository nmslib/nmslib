/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include "searchoracle.h"
#include "space/space_sparse_scalar.h"
#include "space/space_sparse_scalar_fast.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Space<dist_t>* CreateSparseCosineSimilarity(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseCosineSimilarity<dist_t>();
}


template <typename dist_t>
Space<dist_t>* CreateSparseAngularDistance(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseAngularDistance<dist_t>();
}

Space<float>* CreateSparseCosineSimilarityFast(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseCosineSimilarityFast();
}


Space<float>* CreateSparseAngularDistanceFast(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseAngularDistanceFast();
}

/*
 * End of creating functions.
 */

}

