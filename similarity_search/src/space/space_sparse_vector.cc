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
#include <limits>
#include <iomanip>
#include <sstream>
#include <random>

#include "space/space_sparse_vector.h"
#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"

namespace similarity {

using namespace std;

template <typename dist_t>
void SpaceSparseVector<dist_t>::ReadSparseVec(std::string line, size_t line_num, LabelType& label, vector<ElemType>& v) const
{
  v.clear();

  label = Object::extractLabel(line);

  ReplaceSomePunct(line); 
  std::stringstream str(line);

  str.exceptions(std::ios::badbit);

  uint32_t id;
  dist_t   val;

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
        err << "Repeating ID: prevId = " << prevId << " prev val: " << v[i-1].val_ << " current id: " << id << " val = " << v[i].val_ << " (i=" << i << ")";
        throw std::runtime_error(err.str());
      }

      if (id < prevId) {
        stringstream err;
        err << "But: Ids are not sorted, prevId = " << prevId << " prev val: " << v[i-1].val_ << " current id: " << id << " val = " << v[i].val_ << " (i=" << i << ")";
        throw std::runtime_error(err.str());
      }
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what() << std::endl;
    PREPARE_RUNTIME_ERR(err) << "Failed to parse the line # " << line_num << ": '" << line << "'" << std::endl;
    LOG(LIB_ERROR) << err.stream().str() << std::endl;
    THROW_RUNTIME_ERR(err);
  }

  CHECK_MSG(!v.empty(), "Encountered an empty sparse vector: this is not allowed!");
}

/** Standard functions to read/write/create objects */ 

template <typename dist_t>
unique_ptr<DataFileInputState> SpaceSparseVector<dist_t>::OpenReadFileHeader(const string& inpFileName) const {
  return unique_ptr<DataFileInputState>(new DataFileInputStateOneFile(inpFileName));
}

template <typename dist_t>
unique_ptr<DataFileOutputState> SpaceSparseVector<dist_t>::OpenWriteFileHeader(const ObjectVector& dataset,
                                                                              const string& outputFile) const {
  return unique_ptr<DataFileOutputState>(new DataFileOutputState(outputFile));
}

template <typename dist_t>
unique_ptr<Object> 
SpaceSparseVector<dist_t>::CreateObjFromStr(IdType id, LabelType label, const string& s,
                                      DataFileInputState* pInpStateBase) const {
  size_t line_num = 0;
  if (NULL != pInpStateBase) {
    DataFileInputStateOneFile* pInpState = dynamic_cast<DataFileInputStateOneFile*>(pInpStateBase);
    CHECK_MSG(pInpState != NULL, "Bug: unexpected pointer type");
    line_num = pInpState->line_num_; 
  }

  vector<ElemType>  vec;
  ReadSparseVec(s, line_num, label, vec);
  return unique_ptr<Object>(CreateObjFromVect(id, label, vec));
}

template <typename dist_t>
bool 
SpaceSparseVector<dist_t>::ApproxEqual(const Object& obj1, const Object& obj2) const {
  vector<SparseVectElem<dist_t>> target1, target2;
  CreateVectFromObj(&obj1, target1);
  CreateVectFromObj(&obj2, target2);
  return target1 == target2;
}

template <typename dist_t>
string SpaceSparseVector<dist_t>::CreateStrFromObj(const Object* pObj, const string& externId /* ignored */) const {
  stringstream out;

  vector<ElemType> elems;

  vector<SparseVectElem<dist_t>> target;
  CreateVectFromObj(pObj, target);

  for (size_t i = 0; i < target.size(); ++i) {
    if (i) out << " ";
    // Clear all previous flags & set to the maximum precision available
    out.unsetf(ios_base::floatfield);
    out << target[i].id_ << " " << setprecision(numeric_limits<dist_t>::max_digits10) << target[i].val_;
  }

  return out.str();
}


template <typename dist_t>
bool SpaceSparseVector<dist_t>::ReadNextObjStr(DataFileInputState &inpStateBase, string& strObj, LabelType& label, string& externId) const {
  externId.clear();
  DataFileInputStateOneFile* pInpState = dynamic_cast<DataFileInputStateOneFile*>(&inpStateBase);
  CHECK_MSG(pInpState != NULL, "Bug: unexpected reference type");
  if (!pInpState->inp_file_) return false;
  if (!getline(pInpState->inp_file_, strObj)) return false;
  if (strObj.empty()) {
    PREPARE_RUNTIME_ERR(err) << "Encountered an empty line (not allowed), line # " << pInpState->line_num_; 
    THROW_RUNTIME_ERR(err);
  }
  pInpState->line_num_++;
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
