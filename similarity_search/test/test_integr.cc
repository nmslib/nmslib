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

//#define MAX_THREAD_QTY 4
#define MAX_THREAD_QTY 1
#define TEST_SET_QTY   3
#define MAX_NUM_QUERY  700

#define INDEX_FILE_NAME "index.tmp" 

#define TEST_HNSW 1
#define TEST_SW_GRAPH 1
#define TEST_IR 1
#define TEST_NAPP 1
#define TEST_OTHER 1

// TODO something is wrong with FALCONN, 	+#define TEST_FALCONN 1
// When we run it as a single thing in the binary, it works fine,	
// but crashes when we run it jointly with other methods.
#define TEST_FALCONN 0

vector<MethodTestCase>    vTestCaseDesc = {
#if (TEST_HNSW)
  // Make sure, it works with huge M
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil_sparse", "sparse_5K.txt", "hnsw", true, "efConstruction=100,M=400", "ef=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.98, 0.9999, 0.0, 1, 1.3, 2.2),  
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil_sparse", "sparse_5K.txt", "hnsw", true, "efConstruction=200,M=10", "ef=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.88, 0.96, 0.0, 1, 6, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil_sparse", "sparse_5K.txt", "hnsw", true, "efConstruction=200,M=10", "ef=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.88, 0.96, 0.0, 1, 6, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "angulardist_sparse", "sparse_5K.txt", "hnsw", true, "efConstruction=200,M=10", "ef=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.88, 0.96, 0.0, 1, 6, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil", "final8_10K.txt", "hnsw", true, "efConstruction=200,M=10,skip_optimized_index=1", "ef=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.96, 1, 0, 0.1, 40, 60),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "hnsw", true, "efConstruction=200,M=10,skip_optimized_index=1", "ef=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.96, 1, 0, 0.1, 40, 60),  
#endif

#if (TEST_SW_GRAPH)
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "sw-graph", true, "NN=10", "",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.0, 0, 1.0, 36, 55),  
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil_sparse_fast", "sparse_5K.txt", "sw-graph", true, "efConstruction=200,NN=10", "efSearch=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.88, 0.96, 0.0, 1, 5, 10),  
  MethodTestCase(DIST_TYPE_FLOAT, "angulardist_sparse_fast", "sparse_5K.txt", "sw-graph", true, "efConstruction=200,NN=10", "efSearch=50", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.88, 0.96, 0.0, 1, 5, 10),  
#endif


#if (TEST_IR)
  MethodTestCase(DIST_TYPE_FLOAT, "negdotprod_sparse_fast", "sparse_5K.txt", "simple_invindx", false, "", "", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.999, 1.0, 0.0, 0.001, 395, 510),  
  MethodTestCase(DIST_TYPE_FLOAT, "negdotprod_sparse_fast", "sparse_5K.txt", "wand_invindx", false, "", "", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.999, 1.0, 0.0, 0.001, 395, 510),  
  MethodTestCase(DIST_TYPE_FLOAT, "negdotprod_sparse_fast", "sparse_5K.txt", "blkmax_invindx", false, "", "", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.999, 1.0, 0.0, 0.001, 395, 510),  
#endif

  // *************** FALCONN test ***************************** //
#if (TEST_FALCONN)
#ifdef WITH_EXTRAS
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil_sparse_fast", "sparse_5K.txt", "falconn", false, "num_hash_tables=20,num_hash_bits=7,feature_hashing_dimension=128,use_falconn_dist=0", "num_probes=20",
                1 /* KNN-1 */, 0 /* no range search */ , 0.65, 0.79, 0.5, 1.5, 5.75, 6.75),  
  MethodTestCase(DIST_TYPE_FLOAT, "cosinesimil", "final8_10K.txt", "falconn", false, "num_hash_tables=1,num_hash_bits=11,use_falconn_dist=0", "num_probes=1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.65, 0.75, 2.4, 3.5, 4, 5.5),  
#endif
#endif

#if (TEST_NAPP)
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "napp", true, "numPivot=8,numPivotIndex=8,chunkIndexSize=102", "numPivotSearch=8",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "napp", true, "numPivot=8,numPivotIndex=8,chunkIndexSize=102", "numPivotSearch=8",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "napp", true, "numPivot=32,numPivotIndex=8,chunkIndexSize=102", "numPivotSearch=8",
                1 /* KNN-1 */, 0 /* no range search */ , 0.6, 0.8, 2.0, 3.7, 20, 33),
#endif


#if (TEST_OTHER)
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "nonmetr_list_clust", false, "clusterType=clarans,centerQty=10", "dbScanFrac=0.1", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.85, 0.95, 0.01, 5, 2, 7),  
  // ************** Tests for non-metric clustering *********** //
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "nonmetr_list_clust", false, "clusterType=firmal,centerQty=10", "dbScanFrac=0.1", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.8, 0.92, 0.1, 20, 2.5, 6),  
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "nonmetr_list_clust", false, "clusterType=reduct_clarans,centerQty=10", "dbScanFrac=0.1", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.85, 0.95, 0.01, 5, 2, 7),  

  // *************** NEW versions of permutation & projection-based filtering method tests ******************** //
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "seq_search", false, "", "",
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0, 0, 1, 1),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "seq_search", false, "", "",
                0 /* no-knn search */, 0.2 /* range 0.2 */ , 1.0, 1.0, 0, 0, 1, 1),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "seq_search", false, "multiThread=1,threadQty=4", "",
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0, 0, 1, 1),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "seq_search", false, "multiThread=1,threadQty=4", "",
                0 /* no-knn search */, 0.2 /* range 0.2 */ , 1.0, 1.0, 0, 0, 1, 1),  

  // 4 different types of projections
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=perm,projDim=4", "dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=rand,projDim=4", "dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=fastmap,projDim=4", "dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=randrefpt,projDim=4", "dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  

  // Proj. VP-tree
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_vptree", false, "projType=perm,projDim=4", "alphaLeft=2,alphaRight=2,dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  

  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt","pp-index", false, "numPivot=4,prefixLength=4", "minCandidate=10000",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mi-file", false, "numPivot=16,numPivotIndex=16", "numPivotSearch=16,dbScanFrac=1.0",

                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  

  // Binarized permutations
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "perm_incsort_bin", false, "numPivot=32", "dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "perm_bin_vptree", false, "numPivot=32", "alphaLeft=2,alphaRight=2,dbScanFrac=1.0",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0, 0.01, 0.99, 1.01),  


  // 4 different types of projections
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=perm,projDim=4", "dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.4, 0.7, 0.5, 4, 8, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=rand,projDim=4", "dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.01, 0.0, 0.2, 8, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=fastmap,projDim=4", "dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.01, 0.0, 0.2, 8, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_incsort", false, "projType=randrefpt,projDim=4", "dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.01, 0.0, 0.2, 8, 12),  

  // Proj. VP-tree
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "proj_vptree", false, "projType=perm,projDim=4", "alphaLeft=2,alphaRight=2,dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.4, 0.7, 0.5, 4.2, 8, 12),  

  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt","pp-index", false, "numPivot=4,prefixLength=4", "minCandidate=100",
                1 /* KNN-1 */, 0 /* no range search */ , 0.8, 1.0, 0.1, 2, 3, 8),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mi-file", false, "numPivot=16,numPivotIndex=16", "numPivotSearch=16,dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.95, 1.0, 0, 0.5, 8, 12),  

  // Binarized
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "perm_incsort_bin", false, "numPivot=32", "dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.0, 0.01, 0.3, 8, 12),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "perm_bin_vptree", false, "numPivot=32", "alphaLeft=2,alphaRight=2,dbScanFrac=0.1",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.0, 0.01, 0.5, 8, 12),  


  // *************** omedrank tests ******************** //

  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "omedrank", false, "numPivot=4,chunkIndexSize=16536", "dbScanFrac=0.01,minFreq=0.5",
                1 /* KNN-1 */, 0 /* no range search */ , 0.7, 0.97, 0.1, 3, 70, 120),  
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "omedrank", false, "numPivot=4,chunkIndexSize=16536", "dbScanFrac=0.01,minFreq=0.5",
                1 /* KNN-1 */, 0 /* no range search */ , 0.6, 0.9, 0.1, 3, 70, 120),  

  // *************** VP-tree tests ******************** //
  // knn
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10",  "",
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 40, 80),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "alphaLeft=2,alphaRight=2", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.93, 0.97, 0.03, 0.09, 120, 190),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final128_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "",
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 1.5, 2.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final128_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "alphaLeft=2,alphaRight=2", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.98, 1.0, 0.0, 0.02, 2.8, 5.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "",
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 20, 30),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "alphaLeft=2,alphaRight=2", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.93, 0.96, 0.0, 0.02, 56, 80),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final128_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "",
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 1.1, 1.6),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final128_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "alphaLeft=2,alphaRight=2", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.98, 0.999, 0.0, 0.01, 1.5, 2.5),  
  // range
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "",
                0 /* no KNN */, 0.1 /* range search radius 0.1 */ , 1.0, 1.0, 0.0, 0.0, 23, 30),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "vptree", false, "chunkBucket=1,bucketSize=10", "",
                0 /* no KNN */, 0.5 /* range search radius 0.5 */ , 1.0, 1.0, 0.0, 0.0, 2.4, 4),  

  // *************** MVP-tree tests ******************** //
  // knn
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mvptree", false, "maxPathLen=4,bucketSize=10", "",
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 100, 140),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mvptree", false, "maxPathLen=4,bucketSize=10", "",
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 40, 50),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mvptree", false, "maxPathLen=4,bucketSize=10", "maxLeavesToVisit=10", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.82, 0.9, 0.2, 3.5, 210, 250),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mvptree", false, "maxPathLen=4,bucketSize=10", "maxLeavesToVisit=20", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.75, 0.82, 0.2, 2.0, 85, 100),  

  // range
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mvptree", false, "maxPathLen=4,bucketSize=10",  "",
                0 /* no KNN */, 0.1 /* range search radius 0.1 */ , 1.0, 1.0, 0.0, 0.0, 40, 55),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "mvptree", false, "maxPathLen=4,bucketSize=10",  "",
                0 /* no KNN */, 0.5 /* range search radius 0.5*/ , 1.0, 1.0, 0.0, 0.0, 3, 4),  

  // *************** GH-tree tests ******************** //
  // knn
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "ghtree", false, "bucketSize=10",  "",
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 25, 35),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "ghtree", false, "bucketSize=10",  "",
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 8, 10.2),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "ghtree", false, "bucketSize=10", "maxLeavesToVisit=10", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.8, 0.87, 0.2, 1.5, 95, 115),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "ghtree", false, "bucketSize=10", "maxLeavesToVisit=20", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.75, 0.82, 0.1, 1.0, 52, 62),  
  // range
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "ghtree", false, "bucketSize=10", "", 
                0 /* no KNN */, 0.1 /* range search radius 0.1 */ , 1.0, 1.0, 0.0, 0.0, 10, 16),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "ghtree", false, "bucketSize=10", "",
                0 /* no KNN */, 0.5 /* range search radius 0.5*/ , 1.0, 1.0, 0.0, 0.0, 1, 1.2),  

  // *************** SA-tree tests ******************** //
  // knn
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "satree", false, "bucketSize=10", "", 
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 20, 33),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "satree", false, "bucketSize=10", "", 
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 10, 25),  
  // range
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "satree", false, "bucketSize=10", "", 
                0 /* no KNN */, 0.1 /* range search radius 0.1 */ , 1.0, 1.0, 0.0, 0.0, 13, 18),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "satree", false, "bucketSize=10", "", 
                0 /* no KNN */, 0.5 /* range search radius 0.5*/ , 1.0, 1.0, 0.0, 0.0, 2.8, 3.4),  

  // *************** List of clusters tests ******************** //
  // knn
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "list_clusters", false, "strategy=random,useBucketSize=1,bucketSize=10", "", 
                1 /* KNN-1 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 9.5, 11.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "list_clusters", false, "strategy=random,useBucketSize=1,bucketSize=10", "", 
                10 /* KNN-10 */, 0 /* no range search */ , 1.0, 1.0, 0.0, 0.0, 7.5, 8.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "list_clusters", false, "strategy=random,useBucketSize=1,bucketSize=10", "maxLeavesToVisit=10", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.78, 0.9, 0.2, 1.5, 9.5, 11.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "list_clusters", false, "strategy=random,useBucketSize=1,bucketSize=10", "maxLeavesToVisit=20", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.85, 0.97, 0.05, 0.7, 8.5, 10.5),  
  // range
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "list_clusters", false, "strategy=random,useBucketSize=1,bucketSize=10", "",
                0 /* no KNN */, 0.1 /* range search radius 0.1 */ , 1.0, 1.0, 0.0, 0.0, 8, 10),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "list_clusters", false, "strategy=random,useBucketSize=1,bucketSize=10", "",
                0 /* no KNN */, 0.5 /* range search radius 0.5*/ , 1.0, 1.0, 0.0, 0.0, 2.4, 3.4),  

#ifdef WITH_EXTRAS
  // *************** bbtree tests ******************** //
  // knn
  /* 
   * TODO @leo 
   *      bbtree seems to be a bit wacky (missing a tiny fraction of answers), 
   *      need to debug it in the future.
   *      Therefore, we expect a slightly imperfect recall sometimes.
   */
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "bbtree", false, "bucketSize=10", "",
                1 /* KNN-1 */, 0 /* no range search */ , 0.999, 1.0, 0.0, 0.0, 9.5, 11.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "bbtree", false, "bucketSize=10", "",
                10 /* KNN-10 */, 0 /* no range search */ , 0.999, 1.0, 0.0, 0.0, 5.5, 8),  
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "bbtree", false, "bucketSize=10", "maxLeavesToVisit=10", 
                1 /* KNN-1 */, 0 /* no range search */ , 0.75, 0.85, 0.3, 1.6, 45, 55),  
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "bbtree", false, "bucketSize=10", "maxLeavesToVisit=20", 
                10 /* KNN-10 */, 0 /* no range search */ , 0.7, 0.78, 0.3, 1.6, 28, 37),  
  // range
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "bbtree", false, "bucketSize=10", "",
                0 /* no KNN */, 0.1 /* range search radius 0.1 */ , 0.999, 1.0, 0.0, 0.0, 4.5, 6.5),  
  MethodTestCase(DIST_TYPE_FLOAT, "kldivgenfast", "final8_10K.txt", "bbtree", false, "bucketSize=10", "",
                0 /* no KNN */, 0.5 /* range search radius 0.5*/ , 0.999, 1.0, 0.0, 0.0, 1.2, 2.4),  


  // *************** multi-probe LSH tests ******************** //
  // knn
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "lsh_multiprobe", false, "desiredRecall=0.5,tuneK=1,T=5,L=25,H=16535", "",
                1 /* KNN-1 */, 0 /* no range search */ , 0.45, 0.6, 45, 80, 70, 130),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "lsh_multiprobe", false, "desiredRecall=0.5,tuneK=10,T=5,L=25,H=16535", "",
                10 /* KNN-10 */, 0 /* no range search */ , 0.45, 0.6, 10, 40, 70, 130),  
  // *************** Guassian LSH tests ******************** //
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "lsh_gaussian", false, "W=2,L=5,M=40,H=16535", "",

                1 /* KNN-1 */, 0 /* no range search */ , 0.85, 0.95, 0.1, 40, 70, 130),  
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "lsh_gaussian", false, "W=2,L=5,M=40,H=16535", "",

                10 /* KNN-10 */, 0 /* no range search */ , 0.68, 0.82, 0.1, 50, 70, 130),  
  // *************** Cauchy LSH tests ******************** //
  MethodTestCase(DIST_TYPE_FLOAT, "l1", "final8_10K.txt", "lsh_cauchy", false, "W=2,L=5,M=10,H=16535", "",
                1 /* KNN-1 */, 0 /* no range search */ , 0.7, 0.9, 0.1, 50, 70, 130),  
  MethodTestCase(DIST_TYPE_FLOAT, "l1", "final8_10K.txt", "lsh_cauchy", false, "W=2,L=5,M=10,H=16535", "",
                10 /* KNN-10 */, 0 /* no range search */ , 0.5, 0.8, 0.1, 50, 70, 120),  
  // *************** Thresholding LSH tests ******************** //
  MethodTestCase(DIST_TYPE_FLOAT, "l1", "final8_10K.txt", "lsh_threshold", false, "L=5,M=60,H=16535", "",
                1 /* KNN-1 */, 0 /* no range search */ , 0.8, 0.99, 0.1, 50, 40, 70),  
  MethodTestCase(DIST_TYPE_FLOAT, "l1", "final8_10K.txt", "lsh_threshold", false, "L=5,M=60,H=16535", "",
                10 /* KNN-10 */, 0 /* no range search */ , 0.65, 0.85, 0.1, 50, 40, 70),  
  // Old NN-descent
  MethodTestCase(DIST_TYPE_FLOAT, "l2", "final8_10K.txt", "nndes", false, "NN=10,rho=0.5,delta=0.001", "initSearchAttempts=10",
                1 /* KNN-1 */, 0 /* no range search */ , 0.9, 1.0, 0, 1.0, 5, 12),  
#endif

#endif
};


int main(int ac, char* av[]) {
  // This should be the first function called before
  string LogFile;
  if (ac == 2) LogFile = av[1];

  initLibrary(0, LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  WallClockTimer timer;
  timer.reset();

  set<string>           setDistType;
  set<string>           setSpaceType;
  set<string>           setDataSet;
  set<unsigned>         setKNN;
  set<float>            setRange;

  for (const auto& testCase: vTestCaseDesc) {
    setDistType.insert(testCase.distType_);
    setSpaceType.insert(testCase.spaceType_);
    setDataSet.insert(testCase.dataSet_);
    if (testCase.mKNN > 0)
      setKNN.insert(testCase.mKNN);
    if (testCase.mRange > 0)
      setRange.insert(testCase.mRange);
  };  

  size_t nTest = 0;
  size_t nFail = 0;

  try {
    for (int bTestReload = 0; bTestReload < 2; ++bTestReload) {
      cout << "Testing index reload: " << (bTestReload == 1) << endl;
      cout << "==================================================" << endl;
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
        string dataFile = sampleDataPrefix + dataSet;

        for (unsigned K: setKNN) {
          vector<MethodTestCase>  vTestCases; 

          // Select appropriate test cases to share the same gold-standard data
          for (const auto& testCase: vTestCaseDesc) {
            if ((bTestReload == 0 || testCase.testReload_)  &&
                testCase.dataSet_ == dataSet &&
                testCase.distType_ == distType && 
                testCase.spaceType_ == spaceType &&
                testCase.mKNN  == K) {
              vTestCases.push_back(MethodTestCase(testCase));
            }
          }

          if (!vTestCases.empty())  { // Not all combinations of spaces, data sets, and search types are non-empty
            for (size_t threadQty = 1; threadQty <= MAX_THREAD_QTY; ++threadQty) {
              nTest += vTestCases.size();
              nFail += RunOneTest(vTestCases, 
                                  bTestReload == 1,
                                  INDEX_FILE_NAME,
                                  distType,
                                  spaceType,
                                  threadQty,
                                  TEST_SET_QTY,
                                  dataFile,
                                  "",
                                  0,
                                  MAX_NUM_QUERY,
                                  ConvertToString(K),
                                  0,
                                  ""
                                 );
            }
          }
        }

        for (float R: setRange) {
          vector<MethodTestCase>  vTestCases; 


          // Select appropriate test cases to share the same gold-standard data
          for (const auto& testCase: vTestCaseDesc) {
            if ((bTestReload == 0 || testCase.testReload_)  &&
                testCase.dataSet_ == dataSet &&
                testCase.distType_ == distType && 
                testCase.spaceType_ == spaceType &&
                testCase.mRange  == R) {
              vTestCases.push_back(MethodTestCase(testCase));
            }
          }

          if (!vTestCases.empty())  { // Not all combinations of spaces, data sets, and search types are non-empty
            for (size_t threadQty = 1; threadQty <= MAX_THREAD_QTY; ++threadQty) {
              nTest += vTestCases.size();
              nFail += RunOneTest(vTestCases, 
                                  bTestReload == 1,
                                  INDEX_FILE_NAME,
                                  distType,
                                  spaceType,
                                  threadQty,
                                  TEST_SET_QTY,
                                  dataFile,
                                  "",
                                  0,
                                  MAX_NUM_QUERY,
                                  "",
                                  0,
                                  ConvertToString(R)
                                 );
            }
          }

        }

      }

    }

  } catch (const std::exception& e) {
    cout << red << "Failure to test due to exception: " << e.what() << no_color << endl;
    ++nFail;
  } catch (...) {
    ++nFail;
    cout << red << "Failure to test due to unknown exception: " << no_color << endl;
  }

  timer.split();

  LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();

  cout << endl << "==================================================" << endl;
  cout << (nFail ? "FAILURE" : "SUCCESS") << endl;
  cout << "Carried out: " << nTest << "  tests. Failed: " << nFail << " tests" << endl;

  return nFail ? 1:0;
}
