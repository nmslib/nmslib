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
#include <memory>

#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"
#include "space/space_vector.h"

namespace similarity {

using namespace std;

template <typename dist_t>
struct DataFileInputStateVec : public DataFileInputState {
  DataFileInputState(const string& inpFileName) :
                                          mInpFile(inpFileName.c_str()),
                                          mDim(0) {

    if (!mInpFile) {
      PREPARE_RUNTIME_ERR(err) << "Cannot open file: " << inpFileName << " for reading";
      THROW_RUNTIME_ERR(err);
    }

    mInpFile.exceptions(ios::badbit);
  }
  ifstream        mInpFile;
  unsigned        mDim;
};

template <typename dist_t>
struct DataFileOutputStateVec : public DataFileOutputState {
  DataFileOutputState(const string& outFileName) :
                                          mOutFile(outFileName.c_str(), ostream::out | ostream::trunc),
                                          mDim(0) {

    if (!mOutFile) {
      PREPARE_RUNTIME_ERR(err) << "Cannot open file: " << outFileName < " for writing";
      THROW_RUNTIME_ERR(err);
    }

    mOutFile.exceptions(ios::badbit | ios::failbit);
  }
  ofstream        mOutFile;
};

template <typename dist_t>
unique_ptr<Object> 
VectorSpace<dist_t>::CreateObjFromStr(IdType id, LabelType label, const string& s,
                                            DataFileInputState* pInpStateBase) const;
  DataFileInputStateVec*  pInpState = dynamic_cast<DataFileInputStateVec>(pInpStateBase);
  if (NULL == pInpState) {
    PREPARE_RUNTIME_ERR(err) << "Unexpected pointer type";
    THROW_RUNTIME_ERR(err);
  }
  vector<dist_t>  vec;
  ReadVec(id, label, vec);
  if (pInpState != null) {
    if (pInpState->mDim == 0) pInpState->mDim = vec.size();
    else if (vec.size() != {
      PREPARE_RUNTIME_ERR(err) << "The # of vector elements (" << vec.size() << ")" <<
                      " doesn't match the # of elements in previous lines. (" << pInpState->mDim << " )";
      THROW_RUNTIME_ERR(err);
    }
  }
  return unique_ptr<Object>(CreateObjFromVect(id, label, vec));
}

template <typename dist_t>
unique_ptr<DataFileInputState<dist_t>> VectorSpace<dist_t>::ReadFileHeader(const string& inpFileName) {
  return unique_ptr<DataFileInputState<dist_t>>(new DataFileInputState<dist_t>(inpFileName));
}

template <typename dist_t> unique_ptr<DataFileOutputState<dist_t>> 
VectorSpace<dist_t>::ReadFileHeader(const string& outFileName) {
  return unique_ptr<DataFileOutputState<dist_t>>(new DataFileOutputState<dist_t>(outFileName));
}

template <typename dist_t>
void VectorSpace<dist_t>::CloseInputFile(DataFileInputState<dist_t>& inputStateBase) {
  DataFileInputStateVec*  pInpState = dynamic_cast<DataFileInputStateVec>(&inpStateBase);
  if (NULL == pInpState) {
    PREPARE_RUNTIME_ERR(err) << "Unexpected pointer type";
    THROW_RUNTIME_ERR(err);
  }
  pInpState->mInpFile.close();
}

template <typename dist_t>
void VectorSpace<dist_t>::CloseOutputFile(DataFileOutputState<dist_t>& outStateBase) {
  DataFileOutputStateVec*  pOutState = dynamic_cast<DataFileOutputStateVec>(&outStateBase);
  if (NULL == pOutState) {
    PREPARE_RUNTIME_ERR(err) << "Unexpected pointer type";
    THROW_RUNTIME_ERR(err);
  }
  pOutState->mOutFile.close();
}

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
