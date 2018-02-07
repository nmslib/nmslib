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
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "permutation_utils.h"
#include "ported_boost_progress.h"
#include "method/permutation_prefix_index.h"

namespace similarity {

class PrefixNode;
class PrefixNodeInternal;
class PrefixNodeLeaf;

class PrefixNode {
 public:
  virtual ~PrefixNode() {};
  virtual bool IsLeaf() const = 0;
  virtual void Insert(const Permutation& perm,
                      const Object* object,
                      const size_t length,
                      const size_t cur_depth) = 0;
  virtual size_t GetNumberObjects() const = 0;
  virtual void GetAllObjectsInThisSubTree(
      ObjectVector* objects,
      std::unordered_set<const PrefixNode*>* visited) const = 0;
  virtual const PrefixNode* SearchPath(const Permutation& perm,
                                       const size_t sub_length,
                                       const size_t cur_depth) const = 0;
  virtual void ChunkBuckets() = 0;
};

class PrefixNodeLeaf : public PrefixNode {
 public:
  PrefixNodeLeaf() : bucket_(new ObjectVector()), CacheOptimizedBucket_(NULL) {}
  virtual ~PrefixNodeLeaf() { ClearBucket(CacheOptimizedBucket_, bucket_); }

  virtual void ChunkBuckets() {
    ObjectVector* pData = bucket_;

    CreateCacheOptimizedBucket(*pData, CacheOptimizedBucket_, bucket_);

    delete pData;
  }

  bool IsLeaf() const { return true; }

  void Insert(const Permutation& perm,
              const Object* object,
              const size_t length,
              const size_t cur_depth) {
    CHECK(cur_depth == length);
    bucket_->push_back(object);
  }

  size_t GetNumberObjects() const { return bucket_->size(); }

  void GetAllObjectsInThisSubTree(
      ObjectVector* objects,
      std::unordered_set<const PrefixNode*>* visited) const {
    if (visited->count(this) == 0) {
      visited->insert(this);
      objects->insert(objects->end(), bucket_->begin(), bucket_->end());
    }
  }

  const PrefixNode* SearchPath(const Permutation& perm,
                               const size_t sub_length,
                               const size_t cur_depth) const {
    return this;
  }

 private:
  ObjectVector* bucket_;
  char*         CacheOptimizedBucket_;
};

class PrefixNodeInternal : public PrefixNode {
 public:
  PrefixNodeInternal() : num_objects_(0) {}

  ~PrefixNodeInternal() {
    for (auto& it : children_) {
      delete it.second;
    }
  }

  bool IsLeaf() const { return false; }

  virtual void ChunkBuckets() {
    for (auto& it: children_) it.second->ChunkBuckets();
  }

  void Insert(const Permutation& perm,
              const Object* object,
              const size_t length,
              const size_t cur_depth) {
    CHECK(cur_depth < length);
    ++num_objects_;
    PrefixNode* node = NULL;
    auto it = children_.find(perm[cur_depth]);
    if (it != children_.end()) {
      node = it->second;
    } else {
      if (cur_depth + 1 < length) {
        node = new PrefixNodeInternal;
      } else {
        node = new PrefixNodeLeaf;
      }
      children_[perm[cur_depth]] = node;
    }
    node->Insert(perm, object, length, cur_depth + 1);
  }

  size_t GetNumberObjects() const { return num_objects_; }

  void GetAllObjectsInThisSubTree(
      ObjectVector* objects,
      std::unordered_set<const PrefixNode*>* visited) const {
    if (visited->count(this) == 0) {
      visited->insert(this);
      for (const auto& it : children_) {
        it.second->GetAllObjectsInThisSubTree(objects, visited);
      }
    }
  }

  const PrefixNode* SearchPath(const Permutation& perm,
                               const size_t sub_length,
                               const size_t cur_depth) const {
    CHECK(cur_depth <= sub_length);
    if (cur_depth == sub_length) {
      return this;
    } else {
      const auto& it = children_.find(perm[cur_depth]);
      return it != children_.end()
          ? it->second->SearchPath(perm, sub_length, cur_depth + 1)
          : NULL;
    }
  }

 private:
  typedef std::unordered_map<size_t, PrefixNode*> PrefixMap;
  PrefixMap children_;
  size_t num_objects_;
};

class PrefixTree {
 public:
  PrefixTree() : root_(new PrefixNodeInternal) {}
  void ChunkBuckets() { root_->ChunkBuckets(); }

  ~PrefixTree() { delete root_; }

  void Insert(const Permutation& perm,
              const Object* object,
              const size_t length) {
    root_->Insert(perm, object, length, 0);
  }

  void FindCandidates(const Permutation& perm_q,
                      const size_t prefix_length,
                      const size_t min_candidate,
                      ObjectVector* candidates) {
    std::unordered_set<const PrefixNode*> visited;
    visited.reserve(min_candidate * 2);
    for (int i = static_cast<int>(prefix_length); i >= 0; --i) {
      const PrefixNode* node = root_->SearchPath(perm_q, i, 0);
      if (node != NULL &&
          (node->GetNumberObjects() >= min_candidate || i == 0)) {
        node->GetAllObjectsInThisSubTree(candidates, &visited);
        CHECK(node->GetNumberObjects() == candidates->size());
        return;
      }
    }
  }

 private:
  PrefixNode* root_;
};

template <typename dist_t>
void PermutationPrefixIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager   pmgr(QueryTimeParams);

  if (pmgr.hasParam("minCandidate") && pmgr.hasParam("knnAmp")) {
    throw runtime_error("One shouldn't specify both parameters minCandidate and knnAmp, b/c they are synonyms!");
  }

  pmgr.GetParamOptional("minCandidate", min_candidate_, 0);
  pmgr.GetParamOptional("knnAmp",       knn_amp_,       0);

  LOG(LIB_INFO) << "Set query-time parameters for PermutationPrefixIndex:";
  LOG(LIB_INFO) << "knnAmp=       " << knn_amp_;
  LOG(LIB_INFO) << "minCandidate= " << min_candidate_;
}

template <typename dist_t>
PermutationPrefixIndex<dist_t>::PermutationPrefixIndex(
    bool  PrintProgress,
    const Space<dist_t>& space,
    const ObjectVector& data) : Index<dist_t>(data), space_(space), PrintProgress_(PrintProgress) {
}

template <typename dist_t>
void PermutationPrefixIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);

  pmgr.GetParamOptional("numPivot",     num_pivot_,       16);
  pmgr.GetParamOptional("chunkBucket",  chunkBucket_,      true);
  pmgr.GetParamOptional("prefixLength", prefix_length_,   max<size_t>(1, num_pivot_/4));

  pmgr.CheckUnused();
  this->ResetQueryTimeParams();

  LOG(LIB_INFO) << "# pivots         = " << num_pivot_;
  LOG(LIB_INFO) << "prefix length    = " << prefix_length_;
  LOG(LIB_INFO) << "ChunkBucket      = " << chunkBucket_;

  GetPermutationPivot(this->data_, space_, num_pivot_, &pivot_);
  prefixtree_.reset(new PrefixTree);
  Permutation permutation;


  unique_ptr<ProgressDisplay> progress_bar(PrintProgress_ ?
                                new ProgressDisplay(this->data_.size(), cerr)
                                :NULL);

  for (const auto& it : this->data_) {
    permutation.clear();
    GetPermutationPPIndex(pivot_, space_, it, &permutation);
    prefixtree_->Insert(permutation, it, prefix_length_);

    if (progress_bar) ++(*progress_bar);
  }
  // Store elements in leaves/buckets contiguously
  if (chunkBucket_) prefixtree_->ChunkBuckets();
}

template <typename dist_t>
PermutationPrefixIndex<dist_t>::~PermutationPrefixIndex() {
}

template <typename dist_t>
const std::string PermutationPrefixIndex<dist_t>::StrDesc() const {
  return "permutation (pref. index)";
}

template <typename dist_t>
template <typename QueryType>
void PermutationPrefixIndex<dist_t>::GenSearch(QueryType* query, size_t K) const {
  if (prefix_length_ == 0 || prefix_length_ > num_pivot_) {
    stringstream err;
    err << METH_PERMUTATION_PREFIX_IND
               << " requires that prefix length should be in the range in [1,"
               << num_pivot_ << "]";
    throw runtime_error(err.str());
  }

  Permutation perm_q;
  GetPermutationPPIndex(pivot_, query, &perm_q);

  size_t db_scan = computeDbScan(K);

  if (!db_scan) {
    if (!db_scan) {
      throw runtime_error("One should specify a proper value for either minCandidate or knnAmp");
    }
  }

  ObjectVector candidates;
  candidates.reserve(2  * db_scan);
  prefixtree_->FindCandidates(perm_q, prefix_length_,
                              db_scan, &candidates);

  for (const auto& it : candidates) {
    query->CheckAndAddToResult(it);
  }
}

template <typename dist_t>
void PermutationPrefixIndex<dist_t>::Search(RangeQuery<dist_t>* query, IdType ) const {
  GenSearch(query, 0);
}

template <typename dist_t>
void PermutationPrefixIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType ) const {
  GenSearch(query, query->GetK());
}

template class PermutationPrefixIndex<float>;
template class PermutationPrefixIndex<double>;
template class PermutationPrefixIndex<int>;

}  // namespace similarity

