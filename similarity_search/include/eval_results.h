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
#ifndef EVAL_RESULTS_H
#define EVAL_RESULTS_H

#include <iostream>
#include <algorithm>
#include <vector>
#include <unordered_set>
#include <memory>
#include <stdexcept>
#include <unordered_map>

#include "utils.h"
#include "space.h"
#include "object.h"
#include "index.h"
#include "knnqueue.h"
#include "eval_metrics.h"

namespace similarity {

using std::unordered_set;
using std::unordered_map;
using std::vector;
using std::sort;



template <class dist_t>
class EvalResults {
public:
  EvalResults(const typename similarity::Space<dist_t>& space,
              const typename similarity::KNNQuery<dist_t>* query,
              const GoldStandard<dist_t>& gs) : K_(0), SortedAllEntries_(gs.GetSortedEntries()) {
    GetKNNData(query);
    ComputeMetrics(query->QueryObject()->label());
  }

  EvalResults(const typename similarity::Space<dist_t>& space,
              const typename similarity::RangeQuery<dist_t>* query,
              const GoldStandard<dist_t>& gs) : K_(0), SortedAllEntries_(gs.GetSortedEntries()) {
    GetRangeData(query);
    ComputeMetrics(query->QueryObject()->label());
  }

  /* 
   * For all metrics below, pos(i) denotes a position of the 
   * i-th approximate result in the exact list of neighbors.
   * That is, a k=pos(i) means that the i-th element returned
   * by a search method is, in truth, a k-th nearest neighbor.
   */

  /* 
   * GetNumCloser() makes most sense only for 1-NN search,
   * because it computes the number of points closer than
   * the nearest point found by a method.
   * 
   * Formally is it equal to pos(0) - 1.
   *
   * This metric was proposed in:
   * L. Cayton. Fast nearest neighbor retrieval for bregman divergences. 
   * Twenty-Fifth International Conference on Machine Learning (ICML), 2008.
   *
   */
  double GetNumCloser()       const { return NumberCloser_;}

  /*
   * Recall of the closets (1-NN) entry
   */
  double GetRecallAt1()       const { return RecallAt1_;}

   /*
    * An average logarithm of a relative position error.
    * Just exponentiate to get a geometric mean of relative position errors.
    *
    * Formally, it is equal to:
    * 1/K sum_{i=1}^K   log(pos(i))
    */
  double GetLogRelPos()       const { return LogRelPosError_;}
    /*
     * Just the classic recall value
     */
  double GetRecall()          const { return Recall_; }
    /*
     * A more detailed recall information for k-NN: 
     * for neighbors of all possible ranks i <= k, is this neighbor found?
     */
  const vector<char>& GetFound() const { return found_; }
    /*
     * Classification correctness
     */
  ClassResult GetClassCorrect()          const { return ClassCorrect_; }
   /*
    * Precision of approximation.
    *
    * Zezula, P., Savino, P., Amato, G., Rabitti, F., 
    * Approximate similarity retrieval with m-trees. 
    * The VLDB Journal 7(4) (December 1998) 275-293
    *
    * Formally, the precision of approximation is equal to:
    * 1/K sum_{i=1}^K   i/pos(i)
    */
  double GetPrecisionOfApprox() const { return PrecisionOfApprox_; }
private:
  size_t K_;

  /* 
   * In k-NN search the k-neighborhood may be definied ambiguously.
   * It can happen if several points are at the same distance from the query point.
   * All these points are included into the ExactResultIds_, yet, their number can be > K_.
   */
  void GetKNNData(const KNNQuery<dist_t>* query) {
    K_ = query->GetK();
    for (size_t i = 0; i < SortedAllEntries_.size(); ++i) {
      if (i < K_ || (K_ && ApproxEqual(SortedAllEntries_[i].mDist,  SortedAllEntries_[K_-1].mDist))) {
        ExactResultIds_.insert(SortedAllEntries_[i].mId);
      }
      else break; // SortedAllEntries_ are sorted by distance
    }

    unique_ptr<KNNQueue<dist_t>> ResQ(query->Result()->Clone());

    while (!ResQ->Empty()) {
      const Object* ResObject = reinterpret_cast<const Object*>(ResQ->TopObject());
      CHECK(ResObject);
      /*
       * A search method can potentially return duplicate records.
       * We simply ignore duplicates during evaluation.
       */
      if (ApproxResultIds_.find(ResObject->id()) == ApproxResultIds_.end()) {
        ApproxResultIds_.insert(ResObject->id());
        ApproxEntries_.insert(ApproxEntries_.begin(), 
                              ResultEntry<dist_t>(ResObject->id(), ResObject->label(), ResQ->TopDistance()));
      }
      ResQ->Pop();
    }
    // ApproxEntries_ should be sorted
  }

  void GetRangeData(const RangeQuery<dist_t>* query) {
    for (size_t i = 0; i < SortedAllEntries_.size(); ++i) {
      if (SortedAllEntries_[i].mDist <= query->Radius()) ExactResultIds_.insert(SortedAllEntries_[i].mId);
      else break; // SortedAllEntries_ are sorted by distance
    }

    const ObjectVector&         ResQ = *query->Result();
    const std::vector<dist_t>&  ResQDists = *query->ResultDists();

    CHECK(ResQ.size() == ResQDists.size());

    for (size_t i = 0; i < ResQ.size(); ++i) {
      const Object* ResObject = ResQ[i];
      CHECK(ResObject);
      // We should not have any duplicates!
      if (ApproxResultIds_.find(ResObject->id()) == ApproxResultIds_.end()) {
        ApproxResultIds_.insert(ResObject->id());
        ApproxEntries_.insert(ApproxEntries_.begin(), 
                              ResultEntry<dist_t>(ResObject->id(), ResObject->label(), ResQDists[i]));
      }
    }

    std::sort(ApproxEntries_.begin(), ApproxEntries_.end());
  }

  void ComputeMetrics(LabelType queryLabel) {
    size_t ExactResultSize = K_ ? min(K_,ExactResultIds_.size()) /* If the data set is tiny
                                                                    there may be less than K_
                                                                    answers */
                                  :
                                  ExactResultIds_.size();

    ClassCorrect_      = kClassUnknown;
    Recall_            = EvalRecall<dist_t>()(ExactResultSize, SortedAllEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);
    NumberCloser_      = EvalNumberCloser<dist_t>()(ExactResultSize, SortedAllEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);
    RecallAt1_         = NumberCloser_ > 0.1 ? 0.0 : 1;
    PrecisionOfApprox_ = EvalPrecisionOfApprox<dist_t>()(ExactResultSize, SortedAllEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);
    LogRelPosError_    = EvalLogRelPosError<dist_t>()(ExactResultSize, SortedAllEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);

    // 2 For k-NN search, we want to compute a fraction of found neighbors at a given rank
    if (K_ > 0) {
      CHECK_MSG(SortedAllEntries_.size() >= ExactResultSize, "Bug: SortedAllEntries_.size() < ExactResultSize");
      found_.resize(ExactResultSize);
      for (size_t i = 0; i < ExactResultSize; i++) {
        found_[i] = ApproxResultIds_.find(SortedAllEntries_[i].mId) != ApproxResultIds_.end();
      }
    }

    // 3 Obtain class result
    if (queryLabel >= 0) {
      unordered_map<LabelType, int>  hClassQty;
      vector<pair<int,LabelType>>    vClassQty;

      for (size_t k = 0; k < ApproxEntries_.size(); ++k) {
        hClassQty[ApproxEntries_[k].mLabel]++;
      }
      for (auto elem:hClassQty) {
        /* 
         * Revert here: qty now should go first:
         * the minus sign will make sort put entries
         * with the largest qty first.
         */
        vClassQty.push_back(make_pair(-elem.second, elem.first));
      }
      sort(vClassQty.begin(), vClassQty.end());
      if (!vClassQty.empty()) {
        ClassCorrect_ = vClassQty[0].second == queryLabel ? kClassCorrect : kClassWrong;
      }
    }
  }

  /*
   * A similarity measure for indefinite rankings
   * W Webber, A Moffat, J Zobel - ACM Transactions on Information Systems, 2010
   */
  static float 
  ComputeRBO(const vector<ResultEntry<dist_t>>& exactRes, const vector<ResultEntry<dist_t>>& approxRes, 
                   float p, size_t K) {
    unordered_set<IdType> seen;
    K = std::min(K, 
                 std::min(exactRes.size(), approxRes.size()));
    float rbo = 0, mult = 1;
    float overlap = 0;
    for (size_t rank = 0; rank < K; ++rank) {
      seen.insert(exactRes[rank].mId); 
      // At this point, set 'seen' contains IDs for items with ranks up to a given one
      IdType approxId = approxRes[rank].mId;
      if (seen.find(approxId) != seen.end()) 
        overlap++; 
      rbo += mult * overlap / rank;
      mult *= p;
    }
    return rbo * (1-p);
  }

  double                              RecallAt1_;
  double                              NumberCloser_;
  double                              LogRelPosError_;
  double                              Recall_;
  ClassResult                         ClassCorrect_;
  double                              PrecisionOfApprox_;
  vector<char>                        found_;

  std::vector<ResultEntry<dist_t>>    ApproxEntries_;
  std::unordered_set<IdType>          ApproxResultIds_;
  std::unordered_set<IdType>          ExactResultIds_;


  /* 
   * SortedAllEntries_ include all database entries sorted in the order
   * of increasing distance from the query.
   */
  const std::vector<ResultEntry<dist_t>>& SortedAllEntries_;
};

}

#endif

