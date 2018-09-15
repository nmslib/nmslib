/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include "space/space_sparse_scalar_bin.h"
#include "space/space_sparse_bin_common.h"

namespace similarity {

unique_ptr<DataFileInputState> SpaceSparseCosineSimilarityBin::OpenReadFileHeader(const string& inpFileName) const {
  return unique_ptr<DataFileInputState>(new DataFileInputStateBinSparseVec(inpFileName));
}

unique_ptr<Object> SpaceSparseCosineSimilarityBin::CreateObjFromStr(IdType id, LabelType label, const string& s,
                                                              DataFileInputState* pInpState) const {
  vector<SparseVectElem<float>>  vec;
  parseSparseBinVector(s, vec);
  return unique_ptr<Object>(CreateObjFromVect(id, label, vec));
}

bool SpaceSparseCosineSimilarityBin::ReadNextObjStr(DataFileInputState &state, string& strObj,
                                              LabelType&, string&) const {

  return readNextBinSparseVect(dynamic_cast<DataFileInputStateBinSparseVec&>(state), strObj);
}


}