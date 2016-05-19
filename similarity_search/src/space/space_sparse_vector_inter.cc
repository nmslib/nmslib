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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <random>

#include "space/space_sparse_vector_inter.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
void
SpaceSparseVectorInter<dist_t>::CreateDenseVectFromObj(const Object* obj, 
                                                       dist_t* pVect,
                                                       size_t nElem) const {
  static std::hash<size_t>   indexHash;
  fill(pVect, pVect + nElem, static_cast<dist_t>(0));

  vector<SparseVectElem<dist_t>> target;
  UnpackSparseElements(obj->data(), obj->datalength(), target);

  for (SparseVectElem<dist_t> e: target) {
    size_t idx = indexHash(e.id_) % nElem;
    pVect[idx] += e.val_;
  }
}

template <typename dist_t>
void
SpaceSparseVectorInter<dist_t>::CreateVectFromObj(const Object* obj, vector<ElemType>& v) const {
  UnpackSparseElements(obj->data(), obj->datalength(), v);
}

template <typename dist_t>
Object* SpaceSparseVectorInter<dist_t>::CreateObjFromVect(IdType id, LabelType label, const std::vector<ElemType>& InpVect) const {
  char    *pData = NULL;
  size_t  dataLen = 0;

  try {
    PackSparseElements(InpVect, pData, dataLen);
    return new Object(id, label, dataLen, pData);
  } catch (...) {
    delete [] pData;
    throw;
  }
}

template class SpaceSparseVectorInter<float>;
template class SpaceSparseVectorInter<double>;

}  // namespace similarity
