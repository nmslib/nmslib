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
#include <space.h>
#include <query.h>

namespace similarity {

using std::vector;
using std::string;

template <typename dist_t>
unique_ptr<DataFileInputState> Space<dist_t>::ReadDataset(ObjectVector& dataset,
                           vector<string>& vExternIds,
                           const string& inputFile,
                           const IdTypeUnsign MaxNumObjects) const {
  CHECK_MSG(MaxNumObjects >=0, "Bug: MaxNumObjects should be >= 0");
  unique_ptr<DataFileInputState> inpState(OpenReadFileHeader(inputFile));
  string line;
  LabelType label;
  string externId;
  for (size_t id = 0; id < MaxNumObjects || !MaxNumObjects; ++id) {
    if (!ReadNextObjStr(*inpState, line, label, externId)) break;
    dataset.push_back(CreateObjFromStr(id, label, line, inpState.get()).release());
    vExternIds.push_back(externId);
  }
  inpState->Close();
  return inpState;
}

template <typename dist_t>
void Space<dist_t>::WriteDataset(const ObjectVector& dataset,
                           const vector<string>& vExternIds,
                           const string& outputFile,
                           const IdTypeUnsign MaxNumObjects) const {
  CHECK_MSG(MaxNumObjects >=0, "Bug: MaxNumObjects should be >= 0");
  if (dataset.size() != vExternIds.size()) {
    PREPARE_RUNTIME_ERR(err) << "Bug, dataset.size() != vExternIds.size(): " << dataset.size() << " != " << vExternIds.size();
    THROW_RUNTIME_ERR(err);
  }
  unique_ptr<DataFileOutputState> outState(OpenWriteFileHeader(dataset, outputFile));
  for (size_t i = 0; i < MaxNumObjects && i < dataset.size(); ++i) {
    WriteNextObj(*dataset[i], vExternIds[i], *outState);
  }
  outState->Close();
}

template class Space<int>;
template class Space<float>;
template class Space<double>;

template <class dist_t>
 void DummyPivotIndex<dist_t>::ComputePivotDistancesIndexTime(const Object* pObj, vector<dist_t>& vResDist) const  {
  vResDist.resize(pivots_.size());
// Pivot is a left argument
  for (size_t i = 0; i < pivots_.size(); ++i) vResDist[i] = space_.IndexTimeDistance(pivots_[i], pObj);
}

template <class dist_t>
void DummyPivotIndex<dist_t>::ComputePivotDistancesQueryTime(const Query<dist_t>* pQuery, vector<dist_t>& vResDist) const  {
  vResDist.resize(pivots_.size());
  for (size_t i = 0; i < pivots_.size(); ++i) vResDist[i] = pQuery->DistanceObjLeft(pivots_[i]);
}

template class DummyPivotIndex<int>;
template class DummyPivotIndex<float>;
template class DummyPivotIndex<double>;

}
