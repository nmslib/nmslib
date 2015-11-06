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
#ifndef _DUMMY_METHOD_H_
#define _DUMMY_METHOD_H_

#include <string>
#include <sstream>

#include "index.h"

#define METH_DUMMY             "dummy"

namespace similarity {

using std::string;

/*
 * This is a dummy, i.e., zero-functionality search method,
 * which can be used as a template to create fully functional
 * search method.
 */

template <typename dist_t>
class DummyMethod : public Index<dist_t> {
 public:
  /*
   * The constructor here space and data-objects' references,
   * which are guaranteed to be be valid during testing.
   * So, we can memorize them safely.
   */
  DummyMethod(Space<dist_t>& space, 
              const ObjectVector& data) : data_(data), space_(space) {}

  /*
   * This function is supposed to create a search index (or call a 
   * function to create it)!
   */
  void CreateIndex(const AnyParams& IndexParams) {
    AnyParamManager  pmgr(IndexParams);
    pmgr.GetParamOptional("doSeqSearch",  
                          bDoSeqSearch_, 
      // One should always specify the default value of an optional parameter!
                          false
                          );
    // Check if a user specified extra parameters, which can be also misspelled variants of existing ones
    pmgr.CheckUnused();
  }

  /*
   * One can implement functions for index serialization and reading.
   * However, this is not required.
   */
  // SaveIndex is not necessarily implemented
  virtual void SaveIndex(const string& location) {
    throw runtime_error("SaveIndex is not implemented for method: " + ToString());
  }
  // LoadIndex is not necessarily implemented
  virtual void LoadIndex(const string& location) {
    throw runtime_error("LoadIndex is not implemented for method: " + ToString());
  }

  void SetQueryTimeParams(const AnyParams& QueryTimeParams);

  ~DummyMethod(){};

  /* 
   * Just the name of the method, consider printing crucial parameter values
   */
  const std::string ToString() const { 
    stringstream str;
    str << "Dummy method: " << (bDoSeqSearch_ ? " does seq. search " : " does nothing (really dummy)"); 
    return str.str();
  }

  /* 
   *  One needs to implement two search functions.
   */
  void Search(RangeQuery<dist_t>* query, IdType) const;
  void Search(KNNQuery<dist_t>* query, IdType) const;

 private:
  const ObjectVector&     data_;
  Space<dist_t>&          space_;
  bool                    bDoSeqSearch_;
  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(DummyMethod);
};

}   // namespace similarity

#endif     // _DUMMY_METHOD_H_
