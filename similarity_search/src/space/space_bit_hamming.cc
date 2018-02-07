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
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <bitset>

#include "space/space_bit_hamming.h"
#include "permutation_utils.h"
#include "logging.h"
#include "distcomp.h"
#include "read_data.h"
#include "experimentconf.h"

namespace similarity {

using namespace std;

int SpaceBitHamming::HiddenDistance(const Object* obj1, const Object* obj2) const {
  CHECK(obj1->datalength() > 0);
  CHECK(obj1->datalength() == obj2->datalength());
  const uint32_t* x = reinterpret_cast<const uint32_t*>(obj1->data());
  const uint32_t* y = reinterpret_cast<const uint32_t*>(obj2->data());
  const size_t length = obj1->datalength() / sizeof(uint32_t)
                        - 1; // the last integer is an original number of elements 

  return BitHamming(x, y, length);
}

void SpaceBitHamming::ReadBitMaskVect(std::string line, LabelType& label, std::vector<uint32_t>& binVect) const
{
  binVect.clear();

  label = Object::extractLabel(line);

  std::stringstream str(line);

  str.exceptions(std::ios::badbit);


  ReplaceSomePunct(line); 

  vector<PivotIdType> v;

#if 0
  try {
    unsigned val;

    while (str >> val) {
      if (val != 0 && val != 1) {
        throw runtime_error("Only zeros and ones are allowed");
      }
      v.push_back(val);
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what();
    PREPARE_RUNTIME_ERR(err) << "Failed to parse the line: '" << line << "'";
    LOG(LIB_ERROR) << err.stream().str();
    THROW_RUNTIME_ERR(err);
  }
#else
  if (!ReadVecDataEfficiently(line, v)) {
    PREPARE_RUNTIME_ERR(err) << "Failed to parse the line: '" << line << "'";
    LOG(LIB_ERROR) << err.stream().str();
    THROW_RUNTIME_ERR(err);
  }
  for (auto val : v) {
    if (val != 0 && val != 1) {
      PREPARE_RUNTIME_ERR(err) << "Only zeros and ones are allowed, offending line: '" << line << "'";
      LOG(LIB_ERROR) << err.stream().str();
      THROW_RUNTIME_ERR(err);
    }
  }
#endif
  Binarize(v, 1, binVect);      // Create the binary vector
  binVect.push_back(v.size());   // Put the number of elements in the end
}

Object* SpaceBitHamming::CreateObjFromBitMaskVect(IdType id, LabelType label, const std::vector<uint32_t>& bitMaskVect) const {
  return new Object(id, label, bitMaskVect.size() * sizeof(uint32_t), &bitMaskVect[0]);
};

/** Standard functions to read/write/create objects */ 

unique_ptr<DataFileInputState> SpaceBitHamming::OpenReadFileHeader(const string& inpFileName) const {
  return unique_ptr<DataFileInputState>(new DataFileInputStateVec(inpFileName));
}

unique_ptr<DataFileOutputState> SpaceBitHamming::OpenWriteFileHeader(const ObjectVector& dataset,
                                                                     const string& outFileName) const {
  return unique_ptr<DataFileOutputState>(new DataFileOutputState(outFileName));
}

unique_ptr<Object> 
SpaceBitHamming::CreateObjFromStr(IdType id, LabelType label, const string& s,
                                            DataFileInputState* pInpStateBase) const {
  DataFileInputStateVec*  pInpState = NULL;
  if (pInpStateBase != NULL) {
    pInpState = dynamic_cast<DataFileInputStateVec*>(pInpStateBase);
    if (NULL == pInpState) {
      PREPARE_RUNTIME_ERR(err) << "Bug: unexpected pointer type";
      THROW_RUNTIME_ERR(err);
    }
  }
  vector<uint32_t>  vec;
  ReadBitMaskVect(s, label, vec);
  if (pInpState != NULL) {
    size_t elemQty = vec[vec.size() - 1];
    if (pInpState->dim_ == 0) pInpState->dim_ = elemQty;
    else if (elemQty != pInpState->dim_) {
      PREPARE_RUNTIME_ERR(err) << "The # of bit-vector elements (" << elemQty << ")" <<
                      " doesn't match the # of elements in previous lines. (" << pInpState->dim_ << " )";
      THROW_RUNTIME_ERR(err);
    }
  }
  return unique_ptr<Object>(CreateObjFromVectInternal(id, label, vec));
}

Object* SpaceBitHamming::CreateObjFromVectInternal(IdType id, LabelType label, const std::vector<uint32_t>& InpVect) const {
  return new Object(id, label, InpVect.size() * sizeof(uint32_t), &InpVect[0]);
};

bool SpaceBitHamming::ApproxEqual(const Object& obj1, const Object& obj2) const {
  const uint32_t* p1 = reinterpret_cast<const uint32_t*>(obj1.data());
  const uint32_t* p2 = reinterpret_cast<const uint32_t*>(obj2.data());
  const size_t len1 = obj1.datalength() / sizeof(uint32_t)
                        - 1; // the last integer is an original number of elements 
  const size_t len2 = obj2.datalength() / sizeof(uint32_t)
                        - 1; // the last integer is an original number of elements 
  if (len1 != len2) {
    PREPARE_RUNTIME_ERR(err) << "Bug: comparing vectors of different lengths: " << len1 << " and " << len2;
    THROW_RUNTIME_ERR(err);
  }
  for (size_t i = 0; i < len1; ++i) {
    uint32_t v1 =  ((p1[i/32] >> (i & 31)) & 1);
    uint32_t v2 =  ((p2[i/32] >> (i & 31)) & 1);
    if (v1 != v2) return false;
  }

  return true;
}


string SpaceBitHamming::CreateStrFromObj(const Object* pObj, const string& externId /* ignored */) const {
  stringstream out;
  const uint32_t* p = reinterpret_cast<const uint32_t*>(pObj->data());
  const size_t length = pObj->datalength() / sizeof(uint32_t)
                        - 1; // the last integer is an original number of elements 
  const size_t elemQty = p[length]; // last elem

  for (size_t i = 0; i < elemQty; ++i) {
    if (i) out << " ";
    out << ((p[i/32] >> (i & 31)) & 1);
  }

  return out.str();
}

bool SpaceBitHamming::ReadNextObjStr(DataFileInputState &inpStateBase, string& strObj, LabelType& label, string& externId) const {
  externId.clear();
  DataFileInputStateOneFile* pInpState = dynamic_cast<DataFileInputStateOneFile*>(&inpStateBase);
  CHECK_MSG(pInpState != NULL, "Bug: unexpected pointer type");
  if (!pInpState->inp_file_) return false;
  if (!getline(pInpState->inp_file_, strObj)) return false;
  pInpState->line_num_++;
  return true;
}


/** End of standard functions to read/write/create objects */ 

}  // namespace similarity
