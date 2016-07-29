#ifndef __HEAP_H__
#define __HEAP_H__

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace falconn {
namespace core {

template <typename KeyType, typename DataType>
class HeapBase {
 public:
  class Item {
   public:
    KeyType key;
    DataType data;

    Item() {}
    Item(const KeyType& k, const DataType& d) : key(k), data(d) {}

    bool operator<(const Item& i2) const { return key < i2.key; }
  };

  void extract_min(KeyType* key, DataType* data) {
    *key = v_[0].key;
    *data = v_[0].data;
    num_elements_ -= 1;
    v_[0] = v_[num_elements_];
    heap_down(0);
  }

  bool empty() { return num_elements_ == 0; }

  void insert_unsorted(const KeyType& key, const DataType& data) {
    if (v_.size() == static_cast<size_t>(num_elements_)) {
      v_.push_back(Item(key, data));
    } else {
      v_[num_elements_].key = key;
      v_[num_elements_].data = data;
    }
    num_elements_ += 1;
  }

  void insert(const KeyType& key, const DataType& data) {
    if (v_.size() == static_cast<size_t>(num_elements_)) {
      v_.push_back(Item(key, data));
    } else {
      v_[num_elements_].key = key;
      v_[num_elements_].data = data;
    }
    num_elements_ += 1;
    heap_up(num_elements_ - 1);
  }

  void heapify() {
    int_fast32_t rightmost = parent(num_elements_ - 1);
    for (int_fast32_t cur_loc = rightmost; cur_loc >= 0; --cur_loc) {
      heap_down(cur_loc);
    }
  }

  void reset() { num_elements_ = 0; }

  void resize(size_t new_size) { v_.resize(new_size); }

 protected:
  int_fast32_t lchild(int_fast32_t x) { return 2 * x + 1; }

  int_fast32_t rchild(int_fast32_t x) { return 2 * x + 2; }

  int_fast32_t parent(int_fast32_t x) { return (x - 1) / 2; }

  void swap_entries(int_fast32_t a, int_fast32_t b) {
    Item tmp = v_[a];
    v_[a] = v_[b];
    v_[b] = tmp;
  }

  void heap_up(int_fast32_t cur_loc) {
    int_fast32_t p = parent(cur_loc);
    while (cur_loc > 0 && v_[p].key > v_[cur_loc].key) {
      swap_entries(p, cur_loc);
      cur_loc = p;
      p = parent(cur_loc);
    }
  }

  void heap_down(int_fast32_t cur_loc) {
    while (true) {
      int_fast32_t lc = lchild(cur_loc);
      int_fast32_t rc = rchild(cur_loc);
      if (lc >= num_elements_) {
        return;
      }

      if (v_[cur_loc].key <= v_[lc].key) {
        if (rc >= num_elements_ || v_[cur_loc].key <= v_[rc].key) {
          return;
        } else {
          swap_entries(cur_loc, rc);
          cur_loc = rc;
        }
      } else {
        if (rc >= num_elements_ || v_[lc].key <= v_[rc].key) {
          swap_entries(cur_loc, lc);
          cur_loc = lc;
        } else {
          swap_entries(cur_loc, rc);
          cur_loc = rc;
        }
      }
    }
  }

  std::vector<Item> v_;
  int_fast32_t num_elements_ = 0;
};

template <typename KeyType, typename DataType>
class SimpleHeap : public HeapBase<KeyType, DataType> {
 public:
  // TODO: move this to the base class (if necessary) and add implementation for
  // the augmented version
  void replace_top(const KeyType& key, const DataType& data) {
    this->v_[0].key = key;
    this->v_[0].data = data;
    this->heap_down(0);
  }

  // TODO: move this to the base class (if necessary) and add implementation for
  // the augmented version
  KeyType min_key() { return this->v_[0].key; }

  std::vector<typename HeapBase<KeyType, DataType>::Item>& get_data() {
    return this->v_;
  }
};

template <typename KeyType, typename DataType>
class AugmentedHeap : public HeapBase<KeyType, DataType> {
 public:
  void extract_min(KeyType* key, DataType* data) {
    if (has_guaranteed_top_) {
      has_guaranteed_top_ = false;
      *key = guaranteed_top_.key;
      *data = guaranteed_top_.data;
    } else {
      *key = this->v_[0].key;
      *data = this->v_[0].data;
      this->num_elements_ -= 1;
      this->v_[0] = this->v_[this->num_elements_];
      this->heap_down(0);
    }
  }

  bool empty() { return this->num_elements_ == 0 && !has_guaranteed_top_; }

  void insert_guaranteed_top(const KeyType& key, const DataType& data) {
    has_guaranteed_top_ = true;
    guaranteed_top_.key = key;
    guaranteed_top_.data = data;
  }

  void reset() {
    this->num_elements_ = 0;
    has_guaranteed_top_ = false;
  }

 protected:
  typename HeapBase<KeyType, DataType>::Item guaranteed_top_;
  bool has_guaranteed_top_ = false;
};

}  // namespace core
}  // namespace falconn

#endif
