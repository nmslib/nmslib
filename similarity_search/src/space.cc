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

using std::vector;
using std::string;

template <typename dist_t>
void Space<dist_t>::ReadDataset(ObjectVector& dataset,
                           vector<string>& vExternIds,
                           const string& inputFile,
                           const int MaxNumObjects) const {
  unique_ptr<DataFileInputState> inpState(OpenReadFileHeader(inputFile));
  string line;
  IdType label;
  string externId;
  for (int id = 0; id < MaxNumObjects || !MaxNumObjects; ++id) {
    if (!ReadNextObjStr(*inpState, line, label, externId)) break;
    dataset.push_back(CreateObjFromStr(id, label, line, inpState.get()).release());
    vExternIds.push_back(externId);
  }
  inpState->Close();
}

template <typename dist_t>
void Space<dist_t>::WriteDataset(const ObjectVector& dataset,
                           const vector<string>& vExternIds,
                           const string& outputFile,
                           const int MaxNumObjects) const {
  if (dataset.size() != vExternIds.size()) {
    PREPARE_RUNTIME_ERR(err) << "Bug, dataset.size() != vExternIds.size(): " << dataset.size() << " != " << vExternIds.size();
    THROW_RUNTIME_ERR(err);
  }
  unique_ptr<DataFileOutputState> outState(OpenWriteFileHeader(dataset, outputFile));
  for (int i = 0; i < MaxNumObjects && i < dataset.size(); ++i) {
    WriteNextObj(*dataset[i], vExternIds[i], *outState);
  }
  outState->Close();
}

template class Space<int>;
template class Space<float>;
template class Space<double>;


}