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
#ifndef _TEST_DATASET_H_
#define _TEST_DATASET_H_

#include "object.h"
#include "space.h"

#include <string>

namespace similarity {

using std::string;

const string sampleDataPrefix = string("..") + PATH_SEPARATOR + string("sample_data") + PATH_SEPARATOR;

class TestDataset {
 public:
  virtual ~TestDataset() {
    for (auto it = dataobjects_.begin(); it != dataobjects_.end(); ++it) {
      delete *it;
    }
  }

  const ObjectVector& GetDataObjects() const { return dataobjects_; }

 protected:
  ObjectVector dataobjects_;
};

}  // namespace similarity

#endif      //  _TEST_DATASET_H_
