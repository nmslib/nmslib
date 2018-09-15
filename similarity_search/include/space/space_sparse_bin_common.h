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
#ifndef SPACE_SPARSE_BIN_COMMON_H
#define SPACE_SPARSE_BIN_COMMON_H

#include "utils.h"
#include "space.h"
#include "read_data.h"

namespace similarity {

struct DataFileInputStateBinSparseVec : public DataFileInputStateOneFile {
  DataFileInputStateBinSparseVec(const string &inpFileName) : DataFileInputStateOneFile(inpFileName) {
    readBinaryPOD(inp_file_, qty_);
    LOG(LIB_INFO) << "Preparing to read sparse vectors from the binary file: " << inpFileName
                  << " header claims to have: " << qty_ << " vectors";
  }

  size_t qty_;
  size_t readQty_ = 0;
};

bool readNextBinSparseVect(DataFileInputStateBinSparseVec &state, string &strObj);
void parseSparseBinVector(const string &strObj, vector <SparseVectElem<float>> &v,
                          bool sortDimId = true);

}

#endif