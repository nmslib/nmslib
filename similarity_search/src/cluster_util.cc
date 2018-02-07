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
#include <algorithm>
#include <random>
#include <cmath>
#include <unordered_map>
#include <functional>
#include <cluster_util.h>
#include <utils.h>
#include <ported_boost_progress.h>

#define PARANOID_CHECK

namespace similarity {

using namespace std;

template <typename dist_t>
struct DataWrapper {
  DataWrapper(IdTypeUnsign dataId, IdType centerIndex, const Object* pCenter, dist_t dist) :
      dataId_(dataId), centerIndex_(centerIndex), pCenter_(pCenter), dist_(dist) { }
  DataWrapper(){}

  IdTypeUnsign  dataId_ = 0;
  IdType        centerIndex_ = -1;
  const Object* pCenter_ = NULL;
  dist_t        dist_ = 0;
};

using namespace std;
const size_t MIN_ITER_PROGRESS_QTY = 1000; // A minimum # of assigned items that we expect to see in a single iteration

const float  MAX_UNASSIGN_FRACT    = 0.02;

const bool PRINT_FIRMAL_DEBUG = false;
const bool PRINT_CLARANS_DEBUG = false;


template <typename dist_t>
void ClusterUtils<dist_t>::doFIRMAL(bool PrintProgress,
                                    const Space<dist_t>&   space,          // A space object
                                    ObjectVector           data,           // A list of objects, it is copied
                                    float                  ExpCenterQty,   // A hint for the number of clusters, the acutal # fo clusters can be much longer
                                    ObjectVector&          vCenters,       // Centers
                                    vector<shared_ptr<DistObjectPairVector<dist_t>>>&  vClustAssign, // Cluster assignment
                                    ObjectVector&          vUnassigned,
                                    size_t                 SearchCloseIterQty,// A # of search iterations to find a point that is close to already selected centers
    // For good performance it should be in the order of sqrt(data.size())
                                    size_t                 SampleDistQty,   // A # of samples to determine the distribution of distances
                                    bool                   bUseAllClustersInIter // If true the point is compared against all previously created clusters
) {
  std::mt19937  randGen;
  std::shuffle(data.begin(), data.end(), randGen);

  vUnassigned.clear();

  LOG(LIB_INFO) << "ExpCenterQty          = " << ExpCenterQty;
  LOG(LIB_INFO) << "SearchCloseIterQty    = " << SearchCloseIterQty;
  LOG(LIB_INFO) << "SampleDistQty         = " << SampleDistQty;

  CHECK_MSG(SampleDistQty > 0, "The number of samples shouldn't be zero!");

  if (data.empty()) return;
  CHECK_MSG(SampleDistQty > 0, "The number of samples should be > 0!");
  CHECK_MSG(ExpCenterQty >= 2, "There should be at least two centers!");

  vCenters.clear();
  vClustAssign.clear();

  unique_ptr<ProgressDisplay> progress_bar;

  if (PrintProgress) {
    LOG(LIB_INFO) << "Sampling progress: ";
    progress_bar.reset(new ProgressDisplay(SampleDistQty, cerr));
  }

  vector<dist_t> dists;

  for (size_t i = 0; i < SampleDistQty; ++i) {
    IdType id1 = RandomInt() % data.size();
    IdType id2 = RandomInt() % data.size();
    dist_t dist = space.IndexTimeDistance(data[id1], data[id2]);
    dists.push_back(dist);
    if (progress_bar.get()) (*progress_bar) += 1;
  }
  sort (dists.begin(), dists.end());

  float ExpClustSize = round(min<float>(max<float>(1, SampleDistQty / ExpCenterQty), SampleDistQty - 1));
  size_t pctPos = size_t(ExpClustSize);
  CHECK(pctPos < dists.size());
  float R = dists[pctPos];

  LOG(LIB_INFO) << "Expected center #: " << ExpCenterQty << " Expected in sample pos (for percentiles only): " << ExpClustSize;
  LOG(LIB_INFO) << "Sampled distances: [" << dists[0] << "," << dists.back() << "] ";
  CHECK(!dists.empty());
  LOG(LIB_INFO) << "5%/50%/100% percentiles: ["
  << dists[min(dists.size()-1,static_cast<size_t>(dists.size()*0.05))] << ","
  << dists[min(dists.size()-1,static_cast<size_t>(dists.size()*0.5))]  << ","
  << dists[min(dists.size()-1,static_cast<size_t>(dists.size()*0.95))] << "]";
  LOG(LIB_INFO) << "R =" << R << ", this represents " << pctPos << "th entry";

  if (PrintProgress) {
    cerr << "Center-selection progress: ";
    progress_bar.reset(new ProgressDisplay(data.size(), cerr));
  }

  if (data.empty()) return;
  vector<DataWrapper<dist_t>>            dataArr(data.size());
  for (IdTypeUnsign did = 0; did < data.size(); ++did) {
    dataArr[did] = DataWrapper<dist_t>({did, -1, NULL, 0});
  }
  /*
   * In the loop we move assigned objects, including new cluster centers to the beginning of the data array
   * and the temporarily unassigned objects (center candidates) to the end of the array. Clusters, will be
   * collected in the end of the array.
   *
   * The structure of the array:
   * [ assigned ] [ not yet processed ] [not assigned in the current iteration ] [ current iteration clusters ] [ previous iteration clusters ]
   * The number of assigned points is: assignedQty
   * The last index of not yet processed is lastUnprocInIterPlus1 - 1
   * The last index of the not yet assigned in the current iteration is lastCluster - 1
   * The last index of the current iteration clusters is data.size() - iterStartCent - 1
   */
  CHECK(!data.empty());
  IdTypeUnsign lastCluster = data.size();

  IdTypeUnsign stopQty = (IdTypeUnsign)data.size() * 0.01;
  IdTypeUnsign assignedQty = 0, iterNum = 0;

  vector<const Object*> vTmpClustCenter;
  vector<IdTypeUnsign > vTmpClustQty;

  IdTypeUnsign lastUnprocInIterPlus1 = data.size();

  CHECK(stopQty > 0);

  size_t prevAssignQty = 0;

  while (lastCluster > assignedQty && assignedQty < (1-MAX_UNASSIGN_FRACT)*data.size()) {
    ++iterNum;

    IdTypeUnsign  trueCenterQty = vTmpClustCenter.size();
    if (assignedQty - prevAssignQty < MIN_ITER_PROGRESS_QTY) {
      if (PRINT_FIRMAL_DEBUG) {
        LOG(LIB_INFO) << "The progress in the previous iteration is too small, let's try to increase the radius";
      }
      if (pctPos >= dists.size()/2) {
        LOG(LIB_INFO) << "Cannot further increase radius, exiting (pctPos == dists.size())";
        goto fin;
      }

      dist_t oldR = R;
      pctPos = min<IdTypeUnsign>(pctPos*2, min(dists.size()/2, dists.size()- 1));
      R = dists[pctPos];
      if (PRINT_FIRMAL_DEBUG) {
        LOG(LIB_INFO) << "Increasing radius from " << oldR << " to " << R;
      }
    }
    prevAssignQty = assignedQty;

    if (PRINT_FIRMAL_DEBUG)
      LOG(LIB_INFO) << "Iteration: " << iterNum << " assignedQty=" << assignedQty <<
      " centerQty=" << trueCenterQty << " R=" << R << " total centerQty=" << vTmpClustCenter.size();

    IdTypeUnsign iterStartCent = vTmpClustCenter.size();
    IdTypeUnsign lastClusterIterStart = lastCluster;

    CHECK(lastCluster > 1);
    {
      // Select a random cluster in the range [assignedQty, lastCluster)
      IdTypeUnsign cid = assignedQty + RandomInt() % (lastCluster - assignedQty);

      dataArr[cid].pCenter_     = data[dataArr[cid].dataId_];
      dataArr[cid].centerIndex_ = vTmpClustCenter.size();
      vTmpClustCenter.push_back(dataArr[cid].pCenter_);
      if (PRINT_FIRMAL_DEBUG)
        LOG(LIB_INFO) << "New cluster candidate: objId=" << dataArr[cid].pCenter_->id();
      vTmpClustQty.push_back(0);

      --lastCluster;
      swap(dataArr[cid], dataArr[lastCluster]); // Becomes a new cluster
    }

    lastUnprocInIterPlus1 = lastCluster;

    IdTypeUnsign batchUnassignQty = 0;
    while (assignedQty < lastUnprocInIterPlus1) {
      if (batchUnassignQty < SearchCloseIterQty) {
        IdTypeUnsign randUnAssignId = assignedQty + RandomInt() % (lastUnprocInIterPlus1 - assignedQty); // Sample among yet unprocessed
        // Ok here we found a non-yet-visited node, check if it can be attached to one of the existing clusters
        IdType closestCenterIndex = -1;
        dist_t minDist = numeric_limits<dist_t>::max();
        /*
         * Here, we start from iterStartCent, because all the points "had a chance" to be connected to these clusters.
         * In additions, all vTmpClustCenter elements are non-NULL for the current iteration, but this is not true
         * for clusters from previous iterations. Specifically, when we find empty clusters in the end of the iteration,
         * we NULLify respective elements of vTmpClustCenter and return cluster centers into the game (so that they
         * can be linked/clustered in subsequent iterations).
         * An alternative version where we start from zero seems to be too slow (ALSO don't forget that some clusters may be NULLs)
         */
        CHECK(!vTmpClustCenter.empty());
        for (IdTypeUnsign centerId = bUseAllClustersInIter ? 0 : iterStartCent; centerId < vTmpClustCenter.size(); ++centerId) {
          CHECK(vTmpClustCenter[centerId]!=NULL);
          dist_t dist = space.IndexTimeDistance(data[dataArr[randUnAssignId].dataId_] /* object or query is the left argument */,
                                                vTmpClustCenter[centerId] /* cluster center is the right argument */);
          if (dist <= R && dist <= minDist) {
            minDist = dist;
            closestCenterIndex = centerId;
            // Don't break we may find a better cluster assignment
            // break;
          }
        }

        if (closestCenterIndex >= 0) {
          if (progress_bar.get()) *progress_bar += 1;
          vTmpClustQty[closestCenterIndex]++;
          dataArr[randUnAssignId].pCenter_   = vTmpClustCenter[closestCenterIndex];
          dataArr[randUnAssignId].centerIndex_ = -2; // This marks an assigned data point
          dataArr[randUnAssignId].dist_ = minDist;
          if (PRINT_FIRMAL_DEBUG) {
            LOG(LIB_INFO) << "Excluded cid=" << randUnAssignId << " (regular attach) "
            << " dataId=" << dataArr[randUnAssignId].dataId_
            << " objId=" << data[dataArr[randUnAssignId].dataId_]->id()
            << " dist = " << dataArr[randUnAssignId].dist_
            << " cluster center objId=" << dataArr[randUnAssignId].pCenter_->id()
            << " center qty: " << vTmpClustCenter.size() << "# of centers in iteration ("<<iterNum<<"): " << (vTmpClustCenter.size() - iterStartCent)
            << " assignedQty: " << assignedQty;
          }
          swap(dataArr[randUnAssignId], dataArr[assignedQty]); // Moving the assigned entity to the beginning of the queue
          ++assignedQty;
          batchUnassignQty = 0;
          continue;
        } else {
          CHECK(lastUnprocInIterPlus1 > 0); // By design lastUnprocInIterPlus1 > assignedQty >= 0, see the above loop conditions
          CHECK(dataArr[randUnAssignId].centerIndex_ == -1);
          CHECK(dataArr[randUnAssignId].pCenter_ == NULL);
          CHECK(lastUnprocInIterPlus1 > assignedQty);
          swap(dataArr[lastUnprocInIterPlus1 - 1], dataArr[randUnAssignId]);
          lastUnprocInIterPlus1--;
          batchUnassignQty++;
          continue;
        }
      } else {
        /*
         * Let's select a random unassigned point to be a new cluster.
         * After doing so, we also have to check, if this point can become a center
         * of previously unassigned points.
         */
        CHECK(lastUnprocInIterPlus1 < lastCluster);
        /*
        * When the new cluster is selected: lastCluster == lastUnprocInIterPlus1.
        * However, afterwards we make batchUnassignQty < 0 unsuccessful attempts to assign
        * a data points, each of which leads to decrementing lastUnprocInIterPlus1
        */
        IdTypeUnsign cid = lastUnprocInIterPlus1 + RandomInt() % (lastCluster - lastUnprocInIterPlus1);
        --lastCluster;
        IdTypeUnsign  centerId = vTmpClustCenter.size();
        dataArr[cid].centerIndex_ = centerId;
        dataArr[cid].pCenter_ = data[dataArr[cid].dataId_];
        vTmpClustCenter.push_back(dataArr[cid].pCenter_);
        vTmpClustQty.push_back(0);
        if (PRINT_FIRMAL_DEBUG)
          LOG(LIB_INFO) << "New cluster candidate: objId=" << dataArr[cid].pCenter_->id();

        // Now swap
        swap(dataArr[cid], dataArr[lastCluster]);

        for (IdTypeUnsign cid = lastUnprocInIterPlus1; cid < lastCluster; ++cid) {
          CHECK(!vTmpClustCenter.empty());
          dist_t dist = space.IndexTimeDistance(data[dataArr[cid].dataId_] /* object or query is the left argument */,
                                                vTmpClustCenter.back() /* cluster center is the right argument */);
          if (dist <= R) {
            if (progress_bar.get()) *progress_bar += 1;
            dataArr[cid].centerIndex_ = -2; // This marks an assigned data point
            dataArr[cid].pCenter_ = vTmpClustCenter[centerId];
            vTmpClustQty[centerId]++;
            dataArr[cid].dist_ = dist;
            if (PRINT_FIRMAL_DEBUG) {
              LOG(LIB_INFO) << "Excluded cid=" << cid << " (late attach) "
              << " dataId=" << dataArr[cid].dataId_
              << " objId=" << data[dataArr[cid].dataId_]->id()
              << " dist = " << dataArr[cid].dist_
              << " cluster center objId=" << dataArr[cid].pCenter_->id()
              << " center qty: " << (vTmpClustCenter.size()) << "# of centers in iteration ("<<iterNum<< ") "<< (vTmpClustCenter.size() - iterStartCent)
              << " assignedQty: " << assignedQty;
            }
            swap(dataArr[cid], dataArr[assignedQty]);
            ++assignedQty;
          }
        }
        batchUnassignQty=0; // Resetting this value
      }
    }
    /*
     * Let's get rid of empty clusters generated in this iteration!
     * Note that centerIndex_ is only valid in this iteration,
     * however, we are not going to delete clusters generated in previous iterations (with
     * a potentially invalid centerIndex_)
     */

    vector<IdTypeUnsign> vDelIds;
    for (IdTypeUnsign i = lastCluster; i < lastClusterIterStart; ++i) {
      IdType centerIndex = dataArr[i].centerIndex_;
      CHECK(centerIndex >= 0);
      CHECK(centerIndex >= iterStartCent);
      CHECK(vTmpClustCenter[centerIndex] != NULL);
      if (!vTmpClustQty[centerIndex]) {
        CHECK(lastCluster >= assignedQty);
        if (PRINT_FIRMAL_DEBUG) LOG(LIB_INFO) << "Reclaiming a cluster of an empty center! dataId=" <<
              dataArr[i].dataId_ << " objId=" << data[dataArr[i].dataId_]->id();
        dataArr[i].pCenter_ = NULL; // No cluster center any more
        dataArr[i].dist_ = 0;
        dataArr[i].centerIndex_ = -1;
        vDelIds.push_back(centerIndex);
        swap(dataArr[i], dataArr[lastCluster]);
        ++lastCluster;
        CHECK(lastCluster > assignedQty);
      }
    }
    // Don't forget to sort IDs (that we have to delete) in the **DESCENDING** order!
    // otherwise the deletion algorithm will fail miserably
    sort(vDelIds.begin(), vDelIds.end(), std::greater<IdTypeUnsign>());
    CHECK(vDelIds.size() < 2 || (vDelIds[0] > vDelIds[1]));

    for (IdType delId: vDelIds) {
      if (PRINT_FIRMAL_DEBUG)
        LOG(LIB_INFO) << "Actually deleting previously reclaimed cluster, objId=" << vTmpClustCenter[delId]->id();
      vTmpClustCenter.erase(vTmpClustCenter.begin() + delId);
      vTmpClustQty.erase(vTmpClustQty.begin() + delId);
    }
    CHECK(vTmpClustCenter.size() == vTmpClustQty.size());
  }
  fin:
  if (progress_bar.get()) *progress_bar += data.size() - progress_bar->count();

  // Now create the final representation
  unordered_map<const Object*,IdTypeUnsign> ptr2pos;

  for (IdTypeUnsign pos = 0; pos < vTmpClustCenter.size(); ++pos) {
    CHECK(vTmpClustCenter[pos] != NULL);
/*
    if (PRINT_FIRMAL_DEBUG)
      LOG(LIB_INFO) << "Cluster # " << pos << " qty: " << vTmpClustQty[pos] << " objId=" << vTmpClustCenter[pos]->id();
*/
    vCenters.push_back(vTmpClustCenter[pos]);
    vClustAssign.push_back(shared_ptr<DistObjectPairVector<dist_t>>(new DistObjectPairVector<dist_t>()));
    ptr2pos.insert(make_pair(vTmpClustCenter[pos], pos));
  }

  CHECK(vCenters.size() == vClustAssign.size());
  for (IdTypeUnsign i = 0; i < assignedQty; ++i) {
    const DataWrapper<dist_t> o = dataArr[i];
    CHECK(o.centerIndex_ == -2);
    CHECK(o.pCenter_ != NULL);

    CHECK_MSG(o.pCenter_ != NULL, "Bug: encountered an unassigned points, while expecting an assigned one!");
    auto it = ptr2pos.find(o.pCenter_);
    CHECK_MSG(it != ptr2pos.end(),
              "Bug: cannot find an array position of the cluster with objId="+ConvertToString(o.pCenter_->id())+
    " dataId="+ConvertToString(o.dataId_));
    IdTypeUnsign  pos = it->second;
    CHECK(pos < vClustAssign.size());
    vClustAssign[pos]->push_back(make_pair(o.dist_, data[o.dataId_]));
  }

  for (IdTypeUnsign i = assignedQty; i < lastCluster; ++i) {
    const DataWrapper<dist_t> o = dataArr[i];
    CHECK(o.centerIndex_ == -1);
    CHECK(o.pCenter_ == NULL);
    vUnassigned.push_back(data[o.dataId_]);
  }

  LOG(LIB_INFO) << "Created: " << vCenters.size() << " centers in " << iterNum << " iterations";
  LOG(LIB_INFO) << "The number of unassigned data points: " << (data.size() - assignedQty - vCenters.size());
  LOG(LIB_INFO) << "The number of unassigned data points collected: " << vUnassigned.size();

  CHECK(data.size() - assignedQty - vCenters.size() == vUnassigned.size());

  CHECK(vCenters.size() == vClustAssign.size());
  sortInsideClustersDist(vClustAssign);
}

template <typename dist_t>
void ClusterUtils<dist_t>::doCLARANS(bool PrintProgress,
                                     const Space<dist_t>&         space,    // A space object
                                     ObjectVector                 data,     // A list of objects, it is copied
                                     size_t                       centerQty,// Number of clusters/centers
                                     ObjectVector&                vCenters, // Centers
                                     vector<shared_ptr<DistObjectPairVector<dist_t>>>&     vClustAssign, // Cluster assignment
                                     IdTypeUnsign                 inClusterSwapAttempts, // Number of random swaps to find a better cluster center
                                     IdTypeUnsign                 inClusterSampleQty, // Number of random points to estimate if the swap was beneficial
                                     size_t                       randRestQty,// A number of random restarts
                                     IdTypeUnsign                 maxIterQty, // A maximum number of iterations
                                     double                       errMinDiff  // Stop iterating if the total configuration
    // cost doesn't decrease by at least this value
) {
  static std::mt19937       randGen;

  LOG(LIB_INFO) << "centerQty               = " << centerQty;
  LOG(LIB_INFO) << "inClusterSwapAttempts   = " << inClusterSwapAttempts;
  LOG(LIB_INFO) << "inClusterSampleQty      = " << inClusterSampleQty;
  LOG(LIB_INFO) << "randRestQty             = " << randRestQty;
  LOG(LIB_INFO) << "maxIterQty              = " << maxIterQty;
  LOG(LIB_INFO) << "errMinDiff              = " << errMinDiff;

  CHECK_MSG(data.size() >= centerQty,
            "The number of objects is too small, expecting at least the number of centers/clusters! # of clusters/centers: "
            + ConvertToString(centerQty) + " # of data points: " + ConvertToString(data.size()));
  size_t N = data.size();

  double bestConfigCost = numeric_limits<double>::max();

#ifdef PARANOID_CHECK
  {

    IdType MaxId = 0;
    for (const Object *pObj: data) {
      MaxId = max(MaxId, pObj->id());
      CHECK_MSG(pObj->id()>=0, "Got negative Id!");
    }

    // Sanity check, is our data correct?
    {
      vector<bool> seen(MaxId+1);
      for (const Object *pObj: data) {
        CHECK_MSG(!seen[pObj->id()],
                  "Inconsistent data, repeating id: " + ConvertToString(pObj->id()));
        seen[pObj->id()] = true;
      }
    }
  }
#endif

  for (size_t randRestId = 0; randRestId < randRestQty; ++randRestId) {
#ifdef PARANOID_CHECK
    IdType MaxId = 0;
    for (const Object *pObj: data) {
      MaxId = max(MaxId, pObj->id());
    }

    // Sanity check, is our data correct?
    {
      vector<bool> seen(MaxId+1);
      for (const Object *pObj: data) {
        CHECK_MSG(!seen[pObj->id()],
                  "Inconsistent data (before shuffle), repeating id: " + ConvertToString(pObj->id()) + " randRestId=" + ConvertToString(randRestId));
        seen[pObj->id()] = true;
      }
    }
#endif
    std::shuffle(data.begin(), data.end(), randGen);

#ifdef PARANOID_CHECK
    // Sanity check, is our data correct?
    {
      vector<bool> seen(MaxId+1);
      for (const Object *pObj: data) {
        CHECK_MSG(!seen[pObj->id()],
                  "Inconsistent data (after shuffle), repeating id: " + ConvertToString(pObj->id()) + " randRestId=" + ConvertToString(randRestId));
        seen[pObj->id()] = true;
      }
    }
#endif

    LOG(LIB_INFO) << "Found " << centerQty << " random seeds! Random restart id: " << randRestId;

    vector<double> vClustErr(centerQty);

    size_t reserveQty = 4 * float(N) / centerQty;


    vector<vector<pair<dist_t,IdTypeUnsign>>> vClustAssignLoc(centerQty);

    double prevConfCost = numeric_limits<double>::max();

    for (size_t iter = 0; iter < maxIterQty; ++iter) {
      LOG(LIB_INFO) << "Iteration: " << iter << " let's find closest centers!";
      size_t swapQty = 0;
      CHECK(data.size() >= centerQty); // See the check in the beginning of the function

      for (size_t cid = 0; cid < centerQty; ++cid) {
        vClustErr[cid] = 0;
        vClustAssignLoc[cid].clear();
        vClustAssignLoc[cid].reserve(reserveQty);
      }

      double confCost = 0;

      for (size_t did = centerQty; did < N; ++did) {
        dist_t minDist = numeric_limits<dist_t>::max();
        size_t bestClust = 0;
        for (size_t cid = 0; cid < centerQty; ++cid) {
          dist_t d = space.IndexTimeDistance(data[did] /* object or query is the left argument */,
                                             data[cid] /* cluster center is the right argument */);
          if (d < minDist) {
            minDist = d;
            bestClust = cid;
          }
        }

        vClustAssignLoc[bestClust].push_back(make_pair(minDist, did));
        // If a point is selected for cluster-quality estimation
        // incorporate its distance to the center into the error (by adding it)
        if (vClustAssignLoc[bestClust].size() <= inClusterSampleQty)
          vClustErr[bestClust] += minDist;

        confCost += minDist;
      }

      LOG(LIB_INFO) << "All points are assigned to clusters.";
      LOG(LIB_INFO) << "Configuration cost: " << confCost << " previous cost: " << prevConfCost;
      if (iter > 0 && prevConfCost - confCost < errMinDiff) {
        LOG(LIB_ERROR) << "Change in configuration error < " << errMinDiff << " finishing...";
        break;
      }
      if (iter +1 == maxIterQty) {
        LOG(LIB_INFO) << "Maximum # of iterations carried out, exiting.";
        break;
      }
      /*
       * IMPORTANT NOTE: If we have to exit the loop, e.g., because we reached the maximum # of iterations,
       * we must break here, but no further. Subsequent swaps invalidate the distances to centers stored in vClustAsslignLoc[...][...].first
       * Simply because changing the cluster center entails recalculation of all distances to the new center.
       * However, we don't do this until the beginning of the iteration.
       * Hence, if any swaps ocurr, we do not stop until these distances are recalculated. Otherwise, incorrect
       * data will be returned from the function
       */
      if (PRINT_CLARANS_DEBUG) {
        LOG(LIB_INFO) << "Cluster centers: ";
        vector<IdType> tmp;
        for (size_t cid = 0; cid < centerQty; ++cid) {
          tmp.push_back(data[cid]->id());
        }
        LOG(LIB_INFO) << MergeIntoStr(tmp, ' ');
      }
      prevConfCost = confCost;
      for (size_t cid = 0; cid < centerQty; ++cid) {
        // Points are already shuffled
        size_t currClustQty = vClustAssignLoc[cid].size();
        if (currClustQty < inClusterSampleQty) continue; // The cluster is too small

        size_t swapAttempts = min<size_t>(
            currClustQty - inClusterSampleQty, // Doesn't make sense to make more than this # of random swaps
            inClusterSwapAttempts);
        for (size_t att = 0; att < swapAttempts; ++att) {
          IdTypeUnsign randCandPos = inClusterSampleQty +
                                     // This way a new candidate is never the same
                                     // as the one of "samplers"
                                     RandomInt() % (currClustQty - inClusterSampleQty);
          IdTypeUnsign randCandId = vClustAssignLoc[cid][randCandPos].second;
          swap(data[cid], data[randCandId]); // do a swap
          // Compute a new error.
          double newErr = 0;
          CHECK(inClusterSampleQty < vClustAssignLoc[cid].size());
          for (size_t k = 0; k < inClusterSampleQty; ++k) {
            newErr += space.IndexTimeDistance(data[vClustAssignLoc[cid][k].second]/* object or query is the left argument */,
                                              data[cid] /* cluster center is the right argument */);
          }
          if (newErr < vClustErr[cid]) {
            //LOG(LIB_INFO) << vClustErr[cid] << " -> " << newErr;
            vClustErr[cid] = newErr;
            swapQty++;
            //break;
          } else {
            swap(data[cid], data[randCandId]); // do a reversing swap
          }
        }
      }

      /*
       * IMPORTANT NOTE: swaps invalidate the distances to centers stored in vClustAsslignLoc[...][...].first
       * Simply because changing the cluster center entails recalculation of all distances to the new center.
       * However, we don't do this until the beginning of the iteration.
       * Hence, if any swaps ocurr, we do not stop until these distances are recalculated. Otherwise, incorrect
       * data will be returned from the function
       */

      LOG(LIB_INFO) << "The number of swaps in this iteration: " << swapQty;
      if (swapQty == 0) {
        LOG(LIB_INFO) << "No changes, process converged";
        break;
      }
    }

    if (0 == randRestId || prevConfCost < bestConfigCost) {
      LOG(LIB_INFO) << "Found a better configuration: " << prevConfCost << " previous best cost: " << bestConfigCost;
      bestConfigCost = prevConfCost;

#ifdef PARANOID_CHECK
      // Sanity check, if we swapped correctly, all IDs in data should be unique
      {
        vector<bool> seen(MaxId+1);
        for (const Object *pObj: data) {
          CHECK_MSG(!seen[pObj->id()],
                    "Bug, repeating id: " + ConvertToString(pObj->id()));
          seen[pObj->id()] = true;
        }
      }
#endif

      vClustAssign.resize(centerQty);
      for (size_t cid = 0; cid < centerQty; ++cid) {
        vClustAssign[cid].reset(new DistObjectPairVector<dist_t>());
        vClustAssign[cid]->reserve(vClustAssignLoc[cid].size());
        for (auto curr : vClustAssignLoc[cid]) {
          dist_t dist = curr.first;
          IdType id = curr.second;
          vClustAssign[cid]->push_back(make_pair(dist, data[id]));
        }

      }
      vCenters.resize(centerQty);
      for (size_t cid = 0; cid < centerQty; ++cid)
        vCenters[cid] = data[cid];
    } else {
      LOG(LIB_INFO) << "Current configuration's cost: " << prevConfCost << " is worse than the previous best cost: " << bestConfigCost;
    }
  }
}


template <typename dist_t>
void ClusterUtils<dist_t>::doReductiveCLARANS(bool PrintProgress,
                                                  const Space<dist_t>&   space,     // A space object
                                                  ObjectVector           data,     // A list of objects, it is copied
                                                  IdTypeUnsign           maxMetaIterQty,
                                                  float                  keepFrac, // Percentage of assigned points kept after a meta-iteration is finished
                                                  size_t                 centerQty, // Number of clusters/centers
                                                  ObjectVector&          vCentersGlobal,  // Centers
                                                  vector<shared_ptr<DistObjectPairVector<dist_t>>>&  vClusterAssignGlobal, // Cluster assignment
                                                  ObjectVector&          vUnassigned,    // Unassigned points
                                                  IdTypeUnsign           inClusterSwapAttempts, // Number of random swaps to find a better cluster center
                                                  IdTypeUnsign           inClusterSampleQty, // Number of random points to estimate if the swap was beneficial
                                                  IdTypeUnsign           maxIterQty, // A maximum number of iterations in one meta-iteration
                                                  double                 errMinDiff // Stop iterating if the total configuration
                                                                                    // cost doesn't decrease by at least this value
) {
  size_t origDataSize = data.size();

  unique_ptr<ProgressDisplay> progress_bar;

  const size_t SampleDistQty = SAMPLE_LIST_CLUST_DEFAULT_SAMPLE_QTY;
  if (PrintProgress) {
    cerr << "Sampling progress: ";
    progress_bar.reset(new ProgressDisplay(SampleDistQty, cerr));
  }

  vector<dist_t> dists;

  for (size_t i = 0; i < SampleDistQty; ++i) {
    IdType id1 = RandomInt() % data.size();
    IdType id2 = RandomInt() % data.size();
    dist_t dist = space.IndexTimeDistance(data[id1], data[id2]);
    dists.push_back(dist);
    if (progress_bar.get()) (*progress_bar) += 1;
  }
  sort (dists.begin(), dists.end());

  float ExpClustSize = round(min<float>(max<float>(1, SampleDistQty / centerQty), SampleDistQty - 1));
  size_t pctPos = size_t(ExpClustSize);
  CHECK(pctPos < dists.size());
  float R = dists[pctPos];
  LOG(LIB_INFO) << "Sampled distances: [" << dists[0] << "," << dists.back() << "] ";
  CHECK(!dists.empty());
  LOG(LIB_INFO) << "5%/50%/100% percentiles: ["
  << dists[min(dists.size()-1,static_cast<size_t>(dists.size()*0.05))] << ","
  << dists[min(dists.size()-1,static_cast<size_t>(dists.size()*0.5))]  << ","
  << dists[min(dists.size()-1,static_cast<size_t>(dists.size()*0.95))] << "]";
  LOG(LIB_INFO) << "R =" << R << ", this represents " << pctPos << "th entry";


  vUnassigned.clear();

  vCentersGlobal.clear();
  vClusterAssignGlobal.clear();

  LOG(LIB_INFO) << "maxMetaIterQty  = " << maxMetaIterQty;
  LOG(LIB_INFO) << "keepFrac        = " << keepFrac;

  for (IdTypeUnsign metaIter = 0;
       metaIter < maxMetaIterQty && data.size() >= origDataSize * 0.01 ;
//           data.size() >= centerQty * REDUCTIVE_CLARANS_MIN_CLUSTER_SIZE;
       ++metaIter) {

    LOG(LIB_INFO) << "Meta iteration: " << metaIter << " # of data points " << data.size();
    ObjectVector                                      vCentersMetaIter;
    vector<shared_ptr<DistObjectPairVector<dist_t>>>  vClusterAssignMetaIter;

    ClusterUtils<dist_t>::doCLARANS(PrintProgress,
              space,
              data,
              centerQty,
              vCentersMetaIter,
              vClusterAssignMetaIter,
              inClusterSwapAttempts,
              inClusterSampleQty,
               1 /* randRestQty */,
              maxIterQty,
              errMinDiff);


    ObjectVector newData;

    CHECK(vCentersMetaIter.size() == vClusterAssignMetaIter.size());



    for (IdTypeUnsign cid = 0; cid < vCentersMetaIter.size(); ++cid) {
      vCentersGlobal.push_back(vCentersMetaIter[cid]);
      if (metaIter + 1 >= maxMetaIterQty) {
        vClusterAssignGlobal.push_back(vClusterAssignMetaIter[cid]);
      } else {
        size_t keepSize = (size_t) round(keepFrac * vClusterAssignMetaIter[cid]->size());
        shared_ptr<DistObjectPairVector<dist_t>> keepAssign(new DistObjectPairVector<dist_t>());

/*        for (IdTypeUnsign i = 0; i < keepSize; ++i) {
          keepAssign->push_back((*vClusterAssignMetaIter[cid])[i]);
        }
        for (IdTypeUnsign i = keepSize; i < vClusterAssignMetaIter[cid]->size(); ++i) {
          newData.push_back((*vClusterAssignMetaIter[cid])[i].second);
        }*/
        for (IdTypeUnsign i = 0; i < vClusterAssignMetaIter[cid]->size(); ++i) {
          auto e = (*vClusterAssignMetaIter[cid])[i];
          if (e.first < R || i < keepSize) keepAssign->push_back(e);
          else {
            IdType id = e.second->id();
            CHECK(id >= 0);
            newData.push_back(e.second);
          } 
        }

        vClusterAssignGlobal.push_back(keepAssign);
      }
    }

    data.swap(newData);
    CHECK(vCentersGlobal.size() == vClusterAssignGlobal.size());
  }
  // Move what's remained to the unassigned array
  for (const Object* o: data) vUnassigned.push_back(o);

  // Let's do a quick sanity check
  size_t qty = vUnassigned.size();
  for (IdTypeUnsign cid = 0; cid < vCentersGlobal.size(); ++cid) {
    qty += 1 + vClusterAssignGlobal[cid]->size();
  }
  CHECK(qty == origDataSize);
}

template <typename dist_t>
void ClusterUtils<dist_t>::sortInsideClustersDist(const vector<shared_ptr<DistObjectPairVector<dist_t>>>& vClusterAssign) {
  for (const shared_ptr<DistObjectPairVector<dist_t>>& v:vClusterAssign) {
    sort(v->begin(), v->end());
  }
}

template <typename dist_t>
void ClusterUtils<dist_t>::printClusterStat(const Space<dist_t> &space,
                                            const vector<shared_ptr<DistObjectPairVector<dist_t>>>&vClustAssign,
                                            IdTypeUnsign sampleQty) {
  static std::mt19937       randGen;
  for (IdTypeUnsign cid = 0; cid < vClustAssign.size(); ++cid) {
    LOG(LIB_INFO) << "Cluster id: " << cid ;
    vector<dist_t>    dists;

    dists.reserve(sampleQty);
    DistObjectPairVector<dist_t>   vClustElem(*vClustAssign[cid]);
    sort(vClustElem.begin(), vClustElem.end());

    for (IdTypeUnsign i = 0; i < min<size_t>(sampleQty, vClustElem.size()); ++i) {
      dists.push_back(vClustElem[i].first);
    }

    LOG(LIB_INFO) << "# of elements: " << vClustElem.size() ;
    if (!vClustElem.empty())
      LOG(LIB_INFO) << "90% percentile: " << dists[size_t(floor((dists.size() - 1) * 0.9))] ;
    LOG(LIB_INFO) << "Distance quantiles:" ;
    LOG(LIB_INFO) << "[" ;
    if (!dists.empty()) {
      for (size_t i = 0; i < 10; ++i) {
        for (size_t k = 0; k < 10; ++k) {
          auto num = i * 10 + k;
          LOG(LIB_INFO) << num << ":" << dists[size_t(floor((dists.size() - 1) * num / 100.0))] << " ";
        }
        LOG(LIB_INFO) ;
      }
    }
    LOG(LIB_INFO) << "]" ;
  }

}


template <typename dist_t>
void ClusterUtils<dist_t>::printAndVerifyClusterStat(const Space<dist_t> &space, const ObjectVector &vCenters,
                                                     const vector<shared_ptr<DistObjectPairVector<dist_t>>> &vClustAssign,
                                                     IdTypeUnsign sampleQty) {
  //static std::mt19937       randGen;
  for (IdTypeUnsign cid = 0; cid < vClustAssign.size(); ++cid) {
    LOG(LIB_INFO) << "Cluster id: " << cid << " objId=" << vCenters[cid]->id() ;
    vector<dist_t>    dists;

    dists.reserve(sampleQty);
    DistObjectPairVector<dist_t>   vClustElem(*vClustAssign[cid]);
    sort(vClustElem.begin(), vClustElem.end());

    for (IdTypeUnsign i = 0; i < min<size_t>(sampleQty, vClustElem.size()); ++i) {
      dist_t dist = space.IndexTimeDistance(vClustElem[i].second /* object or query is the left argument */,
                                            vCenters[cid] /* cluster center is the right argument */);
      CHECK_MSG(ApproxEqual(vClustElem[i].first, dist),
                "Bug: the precomputed distance: " + ConvertToString(vClustElem[i].first) +
                " is different from the real one: " + ConvertToString(dist) +
                " when computing the distance between " + ConvertToString(vClustElem[i].second->id()) +
                " and " +ConvertToString(vCenters[cid]->id()) );
      dists.push_back(vClustElem[i].first);
    }

    LOG(LIB_INFO) << "# of elements: " << vClustElem.size() ;
    if (!vClustElem.empty())
      LOG(LIB_INFO) << "90% percentile: " << dists[size_t(floor((dists.size() - 1) * 0.9))] ;
    LOG(LIB_INFO) << "Distance quantiles:" ;
    LOG(LIB_INFO) << "[" ;
    if (!dists.empty()) {
      for (size_t i = 0; i < 10; ++i) {
        for (size_t k = 0; k < 10; ++k) {
          auto num = i * 10 + k;
          LOG(LIB_INFO) << num << ":" << dists[size_t(floor((dists.size() - 1) * num / 100.0))] << " ";
        }
        LOG(LIB_INFO) ;
      }
    }
    LOG(LIB_INFO) << "]" ;
  }

}

template class ClusterUtils<float>;
template class ClusterUtils<double>;
template class ClusterUtils<int>;

}   // namespace similarity



