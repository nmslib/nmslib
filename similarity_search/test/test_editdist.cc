/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <string.h>
#include "space.h"
#include "scoped_ptr.h"
#include "common.h"
#include "bunit.h"

namespace similarity {

class StringDataset1 : public TestDataset {
 public:
  StringDataset1() {
    const char* arr[16] = {
      "xyz", "beagcfa", "cea", "cb",
      "d", "c", "bdaf", "ddcd",
      "egbfa", "a", "fba", "bcccfe",
      "ab", "bfgbfdc", "bcbbgf", "bfbb"
    };

    for (int i = 0; i < 16; ++i) {
      dataobjects_.push_back(new Object(i + 1, strlen(arr[i]), arr + i));
    }
  }
};


TEST(DISABLE_EditDistance) {
  /*
  StringDataset1 dataset;
  const ObjectVector& dataobjects = dataset.GetDataObjects();

  scoped_ptr<Space<int>> space(new EditSpace);
  const int expected[16][16] = {
    {0, 7, 3, 3, 3, 3, 4, 4, 5, 3, 3, 6, 3, 7, 6, 4},
    {7, 0, 5, 6, 7, 6, 4, 6, 3, 6, 6, 4, 6, 5, 5, 6},
    {3, 5, 0, 2, 3, 2, 3, 4, 4, 2, 2, 5, 3, 7, 5, 4},
    {3, 6, 2, 0, 2, 1, 4, 3, 4, 2, 2, 5, 1, 6, 4, 3},
    {3, 7, 3, 2, 0, 1, 3, 3, 5, 1, 3, 6, 2, 6, 6, 4},
    {3, 6, 2, 1, 1, 0, 4, 3, 5, 1, 3, 5, 2, 6, 5, 4},
    {4, 4, 3, 4, 3, 4, 0, 3, 4, 3, 3, 4, 3, 5, 4, 3},
    {4, 6, 4, 3, 3, 3, 3, 0, 5, 4, 4, 5, 4, 6, 6, 4},
    {5, 3, 4, 4, 5, 5, 4, 5, 0, 4, 3, 5, 4, 4, 5, 4},
    {3, 6, 2, 2, 1, 1, 3, 4, 4, 0, 2, 6, 1, 7, 6, 4},
    {3, 6, 2, 2, 3, 3, 3, 4, 3, 2, 0, 6, 2, 5, 5, 2},
    {6, 4, 5, 5, 6, 5, 4, 5, 5, 6, 6, 0, 6, 5, 4, 5},
    {3, 6, 3, 1, 2, 2, 3, 4, 4, 1, 2, 6, 0, 6, 5, 3},
    {7, 5, 7, 6, 6, 6, 5, 6, 4, 7, 5, 5, 6, 0, 5, 4},
    {6, 5, 5, 4, 6, 5, 4, 6, 5, 6, 5, 4, 5, 5, 0, 3},
    {4, 6, 4, 3, 4, 4, 3, 4, 4, 4, 2, 5, 3, 4, 3, 0}
  };

  for (size_t i = 0; i < 8; ++i) {
    for (size_t j = 0; j < 8; ++j) {
      const int d = space->Distance(dataobjects[i], dataobjects[j]);
      EXPECT_EQ(expected[i][j], d);
    }
  }
  */
}

}  // namespace similarity
