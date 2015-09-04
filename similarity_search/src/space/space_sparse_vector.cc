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

/** Standard functions to read/write/create objects */ 

template <typename dist_t>
unique_ptr<DataFileInputState> SpaceSparseVector<dist_t>::OpenReadFileHeader(const string& inpFileName) const {
  return unique_ptr<DataFileInputState>(new DataFileInputState(inpFileName));
}

template <typename dist_t>
unique_ptr<DataFileOutputState> SpaceSparseVector<dist_t>::OpenWriteFileHeader(const string& outputFile) const {
  return unique_ptr<DataFileOutputState>(new DataFileOutputState(outputFile));
}

template <typename dist_t>
unique_ptr<Object> 
SpaceSparseVector<dist_t>::CreateObjFromStr(IdType id, LabelType label, const string& s,
                                      DataFileInputState* pInpStateBase) const {
  if (NULL == pInpStateBase) {
    PREPARE_RUNTIME_ERR(err) << "Bug: unexpected NULL pointer type";
    THROW_RUNTIME_ERR(err);
  }
  vector<ElemType>  vec;
  ReadSparseVec(s, label, vec);
  return unique_ptr<Object>(CreateObjFromVect(id, label, vec));
}

template <typename dist_t>
string SpaceSparseVector<dist_t>::CreateStrFromObj(const Object* pObj) const {
  stringstream out;

  vector<ElemType> elems;

  vector<SparseVectElem<dist_t>> target;
  CreateVectFromObj(pObj, target);

  for (size_t i = 0; i < target.size(); ++i) {
    if (i) out << " ";
    out << target[i].id_ << " " << target[i].val_;
  }

  return out.str();
}


template <typename dist_t>
void SpaceSparseVector<dist_t>::WriteNextObj(const Object& obj, DataFileOutputState &outState) const {
  string s = CreateStrFromObj(&obj);
  outState.out_file_ << s << endl;
}

template <typename dist_t>
bool SpaceSparseVector<dist_t>::ReadNextObjStr(DataFileInputState &inpState, string& strObj, LabelType& label) const {
  if (!inpState.inp_file_) return false;
  if (!getline(inpState.inp_file_, strObj)) return false;
  inpState.line_num_++;
  return true;
}

/* 
 * We don't instantiate sparse vector spaces for types other than float & double
 */
template class SpaceSparseVector<float>;
template class SpaceSparseVector<double>;

template class SpaceSparseVectorSimpleStorage<float>;
template class SpaceSparseVectorSimpleStorage<double>;

}  // namespace similarity
