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
#ifndef CLUSTER_UTIL
#define CLUSTER_UTIL

#include <space.h>
#include <idtype.h>
#include <object.h>

#include <vector>
#include <string>

namespace similarity {

  const size_t MAX_CLARANS_ITER_QTY     = 1000; // 1000 is quite a lot, usually an algorithm converges locally in a dozen of attempts
  const size_t MAX_METAITER_CLARANS_ITER_QTY = 20; // This is a maximum number of iterations for a single meta-iteration
  const size_t CLARANS_SWAP_ATTEMPTS    = 20;
  const size_t CLARANS_SAMPLE_QTY       = 10;
  const size_t CLARANS_RAND_RESTART_QTY = 5;
  const size_t REDUCTIVE_CLARANS_MIN_CLUSTER_SIZE = 5;

  const size_t SAMPLE_LIST_CLUST_DEFAULT_SAMPLE_QTY = 1000000;

  using std::vector;
  using std::string;

  const string CLUST_TYPE_CLARAN         = "clarans";
  const string CLUST_TYPE_REDUCT_CLARAN  = "reduct_clarans";
  const string CLUST_TYPE_FIRMAL         = "firmal";

  template <typename dist_t>
  class ClusterUtils {
  public:

    /*
     * IMPORTANT NOTE on all clustring methods:
     * vClustAssign is always sorted by the distance to the cluster in the ascending order.
     */

    /*
     * A variant of the k-medoid clustering called CLARANS. First described in:
     *
     * Raymond T. Ng and Jiawei Han. 1994.
     * Efficient and Effective Clustering Methods for Spatial Data Mining.
     * In Proceedings of the 20th International Conference on Very Large Data Bases (VLDB '94)
     */
    static void doCLARANS(bool PrintProgress,
                          const Space<dist_t>&   space,     // A space object
                          ObjectVector           data,     // A list of objects, it is copied
                          size_t                 centerQty, // Number of clusters/centers
                          ObjectVector&          vCenters,  // Centers
                          vector<shared_ptr<DistObjectPairVector<dist_t>>>&  vClusterAssign, // Cluster assignment
                          IdTypeUnsign           inClusterSwapAttempts = CLARANS_SWAP_ATTEMPTS, // Number of random swaps to find a better cluster center
                          IdTypeUnsign           inClusterSampleQty    = CLARANS_SAMPLE_QTY, // Number of random points to estimate if the swap was beneficial
                          size_t                 randRestQty           = CLARANS_RAND_RESTART_QTY,// A number of random restarts
                          IdTypeUnsign           maxIterQty            = MAX_CLARANS_ITER_QTY, // A maximum number of iterations
                          double                 errMinDiff            = 1e-4  // Stop iterating if the total configuration
        // cost doesn't decrease by at least this value
    );

    /*
     * CLARANS applied to ever-diminishing chunks of data. Processing is split into meta-iterations.
     * In one meta-iteration, we carry out a few (<=maxIterQty) CLARANS iterations.
     * Then, we keep only some of the points (namely keepFrac*data.size()) and cluster the remaining ones
     * recursively. Unlike regular CLARANS, the number of random restarts is 1.
     * We carry out at most maxMetaIterQty iterations. Some points remain unassigned.
     */
    static void doReductiveCLARANS(bool PrintProgress,
                                  const Space<dist_t>&   space,     // A space object
                                  ObjectVector           data,     // A list of objects, it is copied
                                  IdTypeUnsign           maxMetaIterQty, // A maximum number of meta iterations
                                  float                  keepFrac, // Percentage of assigned points kept after a meta-iteration is finished
                                  size_t                 centerQty, // Number of clusters/centers
                                  ObjectVector&vCentersGlobal,  // Centers
                                  vector<shared_ptr<DistObjectPairVector<dist_t>>>&  vClusterAssign, // Cluster assignment
                                  ObjectVector&          vUnassigned,    // Unassigned points
                                  IdTypeUnsign           inClusterSwapAttempts = CLARANS_SWAP_ATTEMPTS, // Number of random swaps to find a better cluster center
                                  IdTypeUnsign           inClusterSampleQty    = CLARANS_SAMPLE_QTY, // Number of random points to estimate if the swap was beneficial
                                  IdTypeUnsign           maxIterQty            = MAX_METAITER_CLARANS_ITER_QTY, // A maximum number of iterations in one meta-iteration
                                  double                 errMinDiff            = 1e-4  // Stop iterating if the total configuration
        // cost doesn't decrease by at least this value
    );

    // FIRMAL: fixed radius multi-attempt linkage
    static void doFIRMAL(bool PrintProgress,
                                 const Space<dist_t>&  space,          // A space object
                                 ObjectVector           data,           // A list of objects, it is copied
                                 float                  ExpCenterQty,   // A hint for the number of clusters, the acutal # fo clusters can be much longer
                                 ObjectVector&          vCenters,       // Centers
                                 vector<shared_ptr<DistObjectPairVector<dist_t>>>&  vClusterAssign, // Cluster assignment
                                 ObjectVector&          vUnassigned,    // Unassigned points
                                 size_t                 SearchCloseIterQty, // A # of search iterations to find a point that is close to already selected centers
                                                                        // For good performance it should be in the order of sqrt(data.size())
                                 size_t                 SampleDistQty = SAMPLE_LIST_CLUST_DEFAULT_SAMPLE_QTY,
                                 bool                   bUseAllClustersInIter = true
    );

    // Sort data points inside each cluster in the order of increasing distance to the cluster center.
    static void sortInsideClustersDist(const vector<shared_ptr<DistObjectPairVector<dist_t>>>& vClusterAssign);

    static void printClusterStat(const Space<dist_t>&         space,
                                 const vector<shared_ptr<DistObjectPairVector<dist_t>>>&vClustAssign, // Cluster assignment
                                 IdTypeUnsign                 sampleQty // a number of point to sample in each cluster

    );
    static void printAndVerifyClusterStat(const Space<dist_t>&         space,
                                 const ObjectVector& vCenters,
                                 const vector<shared_ptr<DistObjectPairVector<dist_t>>>&vClustAssign, // Cluster assignment
                                 IdTypeUnsign                 sampleQty // a number of point to sample in each cluster
    );

  };
}   // namespace similarity

#endif

