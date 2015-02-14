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

#define SEQ_SEARCH_TIME        "SeqSearchTime"
#define SEQ_GS_QTY             "GoldStandQty"
#define GS_NOTE_FIELD          "Note"
#define GS_TEST_SET_ID         "TestSetId"

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
  void writeBinary(ostream& out) const {
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
              const typename similarity::Query<dist_t>* query,
              size_t maxKeepEntryQty
              ) {
    DoSeqSearch(space, datapoints, query->QueryObject());
    if (maxKeepEntryQty != 0) {
      vector<ResultEntry<dist_t>>         tmp(SortedAllEntries_.begin(),
                                             SortedAllEntries_.begin() + maxKeepEntryQty);
      /* 
       * The swap trick actually leads to memory being freed.
       * If we simply erase the extra entries, vector still keeps the memory!
       */
      SortedAllEntries_.swap(tmp);
    }
  }
  /*
   * See the endianness comment.
   */
  void Write(ostream& controlStream, ostream& binaryStream,
             size_t MaxCacheGSQty) const {
    size_t qty = min(MaxCacheGSQty, SortedAllEntries_.size());
    WriteField(controlStream, SEQ_SEARCH_TIME, ConvertToString(SeqSearchTime_));
    WriteField(controlStream, SEQ_GS_QTY, ConvertToString(qty));
    for (size_t i = 0; i < qty; // Note that qty <= SortedAllEntries_.size()
         ++i) {
      SortedAllEntries_[i].writeBinary(binaryStream);
    }
  }

  void Read(istream& controlStream, istream& binaryStream) {
    string s;
    ReadField(controlStream, SEQ_SEARCH_TIME, s);
    ConvertFromString(s, SeqSearchTime_);
    ReadField(controlStream, SEQ_GS_QTY, s);
    size_t qty = 0;
    ConvertFromString(s, qty);
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
  GoldStandardManager(const ExperimentConfig<dist_t>&  config) :
                      config_(config),
                      vvGoldStandardRange_(config_.GetRange().size()),
                      vvGoldStandardKNN_(config_.GetKNN().size()) {}
  // Both read and Compute can be called multiple times
  // if maxKeepEntryQty is non-zero, we keep only maxKeepEntryQty GS entries
  void Compute(size_t maxKeepEntryQty) {
    LOG(LIB_INFO) << "Computing gold standard data, at most " << maxKeepEntryQty << " entries";;
    for (size_t i = 0; i < config_.GetRange().size(); ++i) {
      vvGoldStandardRange_[i].clear();
      const dist_t radius = config_.GetRange()[i];
      RangeCreator<dist_t>  cr(radius);
      procOneSet(cr, vvGoldStandardRange_[i], maxKeepEntryQty);
    }
    for (size_t i = 0; i < config_.GetKNN().size(); ++i) {
      vvGoldStandardKNN_[i].clear();
      const size_t K = config_.GetKNN()[i];
      KNNCreator<dist_t>  cr(K, config_.GetEPS());
      procOneSet(cr, vvGoldStandardKNN_[i], maxKeepEntryQty);
    }
  }
  void Read(istream& controlStream, istream& binaryStream,
            size_t queryQty,
            size_t& testSetId) {
    LOG(LIB_INFO) << "Reading gold standard data from cache";
    string s;
    ReadField(controlStream, GS_TEST_SET_ID, s);
    ConvertFromString(s, testSetId);
    for (size_t i = 0; i < vvGoldStandardRange_.size(); ++i) {
      ReadField(controlStream, GS_NOTE_FIELD, s);
      vvGoldStandardRange_[i].clear();
      readOneGS(controlStream, binaryStream,
                queryQty, vvGoldStandardRange_[i]);
    }
    for (size_t i = 0; i < vvGoldStandardKNN_.size(); ++i) {
      ReadField(controlStream, GS_NOTE_FIELD, s);
      vvGoldStandardKNN_[i].clear();
      readOneGS(controlStream, binaryStream,
                queryQty, vvGoldStandardKNN_[i]);
    }
  }
  void Write(ostream& controlStream, ostream& binaryStream,
             size_t testSetId, size_t MaxCacheGSQty) {
    WriteField(controlStream, GS_TEST_SET_ID, ConvertToString(testSetId));
    // GS_NOTE_FIELD & GS_TEST_SET_ID are for informational purposes only
    for (size_t i = 0; i < vvGoldStandardRange_.size(); ++i) {
      WriteField(controlStream, GS_NOTE_FIELD,
                "range radius=" + ConvertToString(config_.GetRange()[i]));
      writeOneGS(controlStream, binaryStream,
                 vvGoldStandardRange_[i], MaxCacheGSQty);
    }
    for (size_t i = 0; i < vvGoldStandardKNN_.size(); ++i) {
      WriteField(controlStream, GS_NOTE_FIELD,
                "k=" + ConvertToString(config_.GetKNN()[i]) +
                "eps=" + ConvertToString(config_.GetEPS()));
      writeOneGS(controlStream, binaryStream,
                 vvGoldStandardKNN_[i], MaxCacheGSQty);
    }
  }
  const vector<GoldStandard<dist_t>> &GetRangeGS(size_t i) const {
    return vvGoldStandardRange_[i];
  }
  const vector<GoldStandard<dist_t>> &GetKNNGS(size_t i) const {
    return vvGoldStandardKNN_[i];
  }
private:
  const ExperimentConfig<dist_t>&         config_;
  vector<vector<GoldStandard<dist_t>>>    vvGoldStandardRange_;
  vector<vector<GoldStandard<dist_t>>>    vvGoldStandardKNN_;

  void writeOneGS(ostream& controlStream, ostream& binaryStream,
                  const vector<GoldStandard<dist_t>>& oneGS,
                  size_t MaxCacheGSQty) {
    for(const GoldStandard<dist_t>& obj : oneGS) {
      obj.Write(controlStream, binaryStream, MaxCacheGSQty);
    }
  }

  void readOneGS(istream& controlStream, istream& binaryStream,
                 size_t queryQty,
                 vector<GoldStandard<dist_t>>& oneGS) {
    for (size_t k = 0; k < queryQty; ++k) {
      GoldStandard<dist_t>  gs;
      gs.Read(controlStream, binaryStream);
      oneGS.push_back(gs);
    }
  }

  template <typename QueryCreatorType>
  void procOneSet(const QueryCreatorType&       QueryCreator,
                  vector<GoldStandard<dist_t>>& vGoldStand,
                  size_t maxKeepEntryQty) {
    for (size_t q = 0; q < config_.GetQueryObjects().size(); ++q) {
      unique_ptr<Query<dist_t>> query(QueryCreator(config_.GetSpace(),
                                      config_.GetQueryObjects()[q]));
      vGoldStand.push_back(GoldStandard<dist_t>(config_.GetSpace(),
                                                config_.GetDataObjects(),
                                                query.get(),
                                                maxKeepEntryQty));
    }

  }
};

}

#endif
