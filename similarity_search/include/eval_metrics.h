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

#ifndef EVAL_METRICS_H
#define EVAL_METRICS_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <unordered_set>

namespace similarity {

using std::vector;
using std::sort;
using std::ostream;
using std::unordered_map;

template <class dist_t>
struct ResultEntry {
  IdType      mId;
  LabelType   mLabel;
  dist_t      mDist;
  ResultEntry(IdType id = 0, LabelType label = 0, dist_t dist = 0) 
                 : mId(id), mLabel(label), mDist(dist) {}
  bool operator<(const ResultEntry& o) const {
    if (mDist != o.mDist) return mDist < o.mDist;
    return mId < o.mId;
  }
};

template <class dist_t>
ostream& operator<<(ostream& out, const ResultEntry<dist_t>& e) {
  return out << "[" << e.mId << " lab=" << e.mLabel << " dist=" << e.mDist << "]";
}

template <class dist_t>
bool ApproxEqualElem(const ResultEntry<dist_t>& elemApprox, const ResultEntry<dist_t>& elemExact) {
  return elemApprox.mId == elemExact.mId ||
         ApproxEqual(elemApprox.mDist, elemExact.mDist);
}

template <class dist_t>
struct EvalMetrics {
  /*
   * A classic recall measure
   */
  static double Recall(double ExactResultSize,
                      const vector<ResultEntry<dist_t>>& ExactEntries, const unordered_set<IdType>& ExactResultIds,
                      const vector<ResultEntry<dist_t>>& ApproxEntries, const unordered_set<IdType>& ApproxResultIds
                      ) {
    if (ExactResultIds.empty()) return 1.0;
    double recall = 0.0;
    for (auto it = ApproxResultIds.begin(); it != ApproxResultIds.end(); ++it) {
      recall += ExactResultIds.count(*it);
    }
    return recall / ExactResultSize;
  }
  /*
   * Number of the nearest neighbors or range search answers that are closer to the query
   * than the closest element returned by the search.
   */
  static double NumberCloser(double ExactResultSize,
                      const vector<ResultEntry<dist_t>>& ExactEntries, const unordered_set<IdType>& ExactResultIds,
                      const vector<ResultEntry<dist_t>>& ApproxEntries, const unordered_set<IdType>& ApproxResultIds
                      ) {
    if (ExactEntries.empty()) return 1.0;
    if (ApproxEntries.empty()) return ExactResultSize;
    
    double NumberCloser = 0;

    // 2. Compute the number of points closer to the 1-NN then the first result.
    CHECK(!ApproxEntries.empty());
    for (size_t p = 0; p < ExactEntries.size(); ++p) {
      if (ExactEntries[p].mDist >= ApproxEntries[0].mDist) break;
      ++NumberCloser;
    }

    return NumberCloser;
  }

  template <class AccumObj>
  static void iterate(AccumObj& obj,
               const vector<ResultEntry<dist_t>>& ExactEntries, const unordered_set<IdType>& ExactResultIds,
               const vector<ResultEntry<dist_t>>& ApproxEntries, const unordered_set<IdType>& ApproxResultIds
               ) {
      for (size_t k = 0, p = 0; k < ApproxEntries.size(); ++k) {
        const auto& elemApprox = ApproxEntries[k];
        const auto& elemExact  = ExactEntries[p];
        /*
         * There is no guarantee that the floating point arithmetic
         * produces consistent results. For instance, we can call the distance
         * function twice with the same object pointers, but get slightly
         * different results.
         */
        if (elemApprox.mDist -  elemExact.mDist < 0 &&
            !ApproxEqualElem(elemApprox, elemExact)
            ) {
          double mx = std::abs(std::max(ApproxEntries[k].mDist, ExactEntries[p].mDist));
          double mn = std::abs(std::min(ApproxEntries[k].mDist, ExactEntries[p].mDist));
  
          for (size_t i = 0; i < std::min(ExactEntries.size(), ApproxEntries.size()); ++i ) {
            LOG(LIB_INFO) << "Ex: " << ExactEntries[i].mDist << " id = " << ExactEntries[i].mId <<
                         " -> Apr: "<< ApproxEntries[i].mDist << " id = " << ApproxEntries[i].mId << 
                         " 1 - ratio: " << (1 - mn/mx) << " diff: " << (mx - mn);
          }
          LOG(LIB_FATAL) << "bug: the approximate query should not return objects "
                         << "that are closer to the query than object returned by "
                         << "(exact) sequential searching!"
                         << " Approx: " << elemApprox.mDist << " id = " << elemApprox.mId 
                         << " Exact: "  << elemExact.mDist  << " id = " << elemExact.mId;
        }
        // At this point the distance to the true answer is either >= than the distance to the approximate answer,
        // or the distance to the true answer is slightly smaller due to floating point arithmetic inconsistency
        size_t LastEqualP = p;
        if (p < ExactEntries.size() && 
            ApproxEqualElem(elemApprox, elemExact)) {
          ++p;
        } else {
          while (p < ExactEntries.size() && 
                 ExactEntries[p].mDist < ApproxEntries[k].mDist &&
                 !ApproxEqualElem(ExactEntries[p], ApproxEntries[k]))
                 {
            ++p;
            ++LastEqualP;
          }
        }
        if (p < k) {
          for (size_t i = 0; i < std::min(ExactEntries.size(), ApproxEntries.size()); ++i ) {
            LOG(LIB_INFO) << "E: " << ExactEntries[i].mDist << " -> " << ApproxEntries[i].mDist;
          }
          LOG(LIB_FATAL) << "bug: p = " << p << " k = " << k;
        }
        CHECK(p >= k);
        obj(k, LastEqualP);
      }
    }

   /*
    * Precision of approximation.
    *
    * Proposed in:
    * Zezula, P., Savino, P., Amato, G., Rabitti, F., 
    * Approximate similarity retrieval with m-trees. 
    * The VLDB Journal 7(4) (December 1998) 275-293
    *
    * Formally, the precision of approximation is equal to:
    * 1/K sum_{i=1}^K   i/pos(i)
    */
  struct AccumPrecisionOfApprox {
    double PrecisionOfApprox_ = 0;
    void operator()(size_t k, size_t LastEqualP) {
       PrecisionOfApprox_ += static_cast<double>(k + 1) / (LastEqualP + 1);
    }
  };

  static double PrecisionOfApprox(double ExactResultSize,
                      const vector<ResultEntry<dist_t>>& ExactEntries, const unordered_set<IdType>& ExactResultIds,
                      const vector<ResultEntry<dist_t>>& ApproxEntries, const unordered_set<IdType>& ApproxResultIds
                      ) {
    if (ExactEntries.empty()) return 1.0;
    if (ApproxEntries.empty()) return 0.0;

    AccumPrecisionOfApprox res;

    iterate(res, ExactEntries, ExactResultIds, ApproxEntries, ApproxResultIds);
    
    return res.PrecisionOfApprox_ / ApproxEntries.size();
  }

  struct AccumLogRelPossError {
    double LogRelPosError_ = 0;
    void operator()(size_t k, size_t LastEqualP) {
       LogRelPosError_    += log(static_cast<double>(LastEqualP + 1) / (k + 1));
    }
  };

  static double LogRelPosError(double ExactResultSize,
                      const vector<ResultEntry<dist_t>>& ExactEntries, const unordered_set<IdType>& ExactResultIds,
                      const vector<ResultEntry<dist_t>>& ApproxEntries, const unordered_set<IdType>& ApproxResultIds
                      ) {
    if (ExactEntries.empty()) return 1.0;
    if (ApproxEntries.empty()) return log(ExactResultSize);

    AccumLogRelPossError res;

    iterate(res, ExactEntries, ExactResultIds, ApproxEntries, ApproxResultIds);
    
    return res.LogRelPosError_ / ApproxEntries.size();
  }
};

}

#endif
