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
   * The constructor here accepts a pointer to the space, 
   * a reference to an array of data objects,
   * and a reference to the parameter manager,
   * which is used to retrieve parameters.
   *
   * The constructor creates a search index (or calls a function to create it)!
   * However, in this simple case, it simply memorizes a reference to the 
   * array of data objects, which is guaranteed to exist through the complete
   * test cycle.
   *
   * Note that we have a query time parameter here.
   *
   */
  DummyMethod(const Space<dist_t>* space, 
              const ObjectVector& data, 
              AnyParamManager& pmgr) 
              : data_(data) {
    pmgr.GetParamOptional("doSeqSearch",  bDoSeqSearch_);
    SetQueryTimeParamsInternal(pmgr);
  }
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
  void Search(RangeQuery<dist_t>* query);
  void Search(KNNQuery<dist_t>* query);

  virtual vector<string> GetQueryTimeParamNames() const;

 private:
  virtual void SetQueryTimeParamsInternal(AnyParamManager& );

  const ObjectVector&     data_;
  bool                    bDoSeqSearch_;
  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(DummyMethod);
};

}   // namespace similarity

#endif     // _DUMMY_METHOD_H_
