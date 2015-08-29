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

#ifndef _SPACE_DUMMY_H_
#define _SPACE_DUMMY_H_

#include <string>
#include <limits>
#include <map>
#include <stdexcept>
#include <sstream>

#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "space_vector.h"
#include "distcomp.h"

#define SPACE_DUMMY              "dummy"

namespace similarity {


/*
 * This is a dummy, i.e., zero-functionality space,
 * which can be used as a template to create fully functional
 * space.
 */

template <typename dist_t>
class SpaceDummy : public Space<dist_t> {
 public:
  // A constructor can have arbitrary params
  explicit SpaceDummy(int param1, int param2) : param1_(param1), param2_(param2) {
    LOG(LIB_INFO) << "Created " << ToString();
  }
  virtual ~SpaceDummy() {}

  /* 
   * Space name: It will be used in result files.
   * Consider, including all the parameters where you
   * print the space name:
   */
  virtual std::string ToString() const {
    stringstream  str;
    str << "DummySpace param1=" << param1_ << " param2=" << param2_;
    return str.str();
  }

  /** Standard functions to read/write/create objects */ 
  // Create an object from string representation.
  virtual unique_ptr<Object> CreateObjFromStr(IdType id, LabelType label, const string& s) const;
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


  /*
   * CreateVectFromObj and GetElemQty() are only needed, if
   * one wants to use methods with random projections.
   */
  virtual void CreateVectFromObj(const Object* obj, dist_t* pVect,
                                   size_t nElem) const {
    throw runtime_error("Cannot create vector for the space: " + ToString());
  }
  virtual size_t GetElemQty(const Object* object) const {return 0;}
 protected:
 /*
  * This function should always be protected.
  * Only children and friends of the Space class
  * should be able to access it.
  */
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const;
  virtual Space<dist_t>* HiddenClone() const {
    // The clone function should reproduce the space with *IDENTICAL* parameters
    // Another option is to use the default copy constructor (if there are no pointer fields)
    return new SpaceDummy<dist_t>(param1_, param2_);
  }
 private:
 
  /* 
   * We typically use a trailing underscore to distinguish between
   * class-member variables from non-member variables.
   */
  int param1_;
  int param2_;
};


}  // namespace similarity

#endif 
