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

#include "space/space_sparse_vector.h"
#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
void SpaceSparseVector<dist_t>::ReadSparseVec(std::string line, LabelType& label, vector<ElemType>& v) const
{
  v.clear();
  std::stringstream str(line);

  str.exceptions(std::ios::badbit);

  uint32_t id;
  dist_t   val;

  label = Object::extractLabel(line);

  ReplaceSomePunct(line); 

  try {
    while (str >> id && str >> val) {
      v.push_back(ElemType(id, val));
    }
    sort(v.begin(), v.end());

    for (unsigned i = 1; i < v.size(); ++i) {
      uint32_t  prevId = v[i-1].id_;
      uint32_t  id = v[i].id_;

      if (id == prevId) {
        stringstream err;
        err << "Repeating ID: prevId = " << prevId << " current id: " << id;
        throw std::runtime_error(err.str());
      }

      if (id < prevId) {
        stringstream err;
        err << "But: Ids are not sorted, prevId = " << prevId << " current id: " << id;
        throw std::runtime_error(err.str());
      }
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what() << std::endl;
    LOG(LIB_FATAL) << "Failed to parse the line: '" << line << "'" << std::endl;
  }
}


template <typename dist_t>
void SpaceSparseVector<dist_t>::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<dist_t>* /* ignoring it here */,
    const char* FileName,
    const int MaxNumObjects) const {

  dataset.clear();
  dataset.reserve(MaxNumObjects);

  vector<ElemType>    temp;

  std::ifstream InFile(FileName);

  if (!InFile) {
      LOG(LIB_FATAL) << "Cannot open file: " << FileName;
  }

  InFile.exceptions(std::ios::badbit);

  try {

    std::string StrLine;

    int linenum = 0;
    int id = linenum;
    LabelType label = -1;

    while (getline(InFile, StrLine) && (!MaxNumObjects || linenum < MaxNumObjects)) {
      if (StrLine.empty()) continue;
      ReadSparseVec(StrLine, label, temp);
      id = linenum;
      ++linenum;
      dataset.push_back(CreateObjFromVect(id, label, temp));
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what() << std::endl;
    LOG(LIB_FATAL) << "Failed to read/parse the file: '" << FileName << "'" << std::endl;
  }
}

template <typename dist_t>
Object* SpaceSparseVector<dist_t>::CreateObjFromVect(IdType id, LabelType label, const vector<ElemType>& InpVect) const {
  return new Object(id, label, InpVect.size() * sizeof(ElemType), &InpVect[0]);
};

template <typename dist_t>
void SpaceSparseVector<dist_t>::GenRandProjPivots(ObjectVector& vDst, size_t Qty, size_t MaxElem) const {
  // Static is thread-safe in C++-11
  static  std::random_device          rd;
  static  std::mt19937                engine(rd());
  static  std::normal_distribution<>  normGen(0.0f, 1.0f);

  vDst.clear();

  for (unsigned i = 0; i < Qty; ++i) {
    vector<ElemType>  temp;

    for (unsigned id = 0; id < MaxElem; ++id) {
      temp.push_back(ElemType(id, normGen(engine))); 
    }
    vDst.push_back(CreateObjFromVect(i, -1, temp));
  }
}

/* 
 * We don't instantiate sparse vector spaces for types other than float & double
 */
template class SpaceSparseVector<float>;
template class SpaceSparseVector<double>;
}  // namespace similarity
