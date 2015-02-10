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

#ifndef GOLD_STANDARD_H
#define GOLD_STANDARD_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <memory>

#include "object.h"
#include "query_creator.h"
#include "space.h"
#include "utils.h"
#include "ztimer.h"

#define SEQ_SEARCH_TIME_PREFIX "seqSearchTime"
#define SEQ_GS_QTY_PREFIX      "qty"

namespace similarity {

using std::vector;
using std::ostream;
using std::istream;
using std::unique_ptr;

template <class dist_t>
struct ResultEntry {
  IdType      mId;
  LabelType   mLabel;
  dist_t      mDist;
  ResultEntry(IdType id = 0, LabelType label = 0, dist_t dist = 0) 
                 : mId(id), mLabel(label), mDist(dist) {}

  /*
   * Reads entry in the binary format (can't read if the data was written
   * on the machine with another type of endianness.)
   */
  void readBinary(istream& in) {
    in.read(reinterpret_cast<char*>(&mId),    sizeof mId);
    in.read(reinterpret_cast<char*>(&mLabel), sizeof mLabel);
    in.read(reinterpret_cast<char*>(&mDist),  sizeof mDist);
  }
  /*
   * Saves entry in the binary format, see the comment
   * on the endianness above.
   */
  void writeBinary(ostream& out) {
    out.write(reinterpret_cast<const char*>(&mId),    sizeof mId);
    out.write(reinterpret_cast<const char*>(&mLabel), sizeof mLabel);
    out.write(reinterpret_cast<const char*>(&mDist),  sizeof mDist);
  }
  bool operator<(const ResultEntry& o) const {
    if (mDist != o.mDist) return mDist < o.mDist;
    return mId < o.mId;
  }
};

template <class dist_t>
ostream& operator<<(ostream& out, const ResultEntry<dist_t>& e) {
  return out << "[" << e.mId << " lab=" << e.mLabel << " dist=" << e.mDist << "]";
}

enum ClassResult {
  kClassUnknown,
  kClassCorrect,
  kClassWrong,
};

template <class dist_t>
class GoldStandard {
public:
  GoldStandard(){}
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
  /*
   * See the endianness comment.
   */
  void Write(ostream& controlStream, ostream& binaryStream) {
    controlStream << SEQ_SEARCH_TIME_PREFIX << FIELD_DELIMITER << SeqSearchTime_
                  << " " << SEQ_GS_QTY_PREFIX << FIELD_DELIMITER << SortedAllEntries_.size() << endl;
    for (const ResultEntry<dist_t> e: SortedAllEntries_) e.writeBinary(binaryStream);
  }

  void Read(istream& controlStream, istream& binaryStream) {
    string line;
    getline(controlStream, line);
    stringstream str(line);
    string  s1, s2, s;
    if (!(str >> s1) || !(str >> s2)) {
      throw runtime_error("Wrong format of the control string: '" + line + "'");
    }
    ReplaceSomePunct(s1);
    stringstream str1(s1);
    if (!(str1 >> s) || !(str1 >> SeqSearchTime_)) {
      throw runtime_error("Wrong format of the control string: '" + line +
                          "' can't read sequential search time.");
    }
    if (s != SEQ_SEARCH_TIME_PREFIX) {
      throw runtime_error("Wrong format of the control string: '" + line +
                          "' can't find sequential search time prefix.");
    }
    ReplaceSomePunct(s2);
    stringstream str2(s2);
    size_t qty;
    if (!(str2 >> s) || (!str2 >> qty)) {
      throw runtime_error("Wrong format of the control string: '" + line +
                          "' can't read the number of entries.");
    }
    if (s != SEQ_GS_QTY_PREFIX) {
      throw runtime_error("Wrong format of the control string: '" + line +
                          "' can't find the prefix for the number of entries.");
    }
    SortedAllEntries_.resize(qty);
    for (size_t i = 0; i < qty; ++i)
      SortedAllEntries_[i].readBinary(binaryStream);
  }
  uint64_t GetSeqSearchTime()     const { return SeqSearchTime_; }

  /*
   * SortedAllEntries_ include all database entries sorted in the order
   * of increasing distance from the query.
   */
  const vector<ResultEntry<dist_t>>&   GetSortedEntries() const { return  SortedAllEntries_;}
private:
  void DoSeqSearch(const Space<dist_t>* space,
                   const ObjectVector&  datapoints,
                   const Object*        query) {
    WallClockTimer  wtm;

    wtm.reset();

    SortedAllEntries_.resize(datapoints.size());

    for (size_t i = 0; i < datapoints.size(); ++i) {
      // Distance can be asymmetric, but the query is always on the right side
      SortedAllEntries_[i] = ResultEntry<dist_t>(datapoints[i]->id(),
                                                 datapoints[i]->label(),
                                                 space->IndexTimeDistance(datapoints[i], query));
    }

    wtm.split();

    SeqSearchTime_ = wtm.elapsed();

    std::sort(SortedAllEntries_.begin(), SortedAllEntries_.end());
  }

  uint64_t                            SeqSearchTime_;

  vector<ResultEntry<dist_t>>         SortedAllEntries_;
};


template <class dist_t>
class GoldStandardManager {
public:
  GoldStandardManager(const Space<dist_t>* space,
                      const ExperimentConfig<dist_t>&  config) :
                      space_(space),
                      config_(config),
                      vvGoldStandard_(config_.GetRange().size() +
                                      config_.GetKNN().size()) {}
  void Compute() {
    size_t gsIndx = 0;

    for (size_t i = 0; i < config_.GetRange().size(); ++i) {
      const dist_t radius = config_.GetRange()[i];
      RangeCreator<dist_t>  cr(radius);
      procOneSet(cr, vvGoldStandard_[gsIndx++]);
    }


    for (size_t i = 0; i < config_.GetKNN().size(); ++i) {
      const size_t K = config_.GetKNN()[i];
      KNNCreator<dist_t>  cr(K, config_.GetEPS());
      procOneSet(cr, vvGoldStandard_[gsIndx++]);
    }

  }
  void Read(istream& controlStream, istream& binaryStream,
            size_t queryQty) {
    for (size_t i = 0; i < vvGoldStandard_.size(); ++i) {
      for (size_t k = 0; k < queryQty; ++k) {
        GoldStandard<dist_t>  gs;
        gs.Read(controlStream, binaryStream);
        vvGoldStandard_[i].push_back(gs);
      }
    }
  }
  void Write(ostream& controlStream, ostream& binaryStream) {
    for (size_t i = 0; i < vvGoldStandard_.size(); ++i)
      for (auto obj : vvGoldStandard_[i]) obj.Write(controlStream, binaryStream);
  }
private:
  const Space<dist_t>*                    space_;
  const ExperimentConfig<dist_t>&         config_;
  vector<vector<GoldStandard<dist_t>>>    vvGoldStandard_;

  template <typename QueryCreatorType>
  void procOneSet(const QueryCreatorType&       QueryCreator,
                  vector<GoldStandard<dist_t>>& vGoldStand) {
    for (size_t q = 0; q < config_.GetQueryObjects().size(); ++q) {
      unique_ptr<Query<dist_t>> query(QueryCreator(config_.GetSpace(),
                                      config_.GetQueryObjects()[q]));
      vGoldStand.push_back(GoldStandard<dist_t>(config_.GetSpace(),
                                                config_.GetDataObjects(),
                                                query.get()));
    }

  }
};

}

#endif
