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

#include "logging.h"
#include "utils.h"
#include "bunit.h"

#include <limits>

#include <eval_metrics.h>

using namespace std;

namespace similarity {

template <class dist_t, class EvalObj>
void testMetric(
          size_t ExactResultSize,
          vector<ResultEntry<dist_t>> exactEntries,
          vector<ResultEntry<dist_t>> approxEntries,
          const double expVal) {
  unordered_set<IdType> exactIds;
  unordered_set<IdType> approxIds;

  // Let's sort the entries
  std::sort(exactEntries.begin(), exactEntries.end());
  std::sort(approxEntries.begin(), approxEntries.end());

  {
    size_t i = 0;
    for (const auto& e:exactEntries) {
      ++i;
      if (i > ExactResultSize) break;
      exactIds.insert(e.mId);
    }
  }
  for (const auto& e:approxEntries) approxIds.insert(e.mId);

  double val = EvalObj()(ExactResultSize, exactEntries, exactIds, approxEntries, approxIds);

  EXPECT_EQ_EPS(expVal, val, 1e-4);
};

typedef ResultEntry<float>  RESF;
typedef ResultEntry<int>    RESI;

const float EPSF = numeric_limits<float>::min();

const size_t KNN = 10;


TEST(TestRecallFloat) {
  vector<vector<RESF>> exactEntries = {
    {},
    {},
    {RESF(0, 0, 100)},
    { RESF(0, 0, 1), RESF(1, 0, 2), RESF(3, 0, 3), RESF(4, 0, 4), RESF(5, 0, 5), RESF(6, 0, 6), RESF(7, 0, 7), RESF(8, 0, 8), RESF(9, 0, 9), RESF(10, 0, 10), },
    { RESF(0, 0, 1), RESF(1, 0, 2), RESF(3, 0, 3), RESF(4, 0, 4), RESF(5, 0, 5), RESF(6, 0, 6), RESF(7, 0, 7), RESF(8, 0, 8), RESF(9, 0, 9), RESF(10, 0, 10), },
  };
  vector<vector<RESF>> approxEntries = {
    {},
    {RESF(0, 0, 100)},
    {},
    { RESF(0, 0, 1), RESF(3, 0, 3), RESF(5, 0, 5), RESF(7, 0, 7), RESF(9, 0, 9), },
    { RESF(0, 0, 1), RESF(1, 0, 2), RESF(3, 0, 3), RESF(4, 0, 4), RESF(5, 0, 5), RESF(6, 0, 6), RESF(7, 0, 7), RESF(8, 0, 8), RESF(9, 0, 9), RESF(10, 0, 10), },
  };
  vector<double> expRecall {
    1,
    1,
    0.0,
    0.5,
    1.0,
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expRecall.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<float,EvalRecall<float>>(KNN, exactEntries[i], approxEntries[i], expRecall[i]); 
    // In a special case when there are no results recall should be equal to 1
    testMetric<float,EvalRecall<float>>(0, exactEntries[i], approxEntries[i], 1.0);
  }
}

TEST(TestRecallInt) {
  vector<vector<RESI>> exactEntries = {
    {},
    {},
    {RESI(0, 0, 100)},
    { RESI(0, 0, 1), RESI(1, 0, 2), RESI(3, 0, 3), RESI(4, 0, 4), RESI(5, 0, 5), RESI(6, 0, 6), RESI(7, 0, 7), RESI(8, 0, 8), RESI(9, 0, 9), RESI(10, 0, 10), },
    { RESI(0, 0, 1), RESI(1, 0, 2), RESI(3, 0, 3), RESI(4, 0, 4), RESI(5, 0, 5), RESI(6, 0, 6), RESI(7, 0, 7), RESI(8, 0, 8), RESI(9, 0, 9), RESI(10, 0, 10), },
  };
  vector<vector<RESI>> approxEntries = {
    {},
    {RESI(0, 0, 100)},
    {},
    { RESI(0, 0, 1), RESI(3, 0, 3), RESI(5, 0, 5), RESI(7, 0, 7), RESI(9, 0, 9), },
    { RESI(0, 0, 1), RESI(1, 0, 2), RESI(3, 0, 3), RESI(4, 0, 4), RESI(5, 0, 5), RESI(6, 0, 6), RESI(7, 0, 7), RESI(8, 0, 8), RESI(9, 0, 9), RESI(10, 0, 10), },
  };
  vector<double> expRecall {
    1,
    1,
    0.0,
    0.5,
    1.0,
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expRecall.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<int,EvalRecall<int>>(KNN, exactEntries[i], approxEntries[i], expRecall[i]); 
    // In a special case when there are no results recall should be equal to 1
    testMetric<int,EvalRecall<int>>(0, exactEntries[i], approxEntries[i], 1.0);
  }
}

TEST(TestNumCloserFloat) {
  vector<vector<RESF>> exactEntries = {
    {},
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
  };
  vector<vector<RESF>> approxEntries = {
    {},
    {},
    { RESF(33, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
    { RESF(33, 0, EPSF), RESF(1, 0, 1 + numeric_limits<float>::epsilon()), RESF(2, 0, 2 + numeric_limits<float>::epsilon()) },
    { RESF(11, 0, 2), RESF(12, 0, 2.0001), RESF(13, 0, 2.0001) },
  };
  vector<double> expNumCloser {
    0,
    static_cast<double>(exactEntries[1].size()),
    0,
    0,
    2
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expNumCloser.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<float,EvalNumberCloser<float>>(KNN, exactEntries[i], approxEntries[i], expNumCloser[i]); 
    // In a special case when there are no results the number of points that are closer is 0 
    testMetric<float,EvalNumberCloser<float>>(0, exactEntries[i], approxEntries[i], 0.0); 
  }
}

TEST(TestNumCloserInt) {
  vector<vector<RESI>> exactEntries = {
    {},
    { RESI(0, 0, 1), RESI(1, 0, 1), RESI(2, 0, 1) },
    { RESI(0, 0, 1), RESI(1, 0, 1), RESI(2, 0, 1) },
    { RESI(0, 0, 1), RESI(1, 0, 3), RESI(2, 0, 3) },
    { RESI(0, 0, 1), RESI(1, 0, 1), RESI(2, 0, 1) },
  };
  vector<vector<RESI>> approxEntries = {
    {},
    {},
    { RESI(0, 0, 1), RESI(1, 0, 1), RESI(2, 0, 1) },
    { RESI(33, 0, 2), RESI(1, 0, 3), RESI(2, 0, 3) },
    { RESI(10, 0, 1), RESI(11, 0, 1), RESI(12, 0, 1) },
  };
  vector<double> expNumCloser {
    0,
    static_cast<double>(exactEntries[1].size()),
    0,
    1,
    0
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expNumCloser.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<int,EvalNumberCloser<int>>(KNN, exactEntries[i], approxEntries[i], expNumCloser[i]); 
    // In a special case when there are no results the number of points that are closer is 0 
    testMetric<int,EvalNumberCloser<int>>(0, exactEntries[i], approxEntries[i], 0.0); 
  }
}

TEST(TestRelPosErrorFloat) {
  vector<vector<RESF>> exactEntries = {
    {},
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2), RESF(3, 0, 3) },
    { RESF(0, 0, 33), RESF(1, 0, 33), RESF(2, 0, 33) },
    { RESF(0, 0, 33), RESF(1, 0, 33), RESF(2, 0, 33) },
  };
  vector<vector<RESF>> approxEntries = {
    {},
    {},
    { RESF(10, 0, 1), RESF(11, 0, 3) },
    { RESF(10, 0, 33), RESF(11, 0, 33), RESF(12, 0, 33) },
    { RESF(10, 0, 33 + + numeric_limits<float>::epsilon()), RESF(11, 0, 33 + + numeric_limits<float>::epsilon()), RESF(12, 0, 33 + numeric_limits<float>::epsilon()) },
  };
  vector<double> expNumCloser {
    0,
    log(static_cast<double>(exactEntries[1].size())),
    log(2),
    0,
    0,
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expNumCloser.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<float,EvalLogRelPosError<float>>(KNN, exactEntries[i], approxEntries[i], expNumCloser[i]); 
    /* 
     * In a special case when there are no results the relative position error should be 1.0 and
     * its logarithm should be zero
     */
    testMetric<float,EvalLogRelPosError<float>>(0, exactEntries[i], approxEntries[i], 0.0); 
  }
}

TEST(TestRelPosErrorInt) {
  vector<vector<RESI>> exactEntries = {
    {},
    { RESI(0, 0, 0), RESI(1, 0, 1), RESI(2, 0, 2) },
    { RESI(0, 0, 0), RESI(1, 0, 1), RESI(2, 0, 2), RESI(3, 0, 3) },
    { RESI(0, 0, 33), RESI(1, 0, 33), RESI(2, 0, 33) },
  };
  vector<vector<RESI>> approxEntries = {
    {},
    {},
    { RESI(10, 0, 1), RESI(11, 0, 3) },
    { RESI(10, 0, 33), RESI(11, 0, 33), RESI(12, 0, 33) },
  };
  vector<double> expNumCloser {
    0,
    log(static_cast<double>(exactEntries[1].size())),
    log(2),
    0,
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expNumCloser.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<int,EvalLogRelPosError<int>>(KNN, exactEntries[i], approxEntries[i], expNumCloser[i]); 
    /* 
     * In a special case when there are no results the relative position error should be 1.0 and
     * its logarithm should be zero
     */
    testMetric<int,EvalLogRelPosError<int>>(0, exactEntries[i], approxEntries[i], 0.0); 
  }
}

TEST(TestPrecisionOfApproxFloat) {
  vector<vector<RESF>> exactEntries = {
    {},
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2) },
    { RESF(0, 0, 0), RESF(1, 0, 1), RESF(2, 0, 2), RESF(3, 0, 3) },
    { RESF(0, 0, 33), RESF(1, 0, 33), RESF(2, 0, 33) },
    { RESF(0, 0, 33), RESF(1, 0, 33), RESF(2, 0, 33) },
  };
  vector<vector<RESF>> approxEntries = {
    {},
    {},
    { RESF(10, 0, 1), RESF(11, 0, 3) },
    { RESF(10, 0, 33), RESF(11, 0, 33), RESF(12, 0, 33) },
    { RESF(10, 0, 33 + + numeric_limits<float>::epsilon()), RESF(11, 0, 33 + + numeric_limits<float>::epsilon()), RESF(12, 0, 33 + numeric_limits<float>::epsilon()) },
  };
  vector<double> expNumCloser {
    1.0,
    0.0,
    0.5,
    1.0,
    1.0,
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expNumCloser.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<float,EvalPrecisionOfApprox<float>>(KNN, exactEntries[i], approxEntries[i], expNumCloser[i]); 
    // In a special case when there are no results, the precision of approximation should be equal to 1.0
    testMetric<float,EvalPrecisionOfApprox<float>>(0, exactEntries[i], approxEntries[i], 1.0); 
  }
}

TEST(TestPrecisionOfApproxInt) {
  vector<vector<RESI>> exactEntries = {
    {},
    { RESI(0, 0, 0), RESI(1, 0, 1), RESI(2, 0, 2) },
    { RESI(0, 0, 0), RESI(1, 0, 1), RESI(2, 0, 2), RESI(3, 0, 3) },
    { RESI(0, 0, 33), RESI(1, 0, 33), RESI(2, 0, 33) },
  };
  vector<vector<RESI>> approxEntries = {
    {},
    {},
    { RESI(10, 0, 1), RESI(11, 0, 3) },
    { RESI(10, 0, 33), RESI(11, 0, 33), RESI(12, 0, 33) },
  };
  vector<double> expNumCloser {
    1.0,
    0.0,
    0.5,
    1.0,
  };
  
  EXPECT_EQ(exactEntries.size(), approxEntries.size());
  EXPECT_EQ(exactEntries.size(), expNumCloser.size());

  for (size_t i = 0; i < exactEntries.size(); ++i) {
    testMetric<int,EvalPrecisionOfApprox<int>>(KNN, exactEntries[i], approxEntries[i], expNumCloser[i]); 
    // In a special case when there are no results, the precision of approximation should be equal to 1.0
    testMetric<int,EvalPrecisionOfApprox<int>>(0, exactEntries[i], approxEntries[i], 1.0); 
  }
}

}
