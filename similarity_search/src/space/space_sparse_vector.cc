/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "space_sparse_vector.h"
#include "scoped_ptr.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
void SpaceSparseVector<dist_t>::ReadSparseVec(std::string line, std::vector<ElemType>& v) const
{
  v.clear();
  std::stringstream str(line);

  str.exceptions(std::ios::badbit);

  uint32_t id;
  dist_t   val;

  ReplaceSomePunct(line); 

  try {
    while (str >> id && str >> val) {
      v.push_back(ElemType(id, val));
    }
    sort(v.begin(), v.end());

    for (unsigned i = 1; i < v.size(); ++i) {
      uint32_t  prevId = v[i-1].first;
      uint32_t  id = v[i].first;

      if (id <= prevId) {
        stringstream err;
        err << "Ids are not sorted, prevId = " << prevId << " current id: " << id;
        throw std::runtime_error(err.str());
      }
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "Exception: " << e.what() << std::endl;
    LOG(FATAL) << "Failed to parse the line: '" << line << "'" << std::endl;
  }
}


template <typename dist_t>
void SpaceSparseVector<dist_t>::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<dist_t>* config,
    const char* FileName,
    const int MaxNumObjects) const {
  CHECK(config != NULL);

  dataset.clear();
  dataset.reserve(MaxNumObjects);

  std::vector<ElemType>    temp;

  std::ifstream InFile(FileName);
  InFile.exceptions(std::ios::badbit);

  try {

    std::string StrLine;

    int linenum = 0;
    int id = linenum;

    while (getline(InFile, StrLine) && (!MaxNumObjects || linenum < MaxNumObjects)) {
      if (StrLine.empty()) continue;
      ReadSparseVec(StrLine, temp);
      id = linenum;
      ++linenum;
      dataset.push_back(CreateObjFromVect(id, temp));
    }
  } catch (const std::exception &e) {
    LOG(ERROR) << "Exception: " << e.what() << std::endl;
    LOG(FATAL) << "Failed to read/parse the file: '" << FileName << "'" << std::endl;
  }
}

template <typename dist_t>
Object* SpaceSparseVector<dist_t>::CreateObjFromVect(size_t id, const std::vector<ElemType>& InpVect) const {
  return new Object(id, InpVect.size() * sizeof(ElemType), &InpVect[0]);
};

/* 
 * We don't instantiate sparse vector spaces for types other than float & double
 */
template class SpaceSparseVector<float>;
template class SpaceSparseVector<double>;
}  // namespace similarity
