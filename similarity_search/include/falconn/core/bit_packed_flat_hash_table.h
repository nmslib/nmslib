#ifndef __BIT_PACKED_FLAT_HASH_TABLE_H__
#define __BIT_PACKED_FLAT_HASH_TABLE_H__

#include <algorithm>
#include <vector>

#include "bit_packed_vector.h"
#include "hash_table_helpers.h"
#include "math_helpers.h"

namespace falconn {
namespace core {

class BitPackedFlatHashTableError : public HashTableError {
 public:
  BitPackedFlatHashTableError(const char* msg) : HashTableError(msg) {}
};

template <typename KeyType, typename ValueType = int_fast64_t,
          typename IndexType = int_fast64_t>
class BitPackedFlatHashTable {
 public:
  class Factory {
   public:
    Factory(IndexType num_buckets, ValueType num_items)
        : num_buckets_(num_buckets), num_items_(num_items) {
      if (num_buckets_ < 1) {
        throw BitPackedFlatHashTableError(
            "Number of buckets must be at least 1.");
      }
      if (num_items_ < 1) {
        throw BitPackedFlatHashTableError(
            "Number of items must be at least 1.");
      }
    }

    BitPackedFlatHashTable<KeyType, ValueType, IndexType>* new_hash_table() {
      return new BitPackedFlatHashTable<KeyType, ValueType, IndexType>(
          num_buckets_, num_items_);
    }

   private:
    IndexType num_buckets_ = 0;
    ValueType num_items_ = 0;
  };

  class Iterator {
   public:
    Iterator() : index_(0), parent_(nullptr) {}

    Iterator(ValueType index, const BitPackedFlatHashTable* parent)
        : index_(index), parent_(parent) {}

    ValueType operator*() const { return parent_->indices_.get(index_); }

    bool operator!=(const Iterator& iter) const {
      if (parent_ != iter.parent_) {
        return false;
      } else {
        return index_ != iter.index_;
      }
    }

    bool operator==(const Iterator& iter) const { return !(*this != iter); }

    Iterator& operator++() {
      index_ += 1;
      return *this;
    }

   private:
    ValueType index_;
    const BitPackedFlatHashTable* parent_;
  };

  BitPackedFlatHashTable(IndexType num_buckets, ValueType num_items)
      : num_buckets_(num_buckets),
        num_items_(num_items),
        bucket_start_(num_buckets, log2ceil(num_items)),
        indices_(num_items, log2ceil(num_items)) {
    // printf("num_buckets = %d  num_items_ = %d\n", num_buckets_, num_items_);
    if (num_buckets_ < 1) {
      throw BitPackedFlatHashTableError(
          "Number of buckets must be at least 1.");
    }
    if (num_items_ < 1) {
      throw BitPackedFlatHashTableError("Number of items must be at least 1.");
    }
  }

  void add_entries(const std::vector<KeyType>& keys) {
    if (entries_added_) {
      throw BitPackedFlatHashTableError("Entries were already added.");
    }
    entries_added_ = true;
    if (static_cast<ValueType>(keys.size()) != num_items_) {
      throw BitPackedFlatHashTableError(
          "Incorrect number of items in add_entries.");
    }

    KeyComparator comp(keys);
    std::vector<ValueType> tmp_indices(keys.size());
    for (IndexType ii = 0; ii < static_cast<IndexType>(tmp_indices.size());
         ++ii) {
      if (static_cast<IndexType>(keys[ii]) >= num_buckets_ || keys[ii] < 0) {
        throw BitPackedFlatHashTableError("Key value out of range.");
      }
      tmp_indices[ii] = ii;
    }
    std::sort(tmp_indices.begin(), tmp_indices.end(), comp);

    for (IndexType ii = 0; ii < static_cast<IndexType>(tmp_indices.size());
         ++ii) {
      indices_.set(ii, tmp_indices[ii]);
    }

    IndexType cur_index = 0;
    std::vector<bool> bucket_empty(num_buckets_, true);

    while (cur_index < static_cast<IndexType>(tmp_indices.size())) {
      IndexType end_index = cur_index;
      do {
        end_index += 1;
      } while (end_index < static_cast<IndexType>(tmp_indices.size()) &&
               keys[tmp_indices[cur_index]] == keys[tmp_indices[end_index]]);

      bucket_start_.set(keys[tmp_indices[cur_index]], cur_index);
      bucket_empty[keys[tmp_indices[cur_index]]] = false;
      cur_index = end_index;
    }

    if (bucket_empty[num_buckets_ - 1]) {
      bucket_start_.set(num_buckets_ - 1, num_items_);
    }
    for (IndexType ii = num_buckets_ - 2; ii >= 0; --ii) {
      if (bucket_empty[ii]) {
        bucket_start_.set(ii, bucket_start_.get(ii + 1));
      }
    }
  }

  std::pair<Iterator, Iterator> retrieve(const KeyType& key) {
    ValueType start = bucket_start_.get(key);
    ValueType end = num_items_;
    if (static_cast<IndexType>(key) < num_buckets_ - 1) {
      end = bucket_start_.get(key + 1);
    }
    // printf("retrieve for key %u\n", key);
    // printf("  start: %lld  end %lld\n", start, end);
    return std::make_pair(Iterator(start, this), Iterator(end, this));
  }

 private:
  IndexType num_buckets_ = 0;
  ValueType num_items_ = 0;
  bool entries_added_ = false;

  // start of the respective hash bucket
  BitPackedVector<ValueType> bucket_start_;
  // point indices
  BitPackedVector<ValueType> indices_;

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
