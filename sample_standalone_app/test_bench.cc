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
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "space.h"
#include "init.h"
#include "index.h"
#include "params.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "methodfactory.h"
#include "spacefactory.h"

#include "ztimer.h"

#define REP_QTY 1000

#ifdef _MSC_VER
#include <windows.h>
#endif

using namespace std;
using namespace similarity;

float compAvgDist(KNNQuery<float>* qobj) {
  float sum = 0;
  size_t qty = 0;
  KNNQueue<float>* res = qobj->Result()->Clone();

  while (!res->Empty()) {
    sum += res->TopDistance();
    ++qty;
    res->Pop();
  }
  return sum/qty;
}

float compAvgDist(RangeQuery<float>* qobj) {
  const ObjectVector&    objs = *qobj->Result();
  const vector<float>&    dists = *qobj->ResultDists();

  if (objs.empty()) return 0;

  float sum = 0;

  for (size_t i = 0; i < objs.size(); ++i) {
    sum += dists[i];
  }
  return sum / objs.size();
}

template <class QueryType>
float doSearch(Index<float>* index, QueryType* qobj) {
  index->Search(qobj);

  return compAvgDist(qobj);
}

void Usage(const char *pProg, const char* pErr) {
  cerr << pErr << endl;
  cerr << "Usage: " << pProg << " <space name> <data file> <query file> " << endl;
};


int main(int argc, char* argv[]) {
  ObjectVector    dataSet; 
  ObjectVector    querySet; 

  if (argc != 4) {
    Usage(argv[0], "Wrong # of arguments");
    return 1;
  } 

  const char *spaceName = argv[1];
  const char *dataFile  = argv[2];
  const char *queryFile = argv[3];

  
  initLibrary(LIB_LOGSTDERR, NULL);

  // Create an instance of our custom space that uses L2-distance
  AnyParams empty;
  Space<float>    *space = SpaceFactoryRegistry<float>::Instance().CreateSpace(spaceName, empty);

  space->ReadDataset(dataSet,
                      NULL, // we don't need config here
                      dataFile,
                      0);

  space->ReadDataset(querySet,
                      NULL, // we don't need config here
                      queryFile,
                      0);

  Index<float>*   indexSmallWorld =  
                        MethodFactoryRegistry<float>::Instance().
                                CreateMethod(true /* print progress */,
                                        "small_world_rand",
                                        "custom", space,
                                        dataSet, 
                                        AnyParams(
                                                  {
                                                  "NN=17",
                                                  "initIndexAttempts=3",
                                                  "initSearchAttempts=1",
                                                  "indexThreadQty=4", /* 4 indexing threads */
                                                  }
                                                  )
                                        );

  cout << "Small-world index is created!" << endl;

  Index<float>*   indexSeqSearch = 
                        MethodFactoryRegistry<float>::Instance().
                                CreateMethod(false /* don't print progress */,
                                        "seq_search",
                                        "custom", space,
                                        dataSet, 
                                        AnyParams()
                                        );

  cout << "SEQ-search index is created!" << endl;

  Index<float>*methodsKNN[] = {indexSmallWorld, indexSeqSearch};


  for (Index<float>* method: methodsKNN) {
    float res = 0;
    WallClockTimer timer;

    for (const Object* queryObj: querySet) {
      unsigned K = 5; // 5-NN query
      KNNQuery<float>   knnQ(space, queryObj, K);

      res += doSearch(method, &knnQ);
    }

    timer.split();
    cout << "KNN-search" << endl;
    cout << "Method:       " << method->ToString() << endl;
    cout << "Avg time:     " << timer.elapsed()/1000.0/querySet.size() << " ms" << endl;
    cout << "Avg distance: " << res/querySet.size() << endl;
  }

  Index<float>*methodsRange[] = {indexSeqSearch};


  for (Index<float>* method: methodsRange) {
    float res = 0;
    WallClockTimer timer;

    for (const Object* queryObj: querySet) {
      float R = 100.0;
      RangeQuery<float>   knnQ(space, queryObj, R);

      res += doSearch(method, &knnQ);
    }

    timer.split();
    cout << "Range-search" << endl;
    cout << "Method:       " << method->ToString() << endl;
    cout << "Avg time:     " << timer.elapsed()/1000.0/querySet.size() << " ms" << endl;
    cout << "Avg distance: " << res/querySet.size() << endl;
  }


  delete indexSmallWorld;
  delete indexSeqSearch;

  for (const Object* obj: dataSet) delete obj;
  for (const Object* obj: querySet) delete obj;

  return 0;
};

