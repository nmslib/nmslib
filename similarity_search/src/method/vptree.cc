/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/nmslib/nmslib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>

#include "portable_prefetch.h"
#if defined(_WIN32) || defined(WIN32)
#include <intrin.h>
#endif

#include "portable_simd.h"
#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "searchoracle.h"
#include "method/vptree.h"
#include "method/vptree_utils.h"
#include "methodfactory.h"

#define MIN_PIVOT_SELECT_DATA_QTY 10
#define MAX_PIVOT_SELECT_ATTEMPTS 5

namespace similarity {

using std::string;
using std::stringstream;
using std::endl;
using std::cout;
using std::cerr;
    
template <typename dist_t, typename SearchOracle>
VPTree<dist_t, SearchOracle>::VPTree(
                       bool  PrintProgress,
                       Space<dist_t>& space,
                       const ObjectVector& data,
                       bool use_random_center) :
                              Index<dist_t>(data),
                              space_(space),
                              PrintProgress_(PrintProgress),
                              use_random_center_(use_random_center),
                              max_pivot_select_attempts_(MAX_PIVOT_SELECT_ATTEMPTS),
                              oracle_(space, data, PrintProgress),
                              QueryTimeParams_(oracle_.GetQueryTimeParamNames()) { 
                                QueryTimeParams_.push_back("maxLeavesToVisit");
                              }

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("bucketSize", BucketSize_, 50);
  pmgr.GetParamOptional("chunkBucket", ChunkBucket_, true);
  pmgr.GetParamOptional("selectPivotAttempts", max_pivot_select_attempts_, MAX_PIVOT_SELECT_ATTEMPTS);

  CHECK_MSG(max_pivot_select_attempts_ >= 1, "selectPivotAttempts should be >=1");

  LOG(LIB_INFO) << "bucketSize          = " << BucketSize_;
  LOG(LIB_INFO) << "chunkBucket         = " << ChunkBucket_;
  LOG(LIB_INFO) << "selectPivotAttempts = " << max_pivot_select_attempts_;

  // Call this function *ONLY AFTER* the bucket size is obtained!
  oracle_.SetIndexTimeParams(pmgr);
  oracle_.LogParams();

  pmgr.CheckUnused();

  this->ResetQueryTimeParams(); // reset query-time parameters

  unique_ptr<ProgressDisplay>   progress_bar(PrintProgress_ ? 
                                              new ProgressDisplay(this->data_.size(), cerr):
                                              NULL);

  root_.reset(new VPNode(0,
                     progress_bar.get(), 
                     oracle_, 
                     space_, this->data_,
                     max_pivot_select_attempts_,
                     BucketSize_, ChunkBucket_,
                     use_random_center_ /* use random center */));

  if (progress_bar) { // make it 100%
    (*progress_bar) += (progress_bar->expected_count() - progress_bar->count());
  }
}

template <typename dist_t,typename SearchOracle>
VPTree<dist_t, SearchOracle>::~VPTree() {
}

template <typename dist_t,typename SearchOracle>
const std::string VPTree<dist_t, SearchOracle>::StrDesc() const {
  return "vptree: " + SearchOracle::GetName();
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::Search(RangeQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::Search(KNNQuery<dist_t>* query, IdType) const {
  int mx = MaxLeavesToVisit_;
  root_->GenericSearch(query, mx);
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::SaveIndex(const std::string& location) {

  std::ofstream output(location, std::ios::binary);
  CHECK_MSG(output, "Cannot open file '" + location + "' for writing");
  output.exceptions(ios::badbit | ios::failbit);

  // Save version number
  const uint32_t version = 1;
  writeBinaryPOD(output, version);

  // Save dataset size, so that we can compare with dataset size upon loading
  writeBinaryPOD(output, this->data_.size());

  // Save tree parameters
  writeBinaryPOD(output, max_pivot_select_attempts_);
  writeBinaryPOD(output, BucketSize_);
  writeBinaryPOD(output, ChunkBucket_);
  writeBinaryPOD(output, use_random_center_);

  // Save node data
  if (root_) {
    SaveNodeData(output, root_.get());
  }
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::LoadIndex(const std::string& location) {

  std::ifstream input(location, std::ios::binary);
  CHECK_MSG(input, "Cannot open file '" + location + "' for reading");
  input.exceptions(ios::badbit | ios::failbit);

  uint32_t version;
  readBinaryPOD(input, version);
  if (version != 1) {
    PREPARE_RUNTIME_ERR(err) << "File version number (" << version << ") differs from "
                             << "expected version (1)";
    THROW_RUNTIME_ERR(err);
  }

  size_t datasize;
  readBinaryPOD(input, datasize);
  if (datasize != this->data_.size()) {
    PREPARE_RUNTIME_ERR(err) << "Saved dataset size (" << datasize
                             << ") differs from actual size (" << this->data_.size() << ")";
    THROW_RUNTIME_ERR(err);
  }

  readBinaryPOD(input, max_pivot_select_attempts_);
  readBinaryPOD(input, BucketSize_);
  readBinaryPOD(input, ChunkBucket_);
  readBinaryPOD(input, use_random_center_);

  root_ = std::unique_ptr<VPNode>(LoadNodeData(input));
}

template <typename dist_t, typename SearchOracle>
void
VPTree<dist_t, SearchOracle>::SaveNodeData(
  std::ofstream& output,
  const VPTree<dist_t, SearchOracle>::VPNode* node) const {

  // Nodes are written to output in pre-order. If a node doesn't have a left/right
  // child, a sentinel value of -1.0 is written instead. Regular nodes are
  // serialized in the following order:
  //
  // * mediandist (always non-negative)
  // * number of bucket elements or 0 if not a leaf node (pivot)
  // * IDs of bucket data, or ID of pivot
  // * left child tree
  // * right child tree

  if (node == nullptr) {
    // write sentinel
    writeBinaryPOD(output, static_cast<float>(-1.0));

    return;
  }

  writeBinaryPOD(output, node->mediandist_);

  if (node->bucket_) {
    writeBinaryPOD(output, node->bucket_->size());
    for (const auto& element : *(node->bucket_)) {
      writeBinaryPOD(output, element->id());
    }
  } else if (node->pivot_) {
    size_t bucketsize = 0;
    writeBinaryPOD(output, bucketsize);
    writeBinaryPOD(output, node->pivot_->id());
  }

  SaveNodeData(output, node->left_child_);
  SaveNodeData(output, node->right_child_);
}

template <typename dist_t, typename SearchOracle>
typename VPTree<dist_t, SearchOracle>::VPNode*
VPTree<dist_t, SearchOracle>::LoadNodeData(std::ifstream& input) const {

  float mediandist;

  readBinaryPOD(input, mediandist);
  if (mediandist < 0) {
    // sentinel node
    return nullptr;
  }

  VPNode* node = new VPNode(oracle_);
  node->mediandist_ = mediandist;

  size_t bucketsize;
  readBinaryPOD(input, bucketsize);

  IdType dataId = 0;
  if (bucketsize == 0) {
    // read one element, the pivot
    readBinaryPOD(input, dataId);
    node->pivot_ = this->data_[dataId];
  } else {
    // read bucket content
    ObjectVector bucket(bucketsize);
    for (size_t i = 0; i < bucketsize; i++) {
      readBinaryPOD(input, dataId);
      bucket[i] = this->data_[dataId];
    }
    node->CreateBucket(false, bucket, nullptr);
  }

  node->left_child_ = LoadNodeData(input);
  node->right_child_ = LoadNodeData(input);

  return node;
}

template <typename dist_t, typename SearchOracle>
void VPTree<dist_t, SearchOracle>::VPNode::CreateBucket(bool ChunkBucket, 
                                                        const ObjectVector& data, 
                                                        ProgressDisplay* progress_bar) {
    if (ChunkBucket) {
      CreateCacheOptimizedBucket(data, CacheOptimizedBucket_, bucket_);
    } else {
      bucket_ = new ObjectVector(data);
    }
    if (progress_bar) (*progress_bar) += data.size();
}

template <typename dist_t, typename SearchOracle>
VPTree<dist_t, SearchOracle>::VPNode::VPNode(const SearchOracle& oracle)
    : oracle_(oracle),
      pivot_(NULL),
      mediandist_(0),
      left_child_(NULL),
      right_child_(NULL),
      bucket_(NULL),
      CacheOptimizedBucket_(NULL) {}

template <typename dist_t, typename SearchOracle>
VPTree<dist_t, SearchOracle>::VPNode::VPNode(
                               unsigned level,
                               ProgressDisplay* progress_bar,
                               const SearchOracle& oracle,
                               const Space<dist_t>& space, const ObjectVector& data,
                               size_t max_pivot_select_attempts,
                               size_t BucketSize, bool ChunkBucket,
                               bool use_random_center)
    : oracle_(oracle),
      pivot_(NULL), mediandist_(0),
      left_child_(NULL), right_child_(NULL),
      bucket_(NULL), CacheOptimizedBucket_(NULL)
{
  CHECK(!data.empty());

  if (!data.empty() && data.size() <= BucketSize) {
    CreateBucket(ChunkBucket, data, progress_bar);
    return;
  }


  if (data.size() >= 2) {
    unsigned bestDP = 0;
    float    largestSIGMA = 0;
    vector<DistObjectPairVector<dist_t>> dpARR(max_pivot_select_attempts);

    // To compute StdDev we need at least 2 points not counting the pivot
    size_t maxAtt = data.size() >= max(3, MIN_PIVOT_SELECT_DATA_QTY) ? max_pivot_select_attempts : 1;

    vector<double> dists(data.size());
    for (size_t att = 0; att < maxAtt; ++att) {
      dpARR[att].reserve(data.size());
      const size_t  currPivotIndex = SelectVantagePoint(data, use_random_center);
      const Object* pCurrPivot = data[currPivotIndex];
      for (size_t i = 0; i < data.size(); ++i) {
        if (i == currPivotIndex) {
          continue;
        }
        // Distance can be asymmetric, the pivot is always on the left side!
        dist_t d = space.IndexTimeDistance(pCurrPivot, data[i]);
        dists[i] = d;
        dpARR[att].emplace_back(d, data[i]);
      }

      double sigma = StdDev(&dists[0], dists.size());
      if (att == 0 || sigma > largestSIGMA) {
        //LOG(LIB_INFO) << " ### " << largestSIGMA << " -> "  << sigma << " att=" << att << " data.size()=" << data.size();
        largestSIGMA = sigma;
        bestDP = att;
        pivot_ = pCurrPivot;
        std::sort(dpARR[att].begin(), dpARR[att].end(), DistObjectPairAscComparator<dist_t>());
      }
    }


    DistObjectPairVector<dist_t>& dp = dpARR[bestDP];
    DistObjectPair<dist_t>  medianDistObj = GetMedian(dp);
    mediandist_ = medianDistObj.first; 

    ObjectVector left;
    ObjectVector right;

    for (auto it = dp.begin(); it != dp.end(); ++it) {
      const Object* v = it->second;

      /* 
       * Note that here we compare a pair (distance, pointer)
       * If distances are equal, pointers are compared.
       * Thus, we would get a balanced split, even if the median
       * occurs many times in the array dp[].
       */
      if (*it < medianDistObj) {
        left.push_back(v);
      } else {
        right.push_back(v);
      }
    }

    /*
     * Sometimes, e.g.., for integer-valued distances,
     * mediandist_ will be non-discriminative. In this case
     * it is more efficient to put everything into a single bucket.
     */
    size_t LeastSize = dp.size() / BalanceConst;

    if (left.size() < LeastSize || right.size() < LeastSize) {
        CreateBucket(ChunkBucket, data, progress_bar);
        return;
    }

    if (!left.empty()) {
      left_child_ = new VPNode(level + 1, progress_bar, oracle_, space, left, max_pivot_select_attempts, BucketSize, ChunkBucket, use_random_center);
    }

    if (!right.empty()) {
      right_child_ = new VPNode(level + 1, progress_bar, oracle_, space, right, max_pivot_select_attempts, BucketSize, ChunkBucket, use_random_center);
    }
  } else {
    CHECK_MSG(data.size() == 1, "Bug: expect the subset to contain exactly one element!");
    pivot_ = data[0];
  }
}

template <typename dist_t, typename SearchOracle>
VPTree<dist_t, SearchOracle>::VPNode::~VPNode() {
  delete left_child_;
  delete right_child_;
  ClearBucket(CacheOptimizedBucket_, bucket_);
}

template <typename dist_t, typename SearchOracle>
template <typename QueryType>
void VPTree<dist_t, SearchOracle>::VPNode::GenericSearch(QueryType* query,
                                                         int& MaxLeavesToVisit) const {
  if (MaxLeavesToVisit <= 0) return; // early termination
  if (bucket_) {
    --MaxLeavesToVisit;

    if (CacheOptimizedBucket_) {
      PREFETCH(CacheOptimizedBucket_, _MM_HINT_T0);
    }

    for (unsigned i = 0; i < bucket_->size(); ++i) {
      const Object* Obj = (*bucket_)[i];
      dist_t distQC = query->DistanceObjLeft(Obj);
      query->CheckAndAddToResult(distQC, Obj);
    }
    return;
  }

  // Distance can be asymmetric, the pivot is always the left argument (see the function that creates the node)!
  dist_t distQC = query->DistanceObjLeft(pivot_);
  query->CheckAndAddToResult(distQC, pivot_);

  if (distQC < mediandist_) {      // the query is inside
    // then first check inside
    if (left_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitRight)
       left_child_->GenericSearch(query, MaxLeavesToVisit);

    /* 
     * After potentially visiting the left child, we need to reclassify the node,
     * because the query radius might have decreased.
     */


    // after that outside
    if (right_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, MaxLeavesToVisit);
  } else {                         // the query is outside
    // then first check outside
    if (right_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitLeft)
       right_child_->GenericSearch(query, MaxLeavesToVisit);

    /* 
     * After potentially visiting the left child, we need to reclassify the node,
     * because the query radius might have decreased.
     */

    // after that inside
    if (left_child_ != NULL && oracle_.Classify(distQC, query->Radius(), mediandist_) != kVisitRight)
      left_child_->GenericSearch(query, MaxLeavesToVisit);
  }
}

template class VPTree<float, PolynomialPruner<float> >;
template class VPTree<int, PolynomialPruner<int> >;

}   // namespace similarity

