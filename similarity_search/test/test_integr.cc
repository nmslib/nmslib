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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmath>
#include <limits>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <set>

#include "init.h"
#include "global.h"
#include "utils.h"
#include "memory.h"
#include "ztimer.h"
#include "experiments.h"
#include "experimentconf.h"
#include "space.h"
#include "spacefactory.h"
#include "logging.h"
#include "methodfactory.h"

#include "meta_analysis.h"
#include "params.h"

#include "test_integr_util.h"
#include "testdataset.h"

using namespace similarity;

using std::set;
using std::multimap;
using std::vector;
using std::string;
using std::stringstream;

#define MAX_THREAD_QTY 4
#define TEST_SET_QTY  "10"
#define MAX_NUM_QUERY "100"

vector<MethodTestCase>    vTestCaseDesc = {
#if 0
#endif
  MethodTestCase("float", "l2", "final8_10K.txt", "vptree:chunkBucket=1,bucketSize=10", 
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 55, 65),  
  MethodTestCase("float", "l2", "final8_10K.txt", "vptree:chunkBucket=1,bucketSize=10,alphaLeft=2,alphaRight=2", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.93, 0.97, 0.03, 0.09, 120, 160),  
  MethodTestCase("float", "l2", "final128_10K.txt", "vptree:chunkBucket=1,bucketSize=10", 
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 1.5, 1.8),  
  MethodTestCase("float", "l2", "final128_10K.txt", "vptree:chunkBucket=1,bucketSize=10,alphaLeft=2,alphaRight=2", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.98, 1.0, 0.0, 0.02, 2.8, 3.4),  
  MethodTestCase("float", "l2", "final8_10K.txt", "vptree:chunkBucket=1,bucketSize=10", 
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 20, 24),  
  MethodTestCase("float", "l2", "final8_10K.txt", "vptree:chunkBucket=1,bucketSize=10,alphaLeft=2,alphaRight=2", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.93, 0.96, 0.0, 0.02, 56, 63),  
  MethodTestCase("float", "l2", "final128_10K.txt", "vptree:chunkBucket=1,bucketSize=10", 
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 1.1, 1.3),  
  MethodTestCase("float", "l2", "final128_10K.txt", "vptree:chunkBucket=1,bucketSize=10,alphaLeft=2,alphaRight=2", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.99, 0.999, 0.0, 0.01, 1.7, 1.9),  

};


int main(int ac, char* av[]) {
  // This should be the first function called before
  string LogFile;
  if (ac == 2) LogFile = av[1];

  initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  WallClockTimer timer;
  timer.reset();

  set<string>           setDistType;
  set<string>           setSpaceType;
  set<string>           setDataSet;
  set<unsigned>         setKNN;
  set<float>            setRange;

  for (const auto& testCase: vTestCaseDesc) {
    setDistType.insert(testCase.mDistType);
    setSpaceType.insert(testCase.mSpaceType);
    setDataSet.insert(testCase.mDataSet);
    if (testCase.mKNN > 0)
      setKNN.insert(testCase.mKNN);
    if (testCase.mRange > 0)
      setRange.insert(testCase.mRange);
  };  

  size_t nTest = 0;
  size_t nFail = 0;
  /* 
   * 1. Let's iterate over all combinations of data sets,
   * distance, and space types. 
   * 2. For each combination, we select test cases 
   * with exactly same data set, distance and space type. 
   * 3. Create an array of arguments in the same format
   *    as used by the main benchmarking utility.
   * 4. Use a standard function to parse these arguments.
   */
  for (string dataSet  : setDataSet)
  for (string distType : setDistType)
  for (string spaceType: setSpaceType) {
    vector<string>    vArgvInit;


    vArgvInit.push_back("--distType"); vArgvInit.push_back(distType);
    vArgvInit.push_back("--spaceType"); vArgvInit.push_back(spaceType);
    vArgvInit.push_back("--dataFile"); vArgvInit.push_back(sampleDataPrefix + dataSet);

    vArgvInit.push_back("--testSetQty"); vArgvInit.push_back(TEST_SET_QTY);
    vArgvInit.push_back("--maxNumQuery"); vArgvInit.push_back(MAX_NUM_QUERY);

    
    for (unsigned K: setKNN) {
      vector<string>          vArgv = vArgvInit;
      vector<MethodTestCase>  vTestCases; 

      vArgv.push_back("--knn");
      stringstream cmn;
      cmn << K;
      vArgv.push_back(cmn.str());

      // Select appropriate test cases
      for (const auto& testCase: vTestCaseDesc) {
        if (testCase.mDataSet == dataSet &&
            testCase.mDistType == distType && 
            testCase.mSpaceType == spaceType &&
            testCase.mKNN  == K) {
          vArgv.push_back("--method");
          vArgv.push_back(testCase.mMethodDesc);
          vTestCases.push_back(MethodTestCase(testCase));
        }
      }

      if (!vTestCases.empty())  { // Not all combinations of spaces, data sets, and search types are non-empty
        for (size_t k = 1; k <= MAX_THREAD_QTY; ++k) {
          vArgv.push_back("--threadTestQty");
          stringstream cmn;
          cmn << k;
          vArgv.push_back(cmn.str());

          nTest += vTestCases.size();
          nFail += RunOneTest(vTestCases, vArgv);

          vArgv.erase(vArgv.begin() + vArgv.size() - 1);
          vArgv.erase(vArgv.begin() + vArgv.size() - 1);
        }
      }
    }

    for (float R: setRange) {
      vector<string>          vArgv = vArgvInit;
      vector<MethodTestCase>  vTestCases; 

      vArgv.push_back("--range");
      stringstream cmn;
      cmn << R;
      vArgv.push_back(cmn.str());

      // Select appropriate test cases
      for (const auto& testCase: vTestCaseDesc) {
        if (testCase.mDataSet == dataSet &&
            testCase.mDistType == distType && 
            testCase.mSpaceType == spaceType &&
            testCase.mRange  == R) {
          vArgv.push_back("--method");
          vArgv.push_back(testCase.mMethodDesc);
          vTestCases.push_back(MethodTestCase(testCase));
        }
      }

      if (!vTestCases.empty())  { // Not all combinations of spaces, data sets, and search types are non-empty
        for (size_t k = 1; k <= MAX_THREAD_QTY; ++k) {
          vArgv.push_back("--threadTestQty");
          stringstream cmn;
          cmn << k;
          vArgv.push_back(cmn.str());

          nTest += vTestCases.size();
          nFail += RunOneTest(vTestCases, vArgv);

          vArgv.erase(vArgv.begin() + vArgv.size() - 1);
          vArgv.erase(vArgv.begin() + vArgv.size() - 1);
        }
      }

    }

  }


  timer.split();

  LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();

  cerr << endl << "==================================================" << endl;
  cerr << (nFail ? "FAILURE" : "SUCCESS") << endl;
  cerr << "Carried out: " << nTest << "  tests. Failed: " << nFail << " tests" << endl;

  return nFail ? 1:0;
}
