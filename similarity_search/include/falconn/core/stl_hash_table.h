#ifndef __STL_HASH_TABLE_H__
#define __STL_HASH_TABLE_H__

#include <unordered_map>
#include <utility>
#include <vector>

namespace falconn {
namespace core {

template <typename KeyType, typename ValueType = int32_t,
          typename IndexType = int_fast64_t>
class STLHashTable {
 public:
  class Factory {
   public:
    STLHashTable<KeyType, ValueType, IndexType>* new_hash_table() {
      return new STLHashTable<KeyType, ValueType, IndexType>();
    }
  };

  class Iterator {
   public:
    typedef typename std::unordered_multimap<KeyType, ValueType>::iterator
        InnerIterator;
    Iterator() {}

    Iterator(const InnerIterator& iter) : iter_(iter) {}

    ValueType operator*() const { return iter_->second; }

    Iterator& operator++() {
      ++iter_;
      return *this;
    }

    bool operator!=(const Iterator& iter) const { return iter_ != iter.iter_; }

    bool operator==(const Iterator& iter) const { return iter_ == iter.iter_; }

   private:
    InnerIterator iter_;
  };

  void add_entries(const std::vector<KeyType>& keys) {
    internal_table_.reserve(keys.size());
    for (IndexType ii = 0; ii < static_cast<IndexType>(keys.size()); ++ii) {
      internal_table_.emplace(keys[ii], ii);
    }
  }

  std::pair<Iterator, Iterator> retrieve(const KeyType& key) {
    auto tmp = internal_table_.equal_range(key);
    return std::make_pair(Iterator(tmp.first), Iterator(tmp.second));
  }

 private:
  std::unordered_multimap<KeyType, ValueType> internal_table_;
};

}  // namespace core
}  // namespace falconn

#endif
