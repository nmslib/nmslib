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
#include <cmath>

#include "space.h"
#include "rangequery.h"
#include "cluster_util.h"
#include "knnquery.h"

#include "method/nonmetr_list_clust.h"

namespace similarity {

template <typename dist_t>
void NonMetrListClust<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);
  string clusterType;
  pmgr.GetParamRequired("clusterType", clusterType);
  size_t centerQty;
  pmgr.GetParamRequired("centerQty", centerQty);

  LOG(LIB_INFO) << "clusterType=" << clusterType;
  LOG(LIB_INFO) << "centerQty=" << centerQty;
  if (clusterType == CLUST_TYPE_FIRMAL) {
    size_t searchCloseIterQty = 500;
    pmgr.GetParamOptional("searchCloseIterQty", searchCloseIterQty, searchCloseIterQty);
    size_t sampleDistQty = SAMPLE_LIST_CLUST_DEFAULT_SAMPLE_QTY;
    pmgr.GetParamOptional("sampleDistQty", sampleDistQty, sampleDistQty);

    LOG(LIB_INFO) << "searchCloseIterQty=" << searchCloseIterQty;
    LOG(LIB_INFO) << "sampleDistQty=" << sampleDistQty;
    ClusterUtils<dist_t>::doFIRMAL(printProgress_, space_, this->data_, centerQty, vCenters_, vClusterAssign_, vUnassigned_,
                                   searchCloseIterQty, sampleDistQty, true /* do use all previous clusters in each iteration */);
  } else if (clusterType == CLUST_TYPE_CLARAN) {
    size_t randRestQty = CLARANS_RAND_RESTART_QTY;
    pmgr.GetParamOptional("randRestQty", randRestQty, randRestQty);
    size_t inClusterSwapAttempts = CLARANS_SWAP_ATTEMPTS;
    pmgr.GetParamOptional("inClusterSwapAttempts", inClusterSwapAttempts, inClusterSwapAttempts);
    size_t inClusterSampleQty = CLARANS_SAMPLE_QTY;
    pmgr.GetParamOptional("inClusterSampleQty", inClusterSampleQty, inClusterSampleQty);

    LOG(LIB_INFO) << "randRestQty=" << randRestQty;
    ClusterUtils<dist_t>::doCLARANS(printProgress_, space_, this->data_, centerQty, vCenters_, vClusterAssign_,
                                    inClusterSwapAttempts, inClusterSampleQty, randRestQty);
  } else if (clusterType == CLUST_TYPE_REDUCT_CLARAN) {
    size_t inClusterSwapAttempts = CLARANS_SWAP_ATTEMPTS;
    pmgr.GetParamOptional("inClusterSwapAttempts", inClusterSwapAttempts, inClusterSwapAttempts);
    size_t inClusterSampleQty = CLARANS_SAMPLE_QTY;
    pmgr.GetParamOptional("inClusterSampleQty", inClusterSampleQty, inClusterSampleQty);

    IdTypeUnsign maxMetaIterQty = MAX_METAITER_CLARANS_ITER_QTY;
    pmgr.GetParamOptional("maxMetaIterQty", maxMetaIterQty, maxMetaIterQty);
    float keepFrac = 0.2;
    pmgr.GetParamOptional("keepFrac", keepFrac, keepFrac);
    LOG(LIB_INFO) << "maxMetaIterQty = " << maxMetaIterQty;
    LOG(LIB_INFO) << "keepFrac       = " << keepFrac;
    ClusterUtils<dist_t>::doReductiveCLARANS(printProgress_, space_, this->data_,
                                             maxMetaIterQty, keepFrac,
                                             centerQty, vCenters_, vClusterAssign_, vUnassigned_,
                                             inClusterSwapAttempts, inClusterSampleQty);
  } else throw runtime_error("Wrong cluster type, expecting: " + CLUST_TYPE_CLARAN + " or " + CLUST_TYPE_FIRMAL);

  ClusterUtils<dist_t>::printAndVerifyClusterStat(space_, vCenters_, vClusterAssign_, 1000);

  pmgr.CheckUnused();
}

template <typename dist_t>
template <typename QueryType>
void NonMetrListClust<dist_t>::GenericSearch(QueryType* query, IdType) const {
  size_t dbScan = 0;
  vector<bool>  vis(maxObjId_ + 1);
  for (const Object* o:vUnassigned_) {
    if (vis[o->id()]) continue;
    query->CheckAndAddToResult(o);
    vis[o->id()] = true;
    dbScan++;
  }

  const Object* pQueryObj = query->QueryObject();

  if (dbScan < db_scan_) {
    vector<pair<dist_t, IdTypeUnsign>> orderedClusters;

    for (IdTypeUnsign cid = 0; cid < vCenters_.size(); ++cid) {
      dist_t d = query->Distance(pQueryObj /* object or query is the left argument */,
                                 vCenters_[cid]/* cluster center is the right argument */);
      orderedClusters.push_back(make_pair(d, cid));
    }
    sort(orderedClusters.begin(), orderedClusters.end());

    CHECK(orderedClusters.size() < 2 || (orderedClusters[0].first <= orderedClusters[1].first));
    //cerr << "######## @@@";
    for (IdTypeUnsign i = 0; dbScan <= db_scan_ && i < orderedClusters.size(); ++i) {
      IdTypeUnsign cid = orderedClusters[i].second;
      //cerr << " " << orderedClusters[i].first;
      for (const DistObjectPair<dist_t> p: (*vClusterAssign_[cid])) {
        const Object* o = p.second;
        if (vis[o->id()]) continue;
        query->CheckAndAddToResult(o);
        vis[o->id()] = true;
        dbScan++;
      }
    }
    //cerr<<endl;
  }
}

template <typename dist_t>
void NonMetrListClust<dist_t>::Search(RangeQuery<dist_t>* query, IdType id) const {
  GenericSearch(query, id);
}

template <typename dist_t>
void NonMetrListClust<dist_t>::Search(KNNQuery<dist_t>* query, IdType id) const {
  GenericSearch(query, id);
}

template <typename dist_t>
void
NonMetrListClust<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  // Check if a user specified extra parameters, which can be also misspelled variants of existing ones
  AnyParamManager pmgr(QueryTimeParams);
  float dbScanFrac;
  // Note that GetParamOptional() should always have a default value
  pmgr.GetParamOptional("dbScanFrac", dbScanFrac, 0.1);
  CHECK_MSG(dbScanFrac > 0 && dbScanFrac <= 1, "dbScanFrac should be >0 and <=1");
  db_scan_ = size_t(ceil(dbScanFrac * this->data_.size()));
  LOG(LIB_INFO) << "db_scan=" << db_scan_;
  pmgr.CheckUnused();
}


template class NonMetrListClust<float>;
template class NonMetrListClust<double>;
template class NonMetrListClust<int>;

}
