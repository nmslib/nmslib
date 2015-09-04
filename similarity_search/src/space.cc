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

#include "space.h"

namespace similarity {

template <typename dist_t>
void Space<dist_t>::ReadDataset(ObjectVector& dataset,
                           const string& inputFile,
                           const int MaxNumObjects) {
  unique_ptr<DataFileInputState> inpState(space_->OpenReadFileHeader(inputFile));
  string line;
  IdType label;
  for (int id = 0; id < MaxNumObjects; ++id) {
    if (!ReadNextObjStr(*inpState, line, label)) break;
    dataset.push_back(CreateObjFromStr(id, label, line, inpState.get()).release());
  }
  inpState->Close();
}

template <typename dist_t>
void Space<dist_t>::WriteDataset(ObjectVector& dataset,
                           const string& outputFile,
                           const int MaxNumObjects) {
  unique_ptr<DataFileOutputState> outState(space_->OpenWriteFile(outputFile));
  for (int i = 0; i < MaxNumObjects && i < dataset.size(); ++i) {
    WriteNextObj(dataset[i], *outState);
  }
  outState->Close();
}

template class<Space<int>>;
template class<Space<float>>;
template class<Space<double>>;


}
