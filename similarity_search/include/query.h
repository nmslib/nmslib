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
#ifndef _QUERY_H_
#define _QUERY_H_

#include "object.h"
#include <iostream>
#include <vector>

namespace similarity {

template <typename dist_t>
class Space;


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
  void readBinary(std::istream& in) {
    in.read(reinterpret_cast<char*>(&mId),    sizeof mId);
    in.read(reinterpret_cast<char*>(&mLabel), sizeof mLabel);
    in.read(reinterpret_cast<char*>(&mDist),  sizeof mDist);
  }
  /*
   * Saves entry in the binary format, see the comment
   * on the endianness above.
   */
  void writeBinary(std::ostream& out) const {
    out.write(reinterpret_cast<const char*>(&mId),    sizeof mId);
    out.write(reinterpret_cast<const char*>(&mLabel), sizeof mLabel);
    out.write(reinterpret_cast<const char*>(&mDist),  sizeof mDist);
  }
  bool operator<(const ResultEntry& o) const {
    if (mDist != o.mDist) return mDist < o.mDist;
    return mId < o.mId;
  }
  bool operator==(const ResultEntry& o) const {
    return mId    == o.mId    &&
           mDist  == o.mDist  &&
           mLabel == o.mLabel;
  }
};

template <typename dist_t>
class Query {
 public:
  Query(const Space<dist_t>& space, const Object* query_object);
  virtual ~Query();

  const Object* QueryObject() const;
  uint64_t DistanceComputations() const;
  void AddDistanceComputations(uint64_t DistComp) { distance_computations_ += DistComp; }

  void ResetStats();
  virtual dist_t Distance(const Object* object1, const Object* object2) const;
  // Distance can be asymmetric!
  virtual dist_t DistanceObjLeft(const Object* object) const;
  virtual dist_t DistanceObjRight(const Object* object) const;

  virtual void Reset() = 0;
  virtual dist_t Radius() const = 0;
  virtual unsigned ResultSize() const = 0;
  virtual bool CheckAndAddToResult(const dist_t distance, const Object* object) = 0;
  virtual void Print() const = 0;
  virtual void getSortedResults(std::vector<ResultEntry<dist_t>>&res) const = 0;

 protected:
  const Space<dist_t>& space_;
  const Object* query_object_;
  mutable uint64_t distance_computations_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(Query);
};

}     // namespace similarity

#endif   // _QUERY_H_
