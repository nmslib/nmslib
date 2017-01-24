/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2016
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <sys/time.h>

#include <logging.h>
#include <idtype.h>
#include <distcomp.h>
#include "bunit.h"

#include <vector>
#include <cmath>

#include <space/space_sparse_vector_inter.h>

using namespace std;

namespace similarity {

vector<vector<IdType>> vvIds1 = {
 {1, 2, 3, 4},

 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},

 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},

 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},

 {1, 2, 3, 4},
 {},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
 {1, 2, 3, 4},
};

vector<vector<IdType>> vvIds2 = {
 {1, 2, 3, 4},

 {2, 3, 4},
 {1, 3, 4},
 {1, 2, 4},
 {1, 2, 3},

 {1, 2},
 {1, 3},
 {1, 4},
 {2, 3},
 {2, 4},
 {3, 4},

 {1},
 {2},
 {3},
 {4},

 {},
 {},
 {5,6,7,8,9},
 {-2,-1,0},
 {-2,-1,0,5,6,7,8,9},
};

vector<size_t> vInterQty = {
 4,

 3,3,3,3,

 2,2,2,2,2,2,

 1,1,1,1,


 0, 0, 0, 0, 0
};

TEST(TestIntersect2Way) {
  EXPECT_EQ(vvIds1.size(), vvIds2.size());
  EXPECT_EQ(vvIds1.size(), vInterQty.size());
  
  for (size_t i = 0; i < vvIds1.size(); ++i) {
    size_t qty1 = IntersectSizeScalarFast(&vvIds1[i][0], vvIds1[i].size(), 
                                          &vvIds2[i][0], vvIds2[i].size());
    size_t qty2 = IntersectSizeScalarStand(&vvIds1[i][0], vvIds1[i].size(), 
                                          &vvIds2[i][0], vvIds2[i].size());
    EXPECT_EQ(qty1, vInterQty[i]);
    EXPECT_EQ(qty2, vInterQty[i]);

    if (qty1 != vInterQty[i] || 
        qty2 != vInterQty[i])
         LOG(LIB_INFO) << "Failed test (basic intersect funcs), index: " << i;

    const float oneElemVal = 0.15;
    vector<SparseVectElem<float>> elems1, elems2;
  
    for (IdType id1 : vvIds1[i]) {
      elems1.push_back(SparseVectElem<float>(id1, oneElemVal));
    } 
    for (IdType id2 : vvIds2[i]) {
      elems2.push_back(SparseVectElem<float>(id2, oneElemVal));
    } 
    OverlapInfo oinfo = SpaceSparseVectorInter<float>::ComputeOverlapInfo(elems1, elems2);

    if (oinfo.overlap_qty_ != vInterQty[i])
      LOG(LIB_INFO) << "Failed test (ComputeOverlapInfo), index: " << i;

    EXPECT_EQ((size_t)oinfo.overlap_qty_, vInterQty[i]);
    
    float vectQty1 = elems1.size();
    float norm1 = sqrt(oneElemVal*oneElemVal*vectQty1);
    float vectQty2 = elems2.size();
    float norm2 = sqrt(oneElemVal*oneElemVal*vectQty2);

    float overlap_qty = oinfo.overlap_qty_;
    float overlap_dotprod_norm = std::min(norm1, norm2) >0 ? overlap_qty * oneElemVal * oneElemVal / (norm1 * norm2) : 0; 
    //float overlap_dotprod_norm =  overlap_qty * oneElemVal * oneElemVal / (norm1 * norm2) ;
    CHECK(!isnan(overlap_dotprod_norm));
  
    const float eps = 0.0001f; // somewhat adhoc

    EXPECT_EQ_EPS(overlap_dotprod_norm, oinfo.overlap_dotprod_norm_, eps);
    //cerr << overlap_dotprod_norm << " # " << isnan(overlap_dotprod_norm) << " -> " << oinfo.overlap_dotprod_norm_ << endl;

    float diff_sum_left    = oneElemVal*(vectQty1 - overlap_qty);
    CHECK(!isnan(diff_sum_left));
    EXPECT_EQ_EPS(diff_sum_left, oinfo.diff_sum_left_, eps);
    float overlap_sum_left = oneElemVal*overlap_qty;
    CHECK(!isnan(overlap_sum_left));
    EXPECT_EQ_EPS(overlap_sum_left, oinfo.overlap_sum_left_, eps);

    float diff_sum_right = oneElemVal*(vectQty2 - overlap_qty);
    CHECK(!isnan(diff_sum_right));
    EXPECT_EQ_EPS(diff_sum_right, oinfo.diff_sum_right_, eps);
    float overlap_sum_right = oneElemVal*overlap_qty;
    CHECK(!isnan(overlap_sum_left));
    EXPECT_EQ_EPS(overlap_sum_right, oinfo.overlap_sum_right_, eps);
  }
}

vector<vector<IdType>> vvAddIds1 = {
  {1,2,3},  // 0
  {1,2,3},  // 1

  {1,2,3},  // 2
  {1,2,3},  // 3
  {1,2,3},  // 4

  {1,2,3},  // 5
  {1,2,3},  // 6
  {1,2,3},  // 7

  {1,2,3},  // 8

  {1,2},    // 9
  {1,2},    // 10
  {1,3},    // 11

  {1},      // 12
  {2},      // 13
  {3},      // 14

  {1,2,3},  // 15
};

vector<vector<IdType>> vvAddIds2 = {
  {1,2,3}, // 0
  {1,2,3}, // 1

  {1,2},   // 2
  {1,3},   // 3
  {2,3},   // 4

  {1,2},   // 5
  {1,3},   // 6
  {2,3},   // 7

  {2,3},   // 8

  {1},     // 9
  {2},     // 10
  {3},     // 11

  {1},     // 12
  {2},     // 13
  {3},     // 14
 
  {1,2,3}, // 15
};

vector<vector<IdType>> vvAddIds3 = {
  {1,2,3}, // 0
  {1,2},   // 1

  {1,2},   // 2
  {1,3},   // 3
  {2,3},   // 4

  {1},     // 5
  {3},     // 6
  {2},     // 7

  {1},     // 8

  {1},     // 9
  {2},     // 10
  {3},     // 11

  {3},     // 12
  {1},     // 13
  {2},     // 14

  {},      // 15
};

vector<size_t> vAddInterQty = {
  3, // 0
  2, // 1

  2, // 2
  2, // 3
  2, // 4

  1,
  1,
  1, // 7

  0, // 8

  1,
  1,
  1, // 11

  0, // 12
  0, // 13
  0,
  
  0
};

TEST(TestIntersect3Way) {
  EXPECT_EQ(vvAddIds1.size(), vvAddIds2.size());
  EXPECT_EQ(vvAddIds1.size(), vvAddIds3.size());
  EXPECT_EQ(vvAddIds1.size(), vAddInterQty.size());

  
  for (size_t i = 0; i < vvAddIds1.size(); ++i) {
    size_t qty1 = IntersectSizeScalar3way(&vvAddIds1[i][0], vvAddIds1[i].size(), 
                                          &vvAddIds2[i][0], vvAddIds2[i].size(),
                                          &vvAddIds3[i][0], vvAddIds3[i].size());
    size_t qty2 = IntersectSizeScalar3way(&vvAddIds3[i][0], vvAddIds3[i].size(), 
                                          &vvAddIds1[i][0], vvAddIds1[i].size(),
                                          &vvAddIds2[i][0], vvAddIds2[i].size());
    size_t qty3 = IntersectSizeScalar3way(&vvAddIds3[i][0], vvAddIds3[i].size(), 
                                          &vvAddIds2[i][0], vvAddIds2[i].size(),
                                          &vvAddIds1[i][0], vvAddIds1[i].size());
    if (qty1 != vAddInterQty[i] || 
        qty2 != vAddInterQty[i] || 
        qty3 != vAddInterQty[i]) LOG(LIB_INFO) << "Failed test (3-way intersect funcs), index: " << i;

    EXPECT_EQ(qty1, vAddInterQty[i]);
    EXPECT_EQ(qty2, vAddInterQty[i]);
    EXPECT_EQ(qty3, vAddInterQty[i]);
  }
}


}  // namespace similarity

