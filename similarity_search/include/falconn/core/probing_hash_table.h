#ifndef __PROBING_HASH_TABLE_H__
#define __PROBING_HASH_TABLE_H__

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "hash_table_helpers.h"

namespace falconn {
namespace core {

// A linear probing hash table that implements a multi-map (one key can have
// several values associated with it). Typically, the key is a locality
// sensitive hash and the values are the indices of points that were assigned
// this locality sensitive hash for this this table.
//
// This hash table is static, i.e., the inner array has a fixed size that does
// not change during after setting up the hash table. Moreover, the set of
// points has to be added with a single call. The static version of this table
// can be used when the point set of the overall LSH data structure is fixed.
// Since the static implementation is somewhat more efficient (both in terms
// of time and space), this can be a good idea for static points sets.
//
// See DynamicLinearProbingHashTable for a version of this class that allows
// (key, value) pairs to be added and deleted individually while resizing
// the hash table as necessary.

class StaticProbingHashTableError : public HashTableError {
 public:
  StaticProbingHashTableError(const char* msg) : HashTableError(msg) {}
};

template <typename KeyType,
          typename IndexType =
              int32_t>  // Integer type large enough to index into the
                        // table
class StaticLinearProbingHashTable {
 public:
  class Factory {
   public:
    Factory(int_fast64_t table_size) : table_size_(table_size) {
      if (table_size_ < 1) {
        throw StaticProbingHashTableError("Table size must be at least 1.");
      }
    }

    StaticLinearProbingHashTable<KeyType, IndexType>* new_hash_table() {
      return new StaticLinearProbingHashTable<KeyType, IndexType>(table_size_);
    }

   private:
    int_fast64_t table_size_ = 0;
  };

  typedef IndexType const* Iterator;

  StaticLinearProbingHashTable(int_fast64_t table_size)
      : table_size_(table_size) {}

  // TODO: refactor so that the table supports values other than 1 ... n
  // (which are currently implicit)
  void add_entries(const std::vector<KeyType>& keys) {
    if (table_size_ < 1) {
      throw StaticProbingHashTableError("Table size must be at least 1.");
    }
    if (entries_added_) {
      throw StaticProbingHashTableError("Entries already added.");
    }
    entries_added_ = true;

    TableEntry default_entry;
    default_entry.start = 0;
    default_entry.length = 0;
    table_.assign(table_size_, default_entry);

    KeyComparator comp(keys);
    indices_.resize(keys.size());
    for (IndexType ii = 0; ii < static_cast<IndexType>(indices_.size()); ++ii) {
      indices_[ii] = ii;
    }
    std::sort(indices_.begin(), indices_.end(), comp);

    IndexType cur_index = 0;
    while (cur_index < static_cast<IndexType>(indices_.size())) {
      IndexType end_index = cur_index;
      KeyType cur_hash = keys[indices_[cur_index]];
      do {
        end_index += 1;
      } while (end_index < static_cast<IndexType>(indices_.size()) &&
               keys[indices_[end_index]] == cur_hash);

      IndexType ii = find_free_entry(cur_hash);
      if (ii < 0) {
        throw StaticProbingHashTableError(
            "Free entry is negative (probably "
            "the table size is too small).");
      }
      table_[ii].start = cur_index;
      table_[ii].length = end_index - cur_index;
      table_[ii].key = cur_hash;
      cur_index = end_index;
    }
  }

  std::pair<Iterator, Iterator> retrieve(const KeyType& key) const {
    // hash and then probe
    IndexType cur_loc = hash(key);
    IndexType first_loc = cur_loc;
    bool passed_end = false;
    while (table_[cur_loc].length > 0) {
      if (passed_end && cur_loc == first_loc) {
        // nothing found
        return std::make_pair(&(indices_[0]), &(indices_[0]));
      }
      // check if match
      if (table_[cur_loc].key == key) {
        IndexType start = table_[cur_loc].start;
        IndexType end = start + table_[cur_loc].length;
        return std::make_pair(&(indices_[start]), &(indices_[end]));
      }
      // otherwise go to next entry
      cur_loc += 1;
      if (static_cast<int_fast64_t>(cur_loc) == table_size_) {
        cur_loc = 0;
        passed_end = true;
      }
    }
    // nothing found
    return std::make_pair(&(indices_[0]), &(indices_[0]));
  }

 private:
  // TODO: make the hash function a template argument
  IndexType hash(const KeyType& key) const {
    return (static_cast<uint_fast64_t>(key) * kLargePrime) % table_size_;
  }

  IndexType find_free_entry(const KeyType& key) const {
    // hash and then probe
    IndexType cur_loc = hash(key);
    IndexType first_loc = cur_loc;
    bool passed_end = false;
    while (table_[cur_loc].length > 0) {
      if (passed_end && cur_loc == first_loc) {
        // no empty spots
        return -1;
      }
      cur_loc += 1;
      if (cur_loc == table_size_) {
        cur_loc = 0;
        passed_end = true;
      }
    }
    return cur_loc;
  }

  struct TableEntry {
    KeyType key;
    IndexType start;
    IndexType length;
  };

  int_fast64_t table_size_ = 0;
  bool entries_added_ = false;
  // the pair contains start index and length
  std::vector<TableEntry> table_;
  // point indices
  std::vector<IndexType> indices_;

  // unsigned int to avoid negative values in the hash computation
  const uint_fast64_t kLargePrime = 2147483647;

  class KeyComparator {
   public:
    KeyComparator(const std::vector<KeyType>& keys) : keys_(keys) {}

    bool operator()(IndexType ii, IndexType jj) {
      return keys_[ii] < keys_[jj];
    }

    const std::vector<KeyType>& keys_;
  };
};

// The dynamic hash table uses the following hashing rules:
// On insertion:
//     If the number of active and deleted cells after insertion (i.e., all
//     non-empty cells) is larger than maximum_load * table_size, the entries
//     will be rehashed.
// On deletion:
//     If the fraction of deleted entries (relative to the entire table size)
//     is more than maximum_fraction_deleted after deletion, the entries will
//     be rehashed.
// Rehashing:
//     When rehashing the entries in the table, only the active entries will
//     be inserted in the new table. The size of the new table is the current
//     number of active entries * resizing_factor, and always at least 1.
//     NOTE: this does not always guarantee that the load of the table after
//     rehashing is less than maximum_load (e.g., consider a table with a single
//     active element, initial size 1, maximum_load = 0.1, and resizing_factor
//     2). However, for reasonable values of maximum_load and resizing_factor,
//     the table load after rehashing will be less than maximum_load when the
//     number of active entries is large enough.

class DynamicProbingHashTableError : public HashTableError {
 public:
  DynamicProbingHashTableError(const char* msg) : HashTableError(msg) {}
};

template <typename KeyType, typename ValueType = int32_t,
          typename IndexType =
              int32_t>  // Integer type large enough to index into the
                        // table
class DynamicLinearProbingHashTable {
 public:
  static void check_parameters(double maximum_load,
                               double maximum_fraction_deleted,
                               double resizing_factor, IndexType initial_size) {
    if (maximum_load >= 1.0) {
      throw DynamicProbingHashTableError(
          "Maximum hash table load must be less "
          "than 1.0.");
    }
    if (maximum_load <= 0.0) {
      throw DynamicProbingHashTableError(
          "Maximum hash table load must be "
          "larger than 0.0.");
    }
    if (maximum_fraction_deleted >= 1.0) {
      throw DynamicProbingHashTableError(
          "Maximum hash table fraction of "
          "deleted entries must be less than "
          "1.0.");
    }
    if (resizing_factor <= 1.0) {
      throw DynamicProbingHashTableError(
          "Hash table resizing factor must be "
          "greater than 1.0.");
    }
    if (initial_size < 1) {
      throw DynamicProbingHashTableError(
          "Initial table size must be at "
          "least 1.");
    }
  }

  class Factory {
   public:
    Factory(double maximum_load, double maximum_fraction_deleted,
            double resizing_factor, IndexType initial_size)
        : maximum_load_(maximum_load),
          maximum_fraction_deleted_(maximum_fraction_deleted),
          resizing_factor_(resizing_factor),
          initial_size_(initial_size) {
      DynamicLinearProbingHashTable<KeyType, ValueType>::check_parameters(
          maximum_load_, maximum_fraction_deleted_, resizing_factor_,
          initial_size_);
    }

    DynamicLinearProbingHashTable<KeyType, ValueType>* new_hash_table() {
      return new DynamicLinearProbingHashTable<KeyType, ValueType>(
          maximum_load_, maximum_fraction_deleted_, resizing_factor_,
          initial_size_);
    }

   private:
    double maximum_load_;
    double maximum_fraction_deleted_;
    double resizing_factor_;
    IndexType initial_size_;
  };

  // TODO: derive from std::iterator
  // TODO: change this so that iterator takes a reference to the key? (revisit
  //       when multithreaded version is more clear.)
  class Iterator {
   public:
    Iterator() : cur_loc_(), key_(), parent_(nullptr) {}

    Iterator(IndexType cur_loc, KeyType key,
             DynamicLinearProbingHashTable const* parent)
        : cur_loc_(cur_loc), key_(key), parent_(parent) {}

    ValueType operator*() const { return parent_->table_[cur_loc_].value; }

    bool operator!=(const Iterator& iter) const {
      if (parent_ == iter.parent_) {
        if (parent_ == nullptr) {
          return false;
        } else {
          return cur_loc_ != iter.cur_loc_;
        }
      } else {
        return true;
      }
    }

    bool operator==(const Iterator& iter) const { return !(*this != iter); }

    Iterator& operator++() {
      cur_loc_ += 1;
      if (cur_loc_ == static_cast<IndexType>(parent_->table_.size())) {
        cur_loc_ = 0;
      }
      while (parent_->table_[cur_loc_].state != kEmpty) {
        if (parent_->table_[cur_loc_].state != kDeleted &&
            parent_->table_[cur_loc_].key == key_) {
          return *this;
        }
        cur_loc_ += 1;
        if (cur_loc_ == static_cast<IndexType>(parent_->table_.size())) {
          cur_loc_ = 0;
        }
      }
      parent_ = nullptr;
      return *this;
    }

   private:
    IndexType cur_loc_;
    KeyType key_;
    DynamicLinearProbingHashTable const* parent_;
  };

  DynamicLinearProbingHashTable(double maximum_load,
                                double maximum_fraction_deleted,
                                double resizing_factor,
                                int_fast64_t initial_size)
      : maximum_load_(maximum_load),
        maximum_fraction_deleted_(maximum_fraction_deleted),
        resizing_factor_(resizing_factor) {
    TableEntry default_entry;
    default_entry.key = 0;
    default_entry.value = 0;
    default_entry.state = kEmpty;
    check_parameters(maximum_load_, maximum_fraction_deleted_, resizing_factor_,
                     initial_size);
    table_.assign(initial_size, default_entry);
  }

  void insert(const KeyType& key, const ValueType& value) {
    // This rule also re-hashes if the index is already in the table.
    // We accept this sloppiness because re-insertions should never
    // happen in the first place.
    // printf("In insert for index %d\n", index);
    // printf("  table size: %lu\n", table_.size());

    IndexType cur_loc = hash(key);
    while (table_[cur_loc].state == kActive &&
           (table_[cur_loc].key != key || table_[cur_loc].value != value)) {
      cur_loc += 1;
      if (cur_loc == static_cast<IndexType>(table_.size())) {
        cur_loc = 0;
      }
    }

    // Add new entry if we encounter an empty field.
    // Overwrite deleted entry if we encounter a previously deleted field.
    // Otherwise, the entry already exists.
    if (table_[cur_loc].state == kEmpty) {
      table_[cur_loc].state = kActive;
      table_[cur_loc].value = value;
      table_[cur_loc].key = key;
      num_active_entries_ += 1;
    } else if (table_[cur_loc].state == kDeleted) {
      table_[cur_loc].state = kActive;
      table_[cur_loc].value = value;
      table_[cur_loc].key = key;
      num_active_entries_ += 1;
      num_deleted_entries_ -= 1;
    } else {
      throw DynamicProbingHashTableError(
          "key-value pair already exists in the "
          "hash table.");
    }

    if (static_cast<double>(num_active_entries_ + num_deleted_entries_) /
            table_.size() >
        maximum_load_) {
      // printf("  rehashing ...\n");
      rehash();
      // printf("    done\n");
      // printf("  table size after rehash: %lu\n", table_.size());
    }
  }

  void remove(const KeyType& key, const ValueType& value) {
    // printf("In remove for index %d\n", index);
    // printf("  table size: %lu\n", table_.size());
    IndexType cur_loc = hash(key);
    while (table_[cur_loc].state == kDeleted ||
           (table_[cur_loc].state == kActive &&
            (table_[cur_loc].key != key || table_[cur_loc].value != value))) {
      cur_loc += 1;
      if (cur_loc == static_cast<IndexType>(table_.size())) {
        cur_loc = 0;
      }
    }

    if (table_[cur_loc].state == kEmpty) {
      throw DynamicProbingHashTableError(
          "Entry does not exist in the hash "
          "table.");
    } else {
      table_[cur_loc].state = kDeleted;
      num_deleted_entries_ += 1;
      num_active_entries_ -= 1;
    }

    if (static_cast<double>(num_deleted_entries_) / table_.size() >
        maximum_fraction_deleted_) {
      rehash();
      // printf("  table size after rehash: %lu\n", table_.size());
    }
  }

  std::pair<Iterator, Iterator> retrieve(const KeyType& key) const {
    // printf("In retrieve %d\n", lsh_hash);
    IndexType cur_loc = hash(key);
    while (table_[cur_loc].state != kEmpty) {
      if (table_[cur_loc].state != kDeleted && table_[cur_loc].key == key) {
        // printf("  returning valid iterator.\n");
        return std::make_pair(Iterator(cur_loc, table_[cur_loc].key, this),
                              Iterator());
      }
      cur_loc += 1;
      if (cur_loc == static_cast<IndexType>(table_.size())) {
        cur_loc = 0;
      }
    }
    // printf("  returning end iterator.\n");
    return std::make_pair(Iterator(), Iterator());
  }

  IndexType get_table_size() { return table_.size(); }

 private:
  IndexType hash(const KeyType& key) const { return hash(key, table_.size()); }

  IndexType hash(const KeyType& key, uint_fast64_t table_size) const {
    return (static_cast<uint_fast64_t>(key) * kLargePrime) % table_size;
  }

  enum EntryState {
    kEmpty,
    kActive,
    kDeleted,
  };

  struct TableEntry {
    KeyType key;
    ValueType value;
    EntryState state;
  };

  IndexType num_active_entries_ = 0;
  IndexType num_deleted_entries_ = 0;
  double maximum_load_;
  double maximum_fraction_deleted_;
  double resizing_factor_;

  std::vector<TableEntry> table_;

  const uint_fast64_t kLargePrime = 2147483647;

  void rehash() {
    // printf("In rehash --------------\n");
    IndexType new_size = num_active_entries_;
    new_size = std::max(std::ceil(new_size * resizing_factor_), 1.0);
    if (new_size <= num_active_entries_) {
      throw DynamicProbingHashTableError(
          "Resize did not lead to an empty "
          "cell, increase the resizing factor.");
    }

    std::vector<TableEntry> new_table;
    TableEntry default_entry;
    default_entry.key = 0;
    default_entry.value = 0;
    default_entry.state = kEmpty;
    new_table.assign(new_size, default_entry);

    for (IndexType ii = 0; ii < static_cast<IndexType>(table_.size()); ++ii) {
      if (table_[ii].state == kActive) {
        IndexType cur_loc = hash(table_[ii].key, new_size);
        while (new_table[cur_loc].state == kActive) {
          cur_loc += 1;
          if (cur_loc == static_cast<IndexType>(new_table.size())) {
            cur_loc = 0;
          }
        }

        new_table[cur_loc].state = kActive;
        new_table[cur_loc].value = table_[ii].value;
        new_table[cur_loc].key = table_[ii].key;
      }
    }

    table_.swap(new_table);
    num_deleted_entries_ = 0;
  }
};

}  // namespace core
}  // namespace falconn

#endif
