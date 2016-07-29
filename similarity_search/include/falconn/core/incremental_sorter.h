#ifndef __INCREMENTAL_SORTER_H__
#define __INCREMENTAL_SORTER_H__

#include <algorithm>
#include <cstdint>
#include <vector>

namespace falconn {
namespace core {

template <typename T>
class IncrementalSorter {
 public:
  void reset(std::vector<T>* data, int_fast32_t block_size) {
    data_ = data;
    cur_block_size_ = block_size;
    if (static_cast<size_t>(cur_block_size_) >= data_->size()) {
      std::sort(data_->begin(), data_->end());
      sorted_to_ = data_->size();
    } else {
      std::nth_element(data_->begin(), data_->begin() + cur_block_size_ - 1,
                       data_->end());
      std::sort(data_->begin(), data_->begin() + cur_block_size_);
      sorted_to_ = cur_block_size_;
      cur_block_size_ *= 2;
    }
  }

  const T& get(int_fast32_t index) {
    if (index < sorted_to_) {
      return (*data_)[index];
    } else {
      int_fast32_t next_sorted_to = sorted_to_;
      while (index >= next_sorted_to) {
        next_sorted_to += cur_block_size_;
        cur_block_size_ *= 2;
      }
      if (next_sorted_to >= static_cast<int_fast32_t>(data_->size()) - 1) {
        std::sort(data_->begin() + sorted_to_, data_->end());
        sorted_to_ = data_->size();
      } else {
        std::nth_element(data_->begin() + sorted_to_,
                         data_->begin() + next_sorted_to - 1, data_->end());
        std::sort(data_->begin() + sorted_to_, data_->begin() + next_sorted_to);
        sorted_to_ = next_sorted_to;
      }
      // printf("now sorted to %d\n", sorted_to_);
      return (*data_)[index];
    }
  };

 private:
  std::vector<T>* data_;
  int_fast32_t cur_block_size_;
  int_fast32_t sorted_to_;
};

}  // namespace core
}  // namespace falconn

#endif
