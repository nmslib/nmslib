#ifndef __BIT_PACKED_FLAT_HASH_TABLE_H__
#define __BIT_PACKED_FLAT_HASH_TABLE_H__

#include <algorithm>
#include <cassert>
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
        bucket_start_(num_buckets, log2ceil(num_items + 1)),
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

    std::vector<IndexType> bucket_counts(num_buckets_, 0);
    for (IndexType ii = 0; ii < static_cast<IndexType>(keys.size()); ++ii) {
      if (static_cast<IndexType>(keys[ii]) >= num_buckets_ || keys[ii] < 0) {
        throw BitPackedFlatHashTableError("Key value out of range.");
      }
      bucket_counts[keys[ii]] += 1;
    }

    bucket_start_.set(0, 0);
    for (IndexType ii = 1; ii < num_buckets_; ++ii) {
      bucket_start_.set(ii, bucket_start_.get(ii - 1) + bucket_counts[ii - 1]);
    }

    for (IndexType ii = static_cast<IndexType>(keys.size()) - 1; ii >= 0;
         --ii) {
      KeyType cur_key = keys[ii];
      indices_.set(bucket_start_.get(cur_key) + bucket_counts[cur_key] - 1, ii);
      bucket_counts[cur_key] -= 1;
    }
  }

  std::pair<Iterator, Iterator> retrieve(const KeyType& key) {
    ValueType start = bucket_start_.get(key);
    ValueType end = num_items_;
    if (static_cast<IndexType>(key) < num_buckets_ - 1) {
      end = bucket_start_.get(key + 1);
    }
    assert(start <= end);
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
