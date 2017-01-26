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

#include <memory>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdint>
#include <vector>

#include "utils.h"
#include "object.h"
#include "space/space_sparse_scalar_fast.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"
#include "inmem_inv_index.h"

namespace similarity {

using namespace std;

float 
SpaceSparseCosineSimilarityFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  float val = 1 - NormSparseScalarProductFast(obj1->data(), obj1->datalength(),
                                              obj2->data(), obj2->datalength());

  return max(val, float(0));
}

float 
SpaceSparseAngularDistanceFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return acos(NormSparseScalarProductFast(obj1->data(), obj1->datalength(),
                                          obj2->data(), obj2->datalength()));
}

float
SpaceSparseNegativeScalarProductFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return -SparseScalarProductFast(obj1->data(), obj1->datalength(),
                                  obj2->data(), obj2->datalength());

}

float
SpaceSparseQueryNormNegativeScalarProductFast::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj2->datalength() > 0);

  return -QueryNormSparseScalarProductFast(obj1->data(), obj1->datalength(),
                                           obj2->data(), obj2->datalength());

}

// Now let's describe an implementation of efficient all-pivot distance computation

class SpaceDotProdPivotIndex : public PivotIndex<float> {
public:
  // If hashTrickDim > 0, we use the hashing trick as a simple means to reduce dimensionality
  // at the expense of accuracy loss
  SpaceDotProdPivotIndex(const Space<float>& space,
                         const ObjectVector pivots,
                         bool  bNormData,
                         bool  bNormQuery,
                         size_t hashTrickDim = 0) :
    space_(space), pivots_(pivots), bNormData_(bNormData), bNormQuery_(bNormQuery) {
    createIndex();
  }
  virtual void ComputePivotDistancesIndexTime(const Object* pObj, vector<float>& vResDist) const override;
  virtual void ComputePivotDistancesQueryTime(const Query<float>* pQuery, vector<float>& vResDist) const override;
private:
  InMemInvIndex         invIndex_;
  const Space<float>&   space_;
  ObjectVector          pivots_;
  bool                  bNormData_;
  bool                  bNormQuery_;
  size_t                hashTrickDim_;

  void createIndex();
  void GenVectElems(const Object& obj, bool bNorm, vector<SparseVectElem<float>>& pivElems);
};

void SpaceDotProdPivotIndex::GenVectElems(const Object& obj, bool bNorm, vector<SparseVectElem<float>>& pivElems) {
  pivElems.clear();
  if (hashTrickDim_) {
    vector<float> tmp(hashTrickDim_);
    space_.CreateDenseVectFromObj(&obj, &tmp[0], hashTrickDim_);
    for (size_t id = 0; id < hashTrickDim_; ++id)
    if (fabs(tmp[id]) > 0) {
      pivElems.push_back(SparseVectElem<float>(id, tmp[id]));
    }
  } else {
    UnpackSparseElements(obj.data(), obj.datalength(), pivElems);
  }
  if (bNorm) {
    size_t            blockQty = 0;
    float             SqSum = 0;
    float             normCoeff = 0;
    const size_t*     pBlockQty = NULL;
    const size_t*     pBlockOff = NULL;

    const char* pBlockBegin = NULL;

    ParseSparseElementHeader(obj.data(), blockQty,
                             SqSum, normCoeff,
                             pBlockQty,
                             pBlockOff,
                             pBlockBegin);
    CHECK(obj.datalength() >= (pBlockBegin-reinterpret_cast<const char*>(obj.data())));
    for (auto & e : pivElems) {
      e.val_ *= normCoeff;
    }
  }
}

void SpaceDotProdPivotIndex::createIndex() {
  for (size_t pivId = 0; pivId < pivots_.size(); ++pivId) {
    vector<SparseVectElem<float>> pivElems;
    GenVectElems(*pivots_[pivId], bNormData_, pivElems);
    for (size_t i = 0; i < pivElems.size(); ++i) {
      auto e = pivElems[i];
      invIndex_.addEntry(e.id_, SimpleInvEntry(pivId, e.val_));
    }
  }
  // Sorting of inverted index posting lists can be done, but it is not really needed for fast all-pivot distance computation
  //invIndex_.sort(); //
}
}  // namespace similarity
