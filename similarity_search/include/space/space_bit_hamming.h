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
#ifndef _SPACE_BIT_HAMMING_H_
#define _SPACE_BIT_HAMMING_H_

#include <string>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "distcomp.h"

#define SPACE_BIT_HAMMING "bit_hamming"

namespace similarity {

class SpaceBitHamming : public Space<int> {
 public:
  explicit SpaceBitHamming() {}
  virtual ~SpaceBitHamming() {}

  /** Standard functions to read/write/create objects */ 
  // Create an object from string representation.
  virtual unique_ptr<Object> CreateObjFromStr(IdType id, LabelType label, const string& s,
                                              DataFileInputState* pInpState) const;
  // Create a string representation of an object.
  virtual string CreateStrFromObj(const Object* pObj, const string& externId /* ignored */) const;
  // Open a file for reading, fetch a header (if there is any) and memorize an input state
  virtual unique_ptr<DataFileInputState> OpenReadFileHeader(const string& inputFile) const;
  // Open a file for writing, write a header (if there is any) and memorize an output state
  virtual unique_ptr<DataFileOutputState> OpenWriteFileHeader(const ObjectVector& dataset,
                                                              const string& outputFile) const;
  /*
   * Read a string representation of the next object in a file as well
   * as its label. Return false, on EOF.
   */
  virtual bool ReadNextObjStr(DataFileInputState &, string& strObj, LabelType& label, string& externId) const;
  /** End of standard functions to read/write/create objects */

  /*
   * Used only for testing/debugging: compares objects approximately. Floating point numbers
   * should be nearly equal. Integers and strings should coincide exactly.
   */
  virtual bool ApproxEqual(const Object& obj1, const Object& obj2) const;

  virtual std::string StrDesc() const { return "Hamming (bit-storage) space"; }
  virtual void CreateDenseVectFromObj(const Object* obj, int* pVect,
                                      size_t nElem) const {
    throw runtime_error("Cannot create a dense vector for the space: " + StrDesc());
  }
  virtual size_t GetElemQty(const Object* object) const {return 0;}
  virtual Object* CreateObjFromVect(IdType id, LabelType label, std::vector<uint32_t>& InpVect) const {
    InpVect.push_back(InpVect.size());
    return CreateObjFromVectInternal(id, label, InpVect);
  }
 protected:
  virtual Object* CreateObjFromVectInternal(IdType id, LabelType label, const std::vector<uint32_t>& InpVect) const;
  Object* CreateObjFromBitMaskVect(IdType id, LabelType label, const std::vector<uint32_t>& bitMaskVect) const;
  virtual int HiddenDistance(const Object* obj1, const Object* obj2) const;
  void ReadBitMaskVect(std::string line, LabelType& label, std::vector<uint32_t>& v) const;

  DISABLE_COPY_AND_ASSIGN(SpaceBitHamming);
};

}  // namespace similarity

#endif
