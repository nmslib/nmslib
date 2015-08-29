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

#ifndef _SPACE_STRING_H_
#define _SPACE_STRING_H_

#include <string>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"

namespace similarity {

using std::string;

/*
 * TODO: @leo/@bileg Currently we support only char, but need to support char32_t and UTF-8 as well. 
 */
template <typename dist_t>
class StringSpace : public Space<dist_t> {
 public:
  virtual ~StringSpace() {}

  /** Standard functions to read/write/create objects */ 
  // Create an object from string representation.
  virtual unique_ptr<Object> CreateObjFromStr(IdType id, LabelType label, const string& s) const {
    return unique_ptr<Object>(CreateObjFromStr(id, label, s.data(), s.size())); 
  }
  // Create a string representation of an object.
  virtual string CreateStrFromObj(const Object* pObj) const;
  // Open a file for reading, fetch a header (if there is any) and memorize an input state
  virtual unique_ptr<DataFileInputState> ReadFileHeader(const string& inputFile);
  // Open a file for writing, write a header (if there is any) and memorize an output state
  virtual unique_ptr<DataFileInputState> WriteFileHeader(const string& outputFile);
  // Read a string representation of the next object in a file
  virtual string ReadNextObjStr(DataFileInputState &);
  // Write a string representation of the next object to a file
  virtual void WriteNextObjStr(const Object& obj, DataFileOutputState &);
  // Close the input file
  virtual void CloseInputFile(DataFileInputState& );
  // Close the output file
  virtual void CloseOutputFile(DataFileOutputState& );
  /** End of standard functions to read/write/create objects */ 

  virtual Object* CreateObjFromStr(IdType id, LabelType label, const char *pStr, size_t len) const {
    return new Object(id, label, len * sizeof(char), pStr);
  }

  virtual string ToString() const = 0;
  virtual void CreateVectFromObj(const Object* obj, dist_t* pVect,
                                 size_t nElem) const {
    throw runtime_error("Cannot create vector for the space: " + ToString());
  }
  virtual size_t GetElemQty(const Object* object) const {return 0;}
 protected:
  virtual Space<dist_t>* HiddenClone() const = 0;
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;
  void ReadStr(std::string line, LabelType& label, std::string& str) const;
};

}  // namespace similarity

#endif
