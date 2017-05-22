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
#ifndef INMEM_INV_INDEX_H
#define INMEM_INV_INDEX_H

#include <cstdint>
#include <vector>
#include <unordered_map>

namespace similarity {

typedef uint32_t WORD_ID_TYPE;

using std::unordered_map;
using std::vector;

struct SimpleInvEntry {
  SimpleInvEntry(WORD_ID_TYPE id = 0, float weight = 0) : id_(id), weight_(weight) {}
  WORD_ID_TYPE  id_;
  float         weight_;
  bool operator<(const SimpleInvEntry& o) const {
    if (id_ != o.id_) return id_ < o.id_;
    return weight_ > o.weight_;
  }
};

class InMemInvIndex {
public:
  const vector<SimpleInvEntry>* getDict(WORD_ID_TYPE wordId) const {
    const auto it = dict_.find(wordId);
    return it == dict_.end() ? nullptr : &it->second;
  }
  // addEntry doesn't check for duplicates, it is supposed
  // to be a responsibility of the calling code
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
