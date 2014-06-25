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

enum ClassResult {
  kClassUnknown,
  kClassCorrect,
  kClassWrong,
};

template <class dist_t>
class GoldStandard {
public:
  GoldStandard(const typename similarity::Space<dist_t>* space,
              const ObjectVector& datapoints, 
              const typename similarity::KNNQuery<dist_t>* query 
              ) {
    DoSeqSearch(space, datapoints, query->QueryObject());
  }
  GoldStandard(const typename similarity::Space<dist_t>* space,
              const ObjectVector& datapoints, 
              const typename similarity::RangeQuery<dist_t>* query 
              ) {
    DoSeqSearch(space, datapoints, query->QueryObject());
  }
  uint64_t GetSeqSearchTime()     const { return SeqSearchTime_; }

  const vector<ResultEntry<dist_t>>&   GetExactEntries() const { return  ExactEntries_;}
private:
  void DoSeqSearch(const similarity::Space<dist_t>* space,
                   const ObjectVector&              datapoints,
                   const Object*                    query) {
    WallClockTimer  wtm;

    wtm.reset();

    ExactEntries_.resize(datapoints.size());

    for (size_t i = 0; i < datapoints.size(); ++i) {
      // Distance can be asymmetric, but the query is always on the right side
      ExactEntries_[i] = ResultEntry<dist_t>(datapoints[i]->id(), datapoints[i]->label(), space->IndexTimeDistance(datapoints[i], query));
    }

    wtm.split();

    SeqSearchTime_ = wtm.elapsed();

    std::sort(ExactEntries_.begin(), ExactEntries_.end());
  }

  uint64_t                            SeqSearchTime_;

  vector<ResultEntry<dist_t>>         ExactEntries_;
};

template <class dist_t>
class EvalResults {
public:
  EvalResults(const typename similarity::Space<dist_t>* space,
                   const typename similarity::KNNQuery<dist_t>* query,
                   const GoldStandard<dist_t>& gs) : K_(0), ExactEntries_(gs.GetExactEntries()) {
    GetKNNData(query);
    ComputeMetrics(query->QueryObject()->label());
  }

  EvalResults(const typename similarity::Space<dist_t>* space,
                   const typename similarity::RangeQuery<dist_t>* query,
                   const GoldStandard<dist_t>& gs) : K_(0), ExactEntries_(gs.GetExactEntries()) {
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
    for (size_t i = 0; i < ExactEntries_.size(); ++i) {
      if (i < K_ || (K_ && ApproxEqual(ExactEntries_[i].mDist,  ExactEntries_[K_-1].mDist))) {
        ExactResultIds_.insert(ExactEntries_[i].mId);
      }
      else break; // ExactEntries_ are sorted by distance
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
  }

  void GetRangeData(const RangeQuery<dist_t>* query) {
    for (size_t i = 0; i < ExactEntries_.size(); ++i) {
      if (ExactEntries_[i].mDist <= query->Radius()) ExactResultIds_.insert(ExactEntries_[i].mId);
      else break; // ExactEntries_ are sorted by distance
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
    Recall_            = EvalRecall<dist_t>()(ExactResultSize, ExactEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);
    NumberCloser_      = EvalNumberCloser<dist_t>()(ExactResultSize, ExactEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);
    PrecisionOfApprox_ = EvalPrecisionOfApprox<dist_t>()(ExactResultSize, ExactEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);
    LogRelPosError_    = EvalLogRelPosError<dist_t>()(ExactResultSize, ExactEntries_, ExactResultIds_, ApproxEntries_, ApproxResultIds_);

    // 2 Obtain class result
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

  double                              NumberCloser_;
  double                              LogRelPosError_;
  double                              Recall_;
  ClassResult                         ClassCorrect_;
  double                              PrecisionOfApprox_;

  std::vector<ResultEntry<dist_t>>    ApproxEntries_;
  std::unordered_set<IdType>          ApproxResultIds_;
  std::unordered_set<IdType>          ExactResultIds_;

  const std::vector<ResultEntry<dist_t>>& ExactEntries_;
};

}

#endif

