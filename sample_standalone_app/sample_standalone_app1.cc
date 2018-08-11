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
#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sstream>

#include "data.h"

#include "custom_space.h"
#include "init.h"
#include "index.h"
#include "params.h"
#include "rangequery.h"
#include "knnquery.h"
#include "knnqueue.h"
#include "methodfactory.h"

#include "ztimer.h"

#define REP_QTY 1000

#ifdef _MSC_VER
#include <windows.h>
#endif

using namespace std;
using namespace similarity;

#define LOG_OPTION 1

void Usage(const char *pProg, const char* pErr) {
  cerr << pErr << endl;
  cerr << "Usage: " << pProg << " <test file> " << endl;
  cerr << "Usage: " << pProg << " <test file> <max #of vectors> " << endl;
  cerr << "Usage: " << pProg << endl;
};

/*
 * Define an implementation of the distance function.
 */
struct DistL2 {
  /*
   * Important: the function is const and arguments are const as well!!!
   */
  float operator()(const float* x, const float* y, size_t qty) const {
    float res = 0;
    for (size_t i = 0; i < qty; ++i) res+=(x[i]-y[i])*(x[i]-y[i]);
    return sqrt(res);
  }
};

void printResults(KNNQuery<float>* qobj) {
  KNNQueue<float>* res = qobj->Result()->Clone();

  while (!res->Empty()) {
    cout << res->TopObject()->id() << " : " << res->TopDistance() << endl;
    res->Pop();
  }
  delete res;
}

void printResults(RangeQuery<float>* qobj) {
  const ObjectVector&    objs = *qobj->Result();
  const vector<float>&    dists = *qobj->ResultDists();

  for (size_t i = 0; i < objs.size(); ++i) {
    cout << objs[i]->id() << " : " << dists[i] << endl;
  }
}

template <class QueryType>
void doSearch(Index<float>* index, QueryType* qobj, int repQty) {
  WallClockTimer timer;

  /*
   * In this example we repeat the search many times,
   * but only because we need to measure the result
   * properly.
   */
  for (int i = 0; i < repQty; ++i) {
     index->Search(qobj);
	 if (i + 1 < repQty) qobj->Reset(); // This is needed b/c we reuse the same query many times
  }

  timer.split();

  cout << "Search " << qobj->Type() << " using index: " << index->StrDesc() 
	   << " repeated: " << repQty << " times " << endl;
  cout << "Avg time:  " << timer.elapsed()/1000.0/repQty << " ms" << endl;
  cout << "# of results: " << qobj->ResultSize() << endl;

  printResults(qobj);
}


int main(int argc, char* argv[]) {
  ObjectVector    dataSet; 
  const Object*   queryObj = NULL;

  // Create an instance of our custom space that uses L2-distance
  VectorSpaceGen<float, DistL2>   customSpace;
  vector<string>                  vExternIds;

  const char* fileName = NULL;
  if (argc == 2 || argc == 3) {
    fileName = argv[1];
    // Here we will read data from a file
    int MaxNumObjects = 100; // read at most 100 objects, if 0 all objects are read
    if (argc == 3) MaxNumObjects = atol(argv[2]);
    customSpace.ReadDataset(dataSet, vExternIds, fileName, MaxNumObjects);
    cout << "Read: "<< dataSet.size() << " objects" << endl;
    if (dataSet.size() < 2) {
      cerr << "Too few data elements in " << fileName << endl; 
      return 1;
    }
  } else if (argc == 1) {
    /* 
     * labels to check the accuracy of classification.
     */
    
    vector<LabelType> labels(rawData.size()); 

    // If the file is not specified, create the data set from a vector of vectors
    customSpace.CreateDataset(dataSet, rawData, labels); 
  } else {
    Usage(argv[0], "Wrong # of arguments");
    return 1;
  } 

  assert(dataSet.size() > 1);

  queryObj = dataSet[0];
  dataSet.erase(dataSet.begin());

  cout << "Using the first object as the query vector (this object is removed from the dataset)" << endl;
  cout << "The number of remaining objects is: " << dataSet.size() << " "; 

  if (fileName == NULL) cout << " created from vector<vector<...>> "; 
  else cout << "read from file: " << fileName << endl;

  /* 
   * Clearing memory: we would rather use some smart pointer here.
   *                  can't use standard shared_ptr, b/c they have
   *                  performance issues.
   * see, e.g.: http://nerds-central.blogspot.com/2012/03/sharedptr-performance-issues-and.html 
   */

  cout << "We have the space and the query, let's create some search index." << endl;

  int seed = 0;

  /* 
   * Init library, specify a log file
   */
  if (LOG_OPTION == 1)
    initLibrary(seed, LIB_LOGFILE, "logfile.txt"); 
  // No logging 
  if (LOG_OPTION == 2)
    initLibrary(seed, LIB_LOGNONE, NULL);
  // Use STDERR
  if (LOG_OPTION == 3)
    initLibrary(seed, LIB_LOGSTDERR, NULL);

  AnyParams IndexParams(
                        {
                        "NN=11",
                        "efConstruction=50",
                        "indexThreadQty=4" /* 4 indexing threads */
                        });

  AnyParams QueryTimeParams( { "efSearch=50" });
                          
  Index<float>*   indexSmallWorld =  
                        MethodFactoryRegistry<float>::Instance().
                                CreateMethod(true /* print progress */,
                                        "small_world_rand",
                                        "custom",
                                         customSpace,
                                         dataSet);

  // Creating an index
  indexSmallWorld->CreateIndex(IndexParams);
  // Setting query-time parameters
  indexSmallWorld->SetQueryTimeParams(QueryTimeParams);

  cout << "Small-world index is created!" << endl;

  IndexParams     = getEmptyParams();
  QueryTimeParams = AnyParams({"alphaLeft=1.0", "alphaRight=1.0"});

  Index<float>*   indexVPTree = 
                        MethodFactoryRegistry<float>::Instance().
                                CreateMethod(false /* don't print progress */,
                                        "vptree",
                                        "custom",
                                        customSpace,
                                        dataSet);

  indexVPTree->CreateIndex(IndexParams);
  indexVPTree->SetQueryTimeParams(QueryTimeParams);

  cout << "VP-tree index is created!" << endl;

  IndexParams= AnyParams({ "projDim=16",   // Projection dimensionality
                           "projType=perm", // using permutations => the number of pivots is equal to projDim and should be < #of objects
                         });

  QueryTimeParams = AnyParams({ "dbScanFrac=0.2", // A fraction of the data set to scan
                             });

  Index<float>*   indexPerm = 
                           MethodFactoryRegistry<float>::Instance().
                                CreateMethod(false /* don't print progress */,
                                        "proj_incsort",
                                        "custom",
                                         customSpace,
                                         dataSet);

  indexPerm->CreateIndex(IndexParams);
  indexPerm->SetQueryTimeParams(QueryTimeParams);

  cout << "Permutation index is created!" << endl;

  /* Now let's try some searches */
  float radius = 0.12;
  RangeQuery<float>   rangeQ(customSpace, queryObj, radius);

  //doSearch(indexSmallWorld, &rangeQ); not supported for small world method
  doSearch(indexVPTree, &rangeQ, REP_QTY);
  doSearch(indexPerm, &rangeQ, REP_QTY);

  unsigned K = 5; // 5-NN query
  KNNQuery<float>   knnQ(customSpace, queryObj, K);

  cout << "Setting one value of a query-time param (small  world)" << endl;
  indexSmallWorld->SetQueryTimeParams(AnyParams({ "efSearch=100" }));
  doSearch(indexSmallWorld, &knnQ, REP_QTY);
  cout << "Setting one value of a query-time param (small  world)" << endl;
  indexSmallWorld->SetQueryTimeParams(AnyParams({ "efSearch=50" }));
  doSearch(indexSmallWorld, &knnQ, REP_QTY);

  doSearch(indexVPTree, &knnQ, REP_QTY);

  cout << "Setting one value of a query-time param (permutation method)" << endl;
  indexPerm->SetQueryTimeParams(AnyParams( { "dbScanFrac=0.05" }));
  doSearch(indexPerm, &knnQ, REP_QTY);
  cout << "Setting another value of a query-time param (permutation method)" << endl;
  indexPerm->SetQueryTimeParams(AnyParams( { "dbScanFrac=0.5" }));
  doSearch(indexPerm, &knnQ, REP_QTY);

  cout << "Saving vectors to a file: " << endl;

  // The number of external IDs must match the number of objects, even if these external
  // IDs are ignored by the space API.
  vExternIds.resize(dataSet.size()); 
  customSpace.WriteDataset(dataSet, vExternIds, "testdataset.txt");

  cout << "Deleting objects..." << endl;

  delete indexSmallWorld;
  delete indexVPTree;
  delete indexPerm;

  delete queryObj;

  for (const Object* obj: dataSet) delete obj;

  return 0;
};

