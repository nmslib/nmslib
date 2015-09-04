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
#include <sstream>
#include <string>
#include <sstream>
#include <memory>
#include <iomanip>
#include <limits>

#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"
#include "space/space_vector.h"

namespace similarity {

using namespace std;

/** Standard functions to read/write/create objects */ 

template <typename dist_t>
unique_ptr<DataFileInputState> VectorSpace<dist_t>::OpenReadFileHeader(const string& inpFileName) const {
  return unique_ptr<DataFileInputState>(new DataFileInputStateVec(inpFileName));
}

template <typename dist_t>
unique_ptr<DataFileOutputState> VectorSpace<dist_t>::OpenWriteFileHeader(const string& outFileName) const {
  return unique_ptr<DataFileOutputState>(new DataFileOutputState(outFileName));
}

template <typename dist_t>
unique_ptr<Object> 
VectorSpace<dist_t>::CreateObjFromStr(IdType id, LabelType label, const string& s,
                                            DataFileInputState* pInpStateBase) const {
  DataFileInputStateVec*  pInpState = NULL;
  if (pInpStateBase != NULL) {
    pInpState = dynamic_cast<DataFileInputStateVec*>(pInpStateBase);
    if (NULL == pInpState) {
      PREPARE_RUNTIME_ERR(err) << "Bug: unexpected pointer type";
      THROW_RUNTIME_ERR(err);
    }
  }
  vector<dist_t>  vec;
  ReadVec(s, label, vec);
  if (pInpState != NULL) {
    if (pInpState->dim_ == 0) pInpState->dim_ = vec.size();
    else if (vec.size() != pInpState->dim_) {
      stringstream lineStr;
      if (pInpStateBase != NULL) lineStr <<  " line:" << pInpStateBase->line_num_ << " ";
      PREPARE_RUNTIME_ERR(err) << "The # of vector elements (" << vec.size() << ")" << lineStr.str() << 
                      " doesn't match the # of elements in previous lines. (" << pInpState->dim_ << " )";
      THROW_RUNTIME_ERR(err);
    }
  }
  return unique_ptr<Object>(CreateObjFromVect(id, label, vec));
}

template <typename dist_t>
string VectorSpace<dist_t>::CreateStrFromObj(const Object* pObj) const {
  stringstream out;
  const dist_t* p = reinterpret_cast<const dist_t*>(pObj->data());
  const size_t length = GetElemQty(pObj);
  for (size_t i = 0; i < length; ++i) {
    if (i) out << " ";
    // Set to the maximum precision available
    out << scientific <<  setprecision(numeric_limits<float>::max_digits10) << p[i];
  }

  return out.str();
}

template <typename dist_t>
bool VectorSpace<dist_t>::ReadNextObjStr(DataFileInputState &inpState, string& strObj, LabelType& label) const {
  if (!inpState.inp_file_) return false;
  if (!getline(inpState.inp_file_, strObj)) return false;
  inpState.line_num_++;
  return true;
}

template <typename dist_t>
void VectorSpace<dist_t>::WriteNextObj(const Object& obj, DataFileOutputState &outState) const {
  string s = CreateStrFromObj(&obj);
  outState.out_file_ << s << endl;
}
/** End of standard functions to read/write/create objects */ 

template <typename dist_t>
void VectorSpace<dist_t>::ReadVec(string line, LabelType& label, vector<dist_t>& v) const
{
  v.clear();

  label = Object::extractLabel(line);

  stringstream str(line);

  str.exceptions(ios::badbit);

  dist_t val;

  ReplaceSomePunct(line); 

  try {
    while (str >> val) {
      v.push_back(val);
    }
  } catch (const exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what();
    LOG(LIB_FATAL) << "Failed to parse the line: '" << line << "'";
  }
}

template <typename dist_t>
Object* VectorSpace<dist_t>::CreateObjFromVect(IdType id, LabelType label, const vector<dist_t>& InpVect) const {
  return new Object(id, label, InpVect.size() * sizeof(dist_t), &InpVect[0]);
};

/* 
 * Note that we don't instantiate vector spaces for types other than float & double
 * The only exception is the VectorSpace<PivotIdType>
 */
template class VectorSpace<PivotIdType>;
template class VectorSpace<float>;
template class VectorSpace<double>;

}  // namespace similarity
