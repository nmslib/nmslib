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
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <memory>
#include <cmath>
#include <string>
#include <vector>

#include "bunit.h"
#include "space.h"
#include "spacefactory.h"
#include "testdataset.h"
#include "params.h"
#include "space/space_sparse_lp.h"
#include "space/space_sparse_scalar.h"
#include "space/space_sparse_vector_inter.h"
#include "space/space_sparse_scalar_fast.h"
#include "space/space_sparse_vector.h"
#include "space/space_scalar.h"

namespace similarity {

using namespace std;

template <typename dist_t>
bool fullTest(const string& dataSetFileName, size_t maxNumRec, const string& tmpFileName, const string& spaceName, const char* pSpaceParams[]) {
  vector<string> spaceParams;

  LOG(LIB_INFO) << "Space name: " << spaceName << " data file: " << dataSetFileName << " maxNumRec=" << maxNumRec;

  while (*pSpaceParams) {
    spaceParams.push_back(*pSpaceParams++);
  }

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                  Instance().CreateSpace(spaceName, spaceParams));

  ObjectVector dataSet1;

  space->ReadDataset(dataSet1, sampleDataPrefix + dataSetFileName, maxNumRec);

  if (dataSet1.size() != maxNumRec) {
    LOG(LIB_ERROR) << "Bug or poorly designed test, expected to read " << maxNumRec << " records from " 
          << dataSetFileName << ", but read only: " << dataSet1.size();
    return false;
  }

  space->WriteDataset(dataSet1, tmpFileName);

  ObjectVector dataSet2;

  space->ReadDataset(dataSet2, tmpFileName);

  if (maxNumRec != dataSet2.size()) {
    LOG(LIB_ERROR) << "Expected to read " << maxNumRec << " records from " 
            "dataSet, but read only: " << dataSet2.size();
    return false;
  }

  for (size_t i = 0; i < maxNumRec; ++i) 
  if (!space->ApproxEqual(*dataSet1[i], *dataSet2[i])) {
    LOG(LIB_ERROR) << "Objects are different, i = " << i;
    LOG(LIB_ERROR) << "Object 1 string representation produced by the space:" <<
            space->CreateStrFromObj(dataSet1[i]);
    LOG(LIB_ERROR) << "Object 2 string representation produced by the space:" <<
            space->CreateStrFromObj(dataSet2[i]);
    return false;
  }

  return true;
}

const char *emptyParams[] = {NULL};

TEST(Test_DenseVectorSpace) {
  for (size_t maxNumRec = 1; maxNumRec < 10; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "l2", emptyParams));
  }
}


}
