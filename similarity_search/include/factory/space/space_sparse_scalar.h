/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
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

#ifndef FACTORY_SPACE_SPARSE_SCALAR_H
#define FACTORY_SPACE_SPARSE_SCALAR_H

#include <space/space_sparse_scalar.h>
#include <space/space_sparse_scalar_fast.h>

namespace similarity {

/*
 * Creating functions.
 */

/* Regular sparse spaces */

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

template <typename dist_t>
Space<dist_t>* CreateSparseNegativeScalarProduct(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseNegativeScalarProduct<dist_t>();
}

template <typename dist_t>
Space<dist_t>* CreateSparseQueryNormNegativeScalarProduct(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseQueryNormNegativeScalarProduct<dist_t>();
}

/* Fast sparse spaces */

Space<float>* CreateSparseCosineSimilarityFast(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseCosineSimilarityFast();
}


Space<float>* CreateSparseNegativeScalarProductFast(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseNegativeScalarProductFast();
}

Space<float>* CreateSparseQueryNormNegativeScalarProductFast(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseQueryNormNegativeScalarProductFast();
}

Space<float>* CreateSparseAngularDistanceFast(const AnyParams& /* ignoring params */) {
  // Cosine Similarity
  return new SpaceSparseAngularDistanceFast();
}

/*
 * End of creating functions.
 */

}

#endif
