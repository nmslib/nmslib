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
#ifdef _MSC_VER
#define _SCL_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <memory>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>

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
#include "space/space_word_embed.h"

#define MAX_NUM_REC 10

namespace similarity {

using namespace std;

template <typename dist_t>
bool fullTestCommon(Space<dist_t>* pSpace, const ObjectVector& dataSet1, const vector<string>& vExternIds1, size_t maxNumRec, const string& tmpFileName,bool bTestExternId) {
  pSpace->WriteDataset(dataSet1, vExternIds1, tmpFileName);

  ObjectVector dataSet2;
  vector<string> vExternIds2;

  unique_ptr<DataFileInputState> inpState(pSpace->ReadDataset(dataSet2, vExternIds2, tmpFileName));
  pSpace->UpdateParamsFromFile(*inpState);

  if (maxNumRec != dataSet2.size()) {
    LOG(LIB_ERROR) << "Expected to read " << maxNumRec << " records from " 
            "dataSet, but read only: " << dataSet2.size();
    return false;
  }

  if (vExternIds2.size() != dataSet2.size()) {
    LOG(LIB_ERROR) << "The number of external IDs (" << vExternIds1.size() << ") is different from the number of records: " << dataSet2.size();
    return false;
  }

  for (size_t i = 0; i < maxNumRec; ++i) {
    if (bTestExternId) {
      if (vExternIds1[i] != vExternIds2[i]) {
        LOG(LIB_ERROR) << "External IDs are different, i = " << i << " id1 = '" << vExternIds1[i] << "' id2 = '" << vExternIds2[i] << "'" ;
        return false;
      }
    }
    if (!pSpace->ApproxEqual(*dataSet1[i], *dataSet2[i])) {
      LOG(LIB_ERROR) << "Objects are different, i = " << i;
      LOG(LIB_ERROR) << "Object 1 string representation produced by the space:" <<
              pSpace->CreateStrFromObj(dataSet1[i], vExternIds1[i]);
      LOG(LIB_ERROR) << "Object 2 string representation produced by the space:" <<
              pSpace->CreateStrFromObj(dataSet2[i], vExternIds2[i]);
      return false;
    }
  }

  return true;
}

template <typename dist_t>
bool fullTest(const string& dataSetFileName, size_t maxNumRec, const string& tmpFileName, const string& spaceName, const char* pSpaceParams[], bool bTestExternId) {
  vector<string> spaceParams;

  LOG(LIB_INFO) << "Space name: " << spaceName << " distance type: " << DistTypeName<dist_t>() << " data file: " << dataSetFileName << " maxNumRec=" << maxNumRec;

  while (*pSpaceParams) {
    spaceParams.push_back(*pSpaceParams++);
  }

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                  Instance().CreateSpace(spaceName, spaceParams));

  ObjectVector dataSet1;
  vector<string> vExternIds1;

  unique_ptr<DataFileInputState> inpState(space->ReadDataset(dataSet1, vExternIds1, sampleDataPrefix + dataSetFileName, maxNumRec));
  space->UpdateParamsFromFile(*inpState);

  if (dataSet1.size() != maxNumRec) {
    LOG(LIB_ERROR) << "Bug or poorly designed test, expected to read " << maxNumRec << " records from " 
          << dataSetFileName << ", but read only: " << dataSet1.size();
    return false;
  }

  return fullTestCommon(space.get(), dataSet1, vExternIds1, maxNumRec, tmpFileName, bTestExternId); 
}

template <typename dist_t>
bool fullTest(const vector<string>& dataSetStr, size_t maxNumRec, const string& tmpFileName, const string& spaceName, const char* pSpaceParams[], bool bTestExternId) {
  vector<string> spaceParams;

  LOG(LIB_INFO) << "Space name: " << spaceName << " maxNumRec=" << maxNumRec;

  while (*pSpaceParams) {
    spaceParams.push_back(*pSpaceParams++);
  }

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                  Instance().CreateSpace(spaceName, spaceParams));

  ObjectVector dataSet1;
  vector<string> vExternIds1;

  IdType id = 0;
  for (string s: dataSetStr) {
    stringstream ss;
    ss << id;
      
    dataSet1.push_back(space->CreateObjFromStr(id++, -1, s, NULL).release());
    vExternIds1.push_back(ss.str());
    
    if (id >= maxNumRec) break;
  }

  if (dataSet1.size() < maxNumRec) {
    LOG(LIB_ERROR) << "Bug or poorly designed test, expected to create " << maxNumRec << " records from array but read only: " << dataSet1.size();
    return false;
  }

  return fullTestCommon(space.get(), dataSet1, vExternIds1, maxNumRec, tmpFileName, bTestExternId); 
}

const char *emptyParams[] = {NULL};
const char *paramsDistL2[]     = {"dist=" SPACE_WORD_EMBED_DIST_L2, NULL};
const char *paramsDistCosine[] = {"dist=" SPACE_WORD_EMBED_DIST_COSINE, NULL};

TEST(Test_WordEmbedSpace) {
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("glove.6B.100d_100.txt", maxNumRec, "tmp_out_file.txt", SPACE_WORD_EMBED, paramsDistL2, true));
    EXPECT_EQ(true, fullTest<double>("glove.6B.100d_100.txt", maxNumRec, "tmp_out_file.txt",SPACE_WORD_EMBED, paramsDistCosine, true));
    EXPECT_EQ(true, fullTest<float>("glove.6B.100d_100.txt", maxNumRec, "tmp_out_file.txt", SPACE_WORD_EMBED, paramsDistL2, true));
    EXPECT_EQ(true, fullTest<double>("glove.6B.100d_100.txt", maxNumRec, "tmp_out_file.txt",SPACE_WORD_EMBED, paramsDistCosine, true));
  }
}

TEST(Test_DenseVectorSpace) {
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "l2", emptyParams, false));
    EXPECT_EQ(true, fullTest<double>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "l2", emptyParams, false));
  }
}

TEST(Test_DenseVectorKLDiv) {
  // Test KL-diverg. with and without precomputation of logarithms
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "kldivgenfast", emptyParams, false));
    EXPECT_EQ(true, fullTest<double>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "kldivgenfast", emptyParams, false));
    EXPECT_EQ(true, fullTest<float>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "kldivgenslow", emptyParams, false));
    EXPECT_EQ(true, fullTest<double>("final128_10K.txt", maxNumRec, "tmp_out_file.txt", "kldivgenslow", emptyParams, false));
  }
}

TEST(Test_SparseVectorSpace) {
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("sparse_5K.txt", maxNumRec, "tmp_out_file.txt", "cosinesimil_sparse", emptyParams, false));
    EXPECT_EQ(true, fullTest<float>("sparse_5K.txt", maxNumRec, "tmp_out_file.txt", "angulardist_sparse", emptyParams, false));
    EXPECT_EQ(true, fullTest<double>("sparse_5K.txt", maxNumRec, "tmp_out_file.txt", "cosinesimil_sparse", emptyParams, false));
    EXPECT_EQ(true, fullTest<double>("sparse_5K.txt", maxNumRec, "tmp_out_file.txt", "angulardist_sparse", emptyParams, false));
  }
}

TEST(Test_SparseVectorSpaceFast) {
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("sparse_5K.txt", maxNumRec, "tmp_out_file.txt", "cosinesimil_sparse_fast", emptyParams, false));
    EXPECT_EQ(true, fullTest<float>("sparse_5K.txt", maxNumRec, "tmp_out_file.txt", "angulardist_sparse_fast", emptyParams, false));
  }
}

TEST(Test_StringSpace) {
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<int>("dna32_4_5K.txt", maxNumRec, "tmp_out_file.txt", "leven", emptyParams, false));
  }
}

TEST(Test_BitHamming) {
  vector<string> testVect;

  for (size_t i = 0; i < MAX_NUM_REC; ++i) {
    stringstream ss;

    for (size_t k = 0; k < 128; ++k) {
      if (k) ss << " ";
      ss << (RandomInt() % 2);
    }
    testVect.push_back(ss.str());
  }
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<int>(testVect, maxNumRec, "tmp_out_file.txt", "bit_hamming", emptyParams, false));
  }
}

#if defined(WITH_EXTRAS)
TEST(Test_SQFD) {
  const char* sqfdParams[] = {"alpha=1", NULL} ; 
  for (size_t maxNumRec = 1; maxNumRec < MAX_NUM_REC; ++maxNumRec) {
    EXPECT_EQ(true, fullTest<float>("sqfd20_10k_10k.txt", maxNumRec, "tmp_out_file.txt", "sqfd_heuristic_func", sqfdParams, false));
    EXPECT_EQ(true, fullTest<double>("sqfd20_10k_10k.txt", maxNumRec, "tmp_out_file.txt", "sqfd_heuristic_func", sqfdParams, false));
  }
}
#endif


}
