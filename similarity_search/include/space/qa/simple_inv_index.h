/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2015
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef SIMPLE_INV_INDEX_H
#define SIMPLE_INV_INDEX_H

#include <vector>
#include <unordered_map>

#include "space/qa/simil.h"

namespace similarity {

using std::unordered_map;
using std::vector;

struct SimpleInvEntry {
  SimpleInvEntry(uint32_t id = 0, float weight = 0, uint32_t answRepQty = 0) : id_(id), weight_(weight), answRepQty_(answRepQty) {}
  uint32_t    id_;
  float       weight_;
  uint32_t    answRepQty_;
  bool operator<(const SimpleInvEntry& o) const {
    if (id_ != o.id_) return id_ < o.id_;
    return weight_ > o.weight_;
  }
};

class SimpleInvIndex {
public:
  const vector<SimpleInvEntry>* getDict(WORD_ID_TYPE wordId) const {
    const auto it = dict_.find(wordId);
    return it == dict_.end() ? nullptr : &it->second;
  }
  // addEntry doesn't check for duplicates, it is supposed
  // to be headache of the calling code
  void addEntry(WORD_ID_TYPE wordId, const SimpleInvEntry& e) {
    const auto it = dict_.find(wordId);
    if (it == dict_.end()) {
      vector<SimpleInvEntry> v = { e };
      dict_.insert(make_pair(wordId, v));
    } else {
      it->second.push_back(e);
    }
  }
  void sort() {
    for (auto it = dict_.begin(); it != dict_.end(); ++it) {
      std::sort(it->second.begin(), it->second.end());
    }
  }
private:
  unordered_map<WORD_ID_TYPE, vector<SimpleInvEntry>>   dict_;
};

}
#endif
