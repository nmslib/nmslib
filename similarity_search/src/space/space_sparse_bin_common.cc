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
#include "space/space_sparse_bin_common.h"

namespace  similarity {

bool readNextBinSparseVect(DataFileInputStateBinSparseVec &state, string& strObj) {
  if (state.readQty_ >= state.qty_)
    return false;

  uint32_t qty;
  readBinaryPOD(state.inp_file_, qty);
  size_t elemSize = SparseVectElem<float>::dataSize();
  vector<char> data(elemSize * qty  + sizeof(qty));
  char *p = &data[0];
  state.inp_file_.read(p + sizeof(qty), qty * elemSize);
  writeBinaryPOD(p, qty);

  strObj = string(&data[0], data.size());
  state.readQty_ ++;

  return true;
}

void parseSparseBinVector(const string &strObj,
                          vector<SparseVectElem<float>> &v,
                          bool sortDimId) {
  uint32_t qty;

  CHECK(strObj.size() >= sizeof(qty));

  const char* p = strObj.data();

  readBinaryPOD(p, qty);
  v.resize(qty);
  size_t elemSize = SparseVectElem<float>::dataSize();
  size_t expectSize = sizeof(qty) + elemSize * qty;
  CHECK_MSG(strObj.size() == expectSize,
            "Size mismatch! Binary vec size: " + ConvertToString(strObj.size()) +
            " # of vect elems: " + ConvertToString(qty) +
            " expected size: " + ConvertToString(expectSize));
  p += sizeof(qty);
  for (uint_fast32_t i = 0; i < qty; ++i) {
    readBinaryPOD(p, v[i].id_);
    p += sizeof(v[i].id_);

    readBinaryPOD(p, v[i].val_);
    p += sizeof(v[i].val_);
  }
  if (sortDimId) {
    sort(v.begin(), v.end());
  }
  for (uint_fast32_t i = 1; i < qty; ++i) {
    CHECK_MSG(v[i].id_ > v[i-1].id_, "Ids in the input file are either unsorted or have duplicates!")
  }
}


}