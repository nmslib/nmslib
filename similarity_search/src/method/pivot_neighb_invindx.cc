/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "incremental_quick_select.h"
#include "pivot_neighb_invindx.h"
#include "utils.h"

namespace similarity {

using std::vector;
using std::pair;

struct IdCount {
  size_t id;
  size_t qty;
  IdCount(size_t id=0, int    qty=0) : id(id), qty(qty) {}
};

typedef vector<IdCount> VectIdCount;

/*
void fastMemSet(uint32_t* ptr, size_t qty) {
  size_t qty4 = qty / 4;
  
  __m128i zero = _mm_set1_epi32(0);
  __m128i* ptr128 = reinterpret_cast<__m128i*>(ptr);
  __m128i* ptr128end = ptr128 + qty4;
  while (ptr128 < ptr128end)
    _mm_storeu_si128(ptr128++, zero);
  for (size_t i = 4*qty4; i < qty; ++i)
    ptr[i] = 0;
}
*/

void postListUnion(const VectIdCount& lst1, const PostingListInt lst2, VectIdCount& res) {
  res.clear();
  res.reserve((lst1.size() + lst2.size())/2);
  auto i1 = lst1.begin();
  auto i2 = lst2.begin();

  while (i1 != lst1.end() && i2 != lst2.end()) {
    size_t id2 = static_cast<size_t>(*i2);
    if (i1->id < id2) {
      res.push_back(*i1);
      ++i1;
    } else if (i1->id > id2) {
      res.push_back(IdCount(id2, 1));
      ++i2;
    } else {
      res.push_back(IdCount(i1->id, i1->qty + 1));
      ++i1;
      ++i2;
    }
  }
  while (i1 != lst1.end()) {
      res.push_back(*i1);
    ++i1;
  }
  while (i2 != lst2.end()) {
      res.push_back(IdCount(*i2, 1));
      ++i2;
  }
}

template <typename dist_t>
PivotNeighbInvertedIndex<dist_t>::PivotNeighbInvertedIndex(
    const Space<dist_t>* space,
    const ObjectVector& data,
    const AnyParams& AllParams) 
: data_(data),   // reference
  chunk_index_size_(65536),
  db_scan_(0),
  num_prefix_(32),
  min_times_(2),
  use_sort_(false),
  skip_checking_(false),
  inv_proc_alg_ (kScan) {
  AnyParamManager pmgr(AllParams);

  size_t num_pivot        = 512; 
    
  string inv_proc_alg = PERM_PROC_FAST_SCAN;

  pmgr.GetParamOptional("numPivot", num_pivot);
  pmgr.GetParamOptional("numPrefix", num_prefix_);
  pmgr.GetParamOptional("chunkIndexSize", chunk_index_size_);

  if (num_prefix_ > num_pivot) {
    LOG(FATAL) << METH_PIVOT_NEIGHB_INVINDEX << " requires that numPrefix "
               << "should be less than or equal to numPivot";
  }

  CHECK(num_prefix_ <= num_pivot);
  
  size_t indexQty = (data_.size() + chunk_index_size_ - 1) / chunk_index_size_;


  LOG(INFO) << "# of entries in an index chunk  = " << chunk_index_size_;
  LOG(INFO) << "# of index chunks  = " << posting_lists_.size();
  LOG(INFO) << "# pivots      = " << num_pivot;
  LOG(INFO) << "# prefix (K)  = " << num_prefix_;
  
  SetQueryTimeParams(pmgr);

  GetPermutationPivot(data, space, num_pivot, &pivot_);

  posting_lists_.resize(indexQty);
  for (size_t chunkId = 0; chunkId < indexQty; ++chunkId) {
    size_t minId = chunkId * chunk_index_size_;
    size_t maxId = min(data_.size(), minId + chunk_index_size_);

    auto & chunkPostLists = posting_lists_[chunkId];
    chunkPostLists.resize(num_pivot);

    for (size_t id = 0; id < maxId - minId; ++id) {
      Permutation perm;
      GetPermutationPPIndex(pivot_, space, data[minId + id], &perm);
      for (size_t j = 0; j < num_prefix_; ++j) {
        chunkPostLists[perm[j]].push_back(id);
      }
    }

    // Sorting is essential for merging algos
    for (auto & p:chunkPostLists) {
      sort(p.begin(), p.end());
    }
  }
}
    

    
    
template <typename dist_t>
void 
PivotNeighbInvertedIndex<dist_t>::SetQueryTimeParams(AnyParamManager& pmgr) {
  float db_scan_frac = 0.05;
  
  string inv_proc_alg = PERM_PROC_FAST_SCAN;
  
  pmgr.GetParamOptional("dbScanFrac",   db_scan_frac);
  pmgr.GetParamOptional("skipChecking", skip_checking_);
  pmgr.GetParamOptional("useSort",      use_sort_);
  pmgr.GetParamOptional("invProcAlg",   inv_proc_alg);
  pmgr.GetParamOptional("minTimes",     min_times_);
  
  if (inv_proc_alg == PERM_PROC_FAST_SCAN) {
    inv_proc_alg_ = kScan; 
  } else if (inv_proc_alg == PERM_PROC_MAP) {
    inv_proc_alg_ = kMap; 
  } else if (inv_proc_alg == PERM_PROC_MERGE) {
    inv_proc_alg_ = kMerge; 
  } else {
    LOG(FATAL) << "Unknown value of parameter for the inverted file processing algorithm: " << inv_proc_alg_;
  } 
    
  ComputeDbScan(db_scan_frac);
}

template <typename dist_t>
vector<string>
PivotNeighbInvertedIndex<dist_t>::GetQueryTimeParamNames() const {
  vector<string> names;
  
  names.push_back("dbScanFrac");
  names.push_back("useSort");
  names.push_back("skipChecking");
  names.push_back("invProcAlg");
  names.push_back("minTimes");
    
  return names;
}
    
    

template <typename dist_t>
PivotNeighbInvertedIndex<dist_t>::~PivotNeighbInvertedIndex() {
}

template <typename dist_t>
const string PivotNeighbInvertedIndex<dist_t>::ToString() const {
  stringstream str;
  str <<  "permutation (inverted index over neighboring pivots)";
  return str.str();
}

template <typename dist_t>
template <typename QueryType>
void PivotNeighbInvertedIndex<dist_t>::GenSearch(QueryType* query) {
  Permutation perm_q;
  GetPermutationPPIndex(pivot_, query, &perm_q);

  vector<unsigned>          counter(chunk_index_size_);



  for (size_t chunkId = 0; chunkId < posting_lists_.size(); ++chunkId) {
    const auto & chunkPostLists = posting_lists_[chunkId];
    size_t minId = chunkId * chunk_index_size_;
    size_t maxId = min(data_.size(), minId + chunk_index_size_);
    size_t chunkQty = maxId - minId;

    const auto data_start = &data_[0] + minId;

    if (use_sort_) {
      vector<pair<int, size_t>> candidates;

      if (inv_proc_alg_ == kMap) {
        std::unordered_map<uint32_t, uint32_t> map_counter;
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            map_counter[p]++;
          }
        }

        candidates.reserve(db_scan_);

        for (auto& it : map_counter) {
          if (it.second >= min_times_) {
            candidates.push_back(std::make_pair(-it.second, it.first));
          }
        }
      } else if (inv_proc_alg_ == kScan) {
        candidates.resize(chunkQty);

        for (size_t i = 0; i < candidates.size(); ++i) {
          candidates[i].second = i;
        }
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            candidates[p].first--;
          }
        }
      } else if (inv_proc_alg_ == kMerge) {
        VectIdCount   tmpRes[2];
        unsigned      prevRes = 0;

        for (size_t i = 0; i < num_prefix_; ++i) {
          postListUnion(tmpRes[prevRes], chunkPostLists[perm_q[i]], tmpRes[1-prevRes]);
          prevRes = 1 - prevRes;
        }

        candidates.reserve(db_scan_);

        for (const auto& it: tmpRes[1-prevRes]) {
          if (it.qty >= min_times_) {
            candidates.push_back(std::make_pair(-it.qty, it.id));
          }
        }
      } else {
        LOG(FATAL) << "Bug, unknown inv_proc_alg_: " << inv_proc_alg_;
      }

      IncrementalQuickSelect<IntInt> quick_select(candidates);

      size_t scan_qty = min(db_scan_, candidates.size());

      for (size_t i = 0; i < scan_qty; ++i) {
        auto z = quick_select.GetNext();
        if (static_cast<size_t>(-z.first) >= min_times_) {
          const size_t idx = z.second;
          quick_select.Next();
          if (!skip_checking_) query->CheckAndAddToResult(data_start[idx]);
        } else {
          break;
        }
      }
    } else {
      if (inv_proc_alg_ == kMap) {
        std::unordered_map<uint32_t, uint32_t> map_counter;
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            map_counter[p]++;
          }
        }
        for (auto& it : map_counter) {
          if (it.second >= min_times_) {
            const size_t idx = it.first;
            if (!skip_checking_) query->CheckAndAddToResult(data_start[idx]);
          }
        }
      } else if (inv_proc_alg_ == kScan) {
        if (chunkId) {
          //fastMemSet(&counter[0], counter.size());
          memset(&counter[0], 0, sizeof(counter[0])*counter.size());
        }
        for (size_t i = 0; i < num_prefix_; ++i) {
          for (auto& p : chunkPostLists[perm_q[i]]) {
            counter[p]++;
          }
        }
        for (size_t i = 0; i < chunkQty; ++i) {
          if (counter[i] >= min_times_) {
            if (!skip_checking_) query->CheckAndAddToResult(data_start[i]);
          }
        }
      } else if (inv_proc_alg_ == kMerge) {
        VectIdCount   tmpRes[2];
        unsigned      prevRes = 0;
        for (size_t i = 0; i < num_prefix_; ++i) {
          postListUnion(tmpRes[prevRes], chunkPostLists[perm_q[i]], tmpRes[1-prevRes]);
          prevRes = 1 - prevRes;
        }

        for (const auto& it: tmpRes[1-prevRes]) {
          if (it.qty >= min_times_) {
            if (!skip_checking_) query->CheckAndAddToResult(data_start[it.id]);
          }
        }
      } else {
        LOG(FATAL) << "Bug, unknown inv_proc_alg_: " << inv_proc_alg_;
      }
    }
  }
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::Search(
    RangeQuery<dist_t>* query) {
  GenSearch(query);
}

template <typename dist_t>
void PivotNeighbInvertedIndex<dist_t>::Search(
    KNNQuery<dist_t>* query) {
  GenSearch(query);
}

template class PivotNeighbInvertedIndex<float>;
template class PivotNeighbInvertedIndex<double>;
template class PivotNeighbInvertedIndex<int>;

}  // namespace similarity
