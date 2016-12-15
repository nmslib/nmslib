#ifndef __FLAT_HASH_TABLE_H__
#define __FLAT_HASH_TABLE_H__

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "hash_table_helpers.h"

namespace falconn {
namespace core {

class FlatHashTableError : public HashTableError {
 public:
  FlatHashTableError(const char* msg) : HashTableError(msg) {}
};

template <typename KeyType, typename ValueType = int32_t,
          typename IndexType = int32_t>
class FlatHashTable {
 public:
  class Factory {
   public:
    Factory(IndexType num_buckets) : num_buckets_(num_buckets) {
      if (num_buckets_ < 1) {
        throw FlatHashTableError("Number of buckets must be at least 1.");
      }
    }

    FlatHashTable<KeyType, ValueType, IndexType>* new_hash_table() {
      return new FlatHashTable<KeyType, ValueType, IndexType>(num_buckets_);
    }

   private:
    IndexType num_buckets_ = 0;
  };

  typedef IndexType* Iterator;

  FlatHashTable(IndexType num_buckets) : num_buckets_(num_buckets) {}

  // TODO: add version with explicit values array? (maybe not because the flat
  // hash table is arguably most useful for the static table setting?)
  void add_entries(const std::vector<KeyType>& keys) {
    if (num_buckets_ <= 0) {
      throw FlatHashTableError("Non-positive number of buckets");
    }
    if (entries_added_) {
      throw FlatHashTableError("Entries were already added.");
    }
    bucket_list_.resize(num_buckets_, std::make_pair(0, 0));

    entries_added_ = true;

    KeyComparator comp(keys);
    indices_.resize(keys.size());
    for (IndexType ii = 0; static_cast<size_t>(ii) < indices_.size(); ++ii) {
      if (keys[ii] >= static_cast<KeyType>(num_buckets_) || keys[ii] < 0) {
        throw FlatHashTableError("Key value out of range.");
      }
      indices_[ii] = ii;
    }
    std::sort(indices_.begin(), indices_.end(), comp);

    IndexType cur_index = 0;
    while (cur_index < static_cast<IndexType>(indices_.size())) {
      IndexType end_index = cur_index;
      do {
        end_index += 1;
      } while (end_index < static_cast<IndexType>(indices_.size()) &&
               keys[indices_[cur_index]] == keys[indices_[end_index]]);

      bucket_list_[keys[indices_[cur_index]]].first = cur_index;
      bucket_list_[keys[indices_[cur_index]]].second = end_index - cur_index;
      cur_index = end_index;
    }
  }

  std::pair<Iterator, Iterator> retrieve(const KeyType& key) {
    IndexType start = bucket_list_[key].first;
    IndexType len = bucket_list_[key].second;
    // printf("retrieve for key %u\n", key);
    // printf("  start: %lld  len %lld\n", start, len);
    return std::make_pair(&(indices_[start]), &(indices_[start + len]));
  }

 private:
  IndexType num_buckets_ = -1;
  bool entries_added_ = false;
  // the pair contains start index and length
  std::vector<std::pair<IndexType, IndexType>> bucket_list_;
  // point indices
  std::vector<ValueType> indices_;

  class KeyComparator {
   public:
    KeyComparator(const std::vector<KeyType>& keys) : keys_(keys) {}

    bool operator()(IndexType ii, IndexType jj) {
      return keys_[ii] < keys_[jj];
    }

    const std::vector<KeyType>& keys_;
  };
};

}  // namespace core
}  // namespace falconn

#endif
