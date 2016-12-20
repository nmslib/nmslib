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

TEST(TestJaccard) {
  EXPECT_EQ(vvIds1.size(), vvIds2.size());
  EXPECT_EQ(vvIds1.size(), vInterQty.size());
  
  for (size_t i = 0; i < vvIds1.size(); ++i) {
    size_t qty1 = IntersectSizeScalarFast(&vvIds1[i][0], vvIds1[i].size(), 
                                          &vvIds2[i][0], vvIds2[i].size());
    size_t qty2 = IntersectSizeScalarStand(&vvIds1[i][0], vvIds1[i].size(), 
                                          &vvIds2[i][0], vvIds2[i].size());
    EXPECT_EQ(qty1, vInterQty[i]);
    EXPECT_EQ(qty2, vInterQty[i]);
  }
}

}  // namespace similarity

