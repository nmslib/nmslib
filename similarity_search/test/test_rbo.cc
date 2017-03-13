/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2017
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include "logging.h"
#include "utils.h"
#include "bunit.h"

#include <limits>
#include <iostream>
#include <vector>
#include <stdexcept>

#include <rbo.h>

using namespace std;

/*
 * Checking if our wrapper for RBO code produces correct results.
 *
 *  RBO metric is described in:
 *
 *  @article{wmz10:acmtois,
 *      author = "Webber, William and Moffat, Alistair and Zobel, Justin",
 *      title = "A similarity measure for indefinite rankings",
 *      journal = "ACM Transactions on Information Systems",
 *      year = {2010},
 *  }
 *
 * Extrapolated RBO values are computed using the original utility 
 * provided by the author. It can be downloaded from
 * http://www.williamwebber.com/research/
 *
 * The only difference is that original code produces NaNs if one list is empty.
 * We, in contrast, produce zeros.
 */


namespace similarity {

TEST(TestRBO) {
  try {
    vector<vector<IdType>> vvRanks = {
      {1, 2, 3},  {},
      {1, 2},    {2, 1},
      {1, 2, 3}, {1, 2},
      {1, 2, 3}, {3, 2, 1},
      {1, 2, 3, 4}, {3,1,7, 5},
    };
    vector<float>         vP   = {0.8, 0.9 };
    vector<vector<float>> vRes = {
      {0,   0},
      {0.8, 0.9},
      {1.0, 1.0},
      {0.72, 0.855},
      {0.421333, 0.463500},
    };

    CHECK(vvRanks.size() % 2 == 0);
    size_t testQty = vvRanks.size() / 2;
    CHECK(vRes.size() == testQty);

    for (size_t testId = 0; testId < testQty; ++testId) {
      CHECK(vRes[testId].size() == vP.size());
      for (size_t pId = 0; pId < vP.size(); ++pId) {
        float p = vP[pId];
        float rbo1 = ComputeRBO(vvRanks[2*testId], vvRanks[2*testId+1], p);
        float rbo2 = ComputeRBO(vvRanks[2*testId+1], vvRanks[2*testId], p);
        EXPECT_EQ_EPS(rbo1, rbo2, 1e-5f); 
        EXPECT_EQ_EPS(rbo1, vRes[testId][pId], 1e-5f); 
      }
    }
  } catch (const exception& e) {
    throw TestException(e.what());;
  };
}

}
