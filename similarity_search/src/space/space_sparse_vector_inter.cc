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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <random>

#include "space_sparse_vector_inter.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
Object* SpaceSparseVectorInter<dist_t>::CreateObjFromVect(size_t id, const std::vector<ElemType>& InpVect) const {
  char    *pData = NULL;
  size_t  dataLen = 0;

  try {
    PackSparseElements(InpVect, pData, dataLen);
    return new Object(id, dataLen, pData);
  } catch (...) {
    delete [] pData;
    throw;
  }
}

template class SpaceSparseVectorInter<float>;
template class SpaceSparseVectorInter<double>;

}  // namespace similarity
