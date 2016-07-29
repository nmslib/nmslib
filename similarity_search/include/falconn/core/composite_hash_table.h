#ifndef __COMPOSITE_HASH_TABLE_H__
#define __COMPOSITE_HASH_TABLE_H__

#include <memory>
#include <utility>
#include <vector>

#include "hash_table_helpers.h"

namespace falconn {
namespace core {

class CompositeHashTableError : public HashTableError {
 public:
  CompositeHashTableError(const char* msg) : HashTableError(msg) {}
};

// Note that the KeyType here is usually the HashType in the LSH table and the
// ValueType is usually the KeyType of the LSH table.
template <typename KeyType, typename ValueType, typename InnerHashTable>
class BasicCompositeHashTable {
 public:
  class Iterator {
   public:
    typedef typename std::pair<typename InnerHashTable::Iterator,
                               typename InnerHashTable::Iterator>
        InnerIteratorPair;

    Iterator(int_fast32_t cur_table, int_fast32_t cur_key_index,
             InnerIteratorPair cur_iterators,
             const std::vector<std::vector<KeyType>>& keys,
             const BasicCompositeHashTable& parent)
        : cur_table_(cur_table),
          cur_key_index_(cur_key_index),
          cur_iterators_(cur_iterators),
          keys_(&keys),
          parent_(&parent) {}

    Iterator()
        : cur_table_(-1),
          cur_key_index_(-1),
          cur_iterators_(),
          keys_(nullptr),
          parent_(nullptr) {}

    ValueType operator*() const { return *(cur_iterators_.first); }

    bool operator!=(const Iterator& iter) const {
      if (parent_ == iter.parent_) {
        if (parent_ == nullptr) {
          return false;
        }
        return (cur_table_ != iter.cur_table_) ||
               (cur_key_index_ != iter.cur_key_index_) ||
               (cur_iterators_ != iter.cur_iterators_);
      } else {
        return true;
      }
    }

    Iterator& operator++() {
      ++cur_iterators_.first;
      if (cur_iterators_.first != cur_iterators_.second) {
        return *this;
      } else {
        while (true) {
          ++cur_key_index_;

          while (cur_key_index_ >=
                 static_cast<int_fast32_t>((*keys_)[cur_table_].size())) {
            cur_table_ += 1;
            cur_key_index_ = 0;
            if (cur_table_ ==
                static_cast<int_fast32_t>(parent_->tables_.size())) {
              parent_ = nullptr;
              return *this;
            }
          }

          cur_iterators_ = parent_->tables_[cur_table_]->retrieve(
              ((*keys_)[cur_table_][cur_key_index_]));
          if (cur_iterators_.first != cur_iterators_.second) {
            return *this;
          }
        }
      }
    }

   private:
    int_fast32_t cur_table_;
    int_fast32_t cur_key_index_;
    InnerIteratorPair cur_iterators_;
    const std::vector<std::vector<KeyType>>* keys_;
    const BasicCompositeHashTable* parent_;
  };

  BasicCompositeHashTable(int_fast32_t l,
                          typename InnerHashTable::Factory* factory)
      : l_(l), factory_(factory) {
    tables_.resize(l_);
    for (int_fast32_t ii = 0; ii < l_; ++ii) {
      tables_[ii].reset(factory_->new_hash_table());
      if (!tables_[ii]) {
        throw CompositeHashTableError(
            "Error while setting up the "
            "low-level hash tables.");
      }
    }
  }

  int_fast32_t get_l() { return l_; }

  std::pair<Iterator, Iterator> retrieve_bulk(
      const std::vector<std::vector<KeyType>>& keys) const {
    return std::make_pair(make_first_iterator(keys), Iterator());
  }

  std::pair<typename InnerHashTable::Iterator,
            typename InnerHashTable::Iterator>
  retrieve_individual(const KeyType& key, int_fast32_t table) const {
    return tables_[table]->retrieve(key);
  }

 protected:
  int_fast32_t l_;
  typename InnerHashTable::Factory* factory_;
  std::vector<std::unique_ptr<InnerHashTable>> tables_;

  Iterator make_first_iterator(
      const std::vector<std::vector<KeyType>>& keys) const {
    for (size_t table = 0; table < tables_.size(); ++table) {
      for (size_t key_index = 0; key_index < keys[table].size(); ++key_index) {
        auto iters = tables_[table]->retrieve(keys[table][key_index]);
        if (iters.first != iters.second) {
          return Iterator(table, key_index, iters, keys, *this);
        }
      }
    }
    return Iterator();
  }
};

template <typename KeyType, typename ValueType, typename InnerHashTable>
class StaticCompositeHashTable
    : public BasicCompositeHashTable<KeyType, ValueType, InnerHashTable> {
 public:
  StaticCompositeHashTable(int_fast32_t l,
                           typename InnerHashTable::Factory* factory)
      : BasicCompositeHashTable<KeyType, ValueType, InnerHashTable>(l,
                                                                    factory) {}

  void add_entries_for_table(const std::vector<KeyType>& keys,
                             int_fast32_t table) {
    if (table < 0 || table >= this->l_) {
      throw CompositeHashTableError("Table index incorrect.");
    }

    this->tables_[table]->add_entries(keys);
  }
};

template <typename KeyType, typename ValueType, typename InnerHashTable>
class DynamicCompositeHashTable
    : public BasicCompositeHashTable<KeyType, ValueType, InnerHashTable> {
 public:
  DynamicCompositeHashTable(int_fast32_t l,
                            typename InnerHashTable::Factory* factory)
      : BasicCompositeHashTable<KeyType, ValueType, InnerHashTable>(l,
                                                                    factory) {}

  void insert(const std::vector<KeyType>& keys, ValueType value) {
    if (static_cast<int_fast32_t>(keys.size()) != this->l_) {
      throw CompositeHashTableError("Number of keys in insert incorrect.");
    }
    for (int_fast32_t ii = 0; ii < this->l_; ++ii) {
      (this->tables_[ii])->insert(keys[ii], value);
    }
  }

  void remove(const std::vector<KeyType>& keys, ValueType value) {
    if (static_cast<int_fast32_t>(keys.size()) != this->l_) {
      throw CompositeHashTableError("Number of hashes in remove incorrect.");
    }
    for (int_fast32_t ii = 0; ii < this->l_; ++ii) {
      (this->tables_[ii])->remove(keys[ii], value);
    }
  }
};

}  // namespace core
}  // namespace falconn

#endif
