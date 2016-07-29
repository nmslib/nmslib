#ifndef __BIT_PACKED_VECTOR_H__
#define __BIT_PACKED_VECTOR_H__

#include <cstdint>
#include <vector>

#include "../falconn_global.h"

namespace falconn {
namespace core {

class BitPackedVectorError : public FalconnError {
 public:
  BitPackedVectorError(const char* msg) : FalconnError(msg) {}
};

template <typename DataType, typename StorageType = uint64_t,
          typename IndexType = int_fast64_t>
class BitPackedVector {
 public:
  BitPackedVector(int_fast64_t num_items, int_fast64_t item_size)
      : num_items_(num_items), item_size_(item_size) {
    if (item_size > 8 * static_cast<int_fast64_t>(sizeof(DataType))) {
      throw BitPackedVectorError(
          "DataType too small for the number of bits "
          "specified.");
    }
    if (item_size > num_bits_per_package_) {
      throw BitPackedVectorError(
          "Currently the item size must be at most the "
          "data package size.");
    }
    if (num_items > std::numeric_limits<IndexType>::max()) {
      throw BitPackedVectorError(
          "IndexType too small for the vector size "
          "specified.");
    }
    num_data_packets_ = (num_items_ * item_size_) / num_bits_per_package_;
    if ((num_items_ * item_size_) % num_bits_per_package_ != 0) {
      num_data_packets_ += 1;
    }
    data_.resize(num_data_packets_);
    for (int_fast64_t ii = 0; ii < num_data_packets_; ++ii) {
      data_[ii] = 0;
    }
  }

  // For (potential) performance reasons, get() does no bounds checking.
  DataType get(IndexType index) const {
    int_fast64_t first_bit = index * item_size_;
    int_fast64_t first_package = first_bit / num_bits_per_package_;
    int_fast64_t offset_in_package =
        first_bit - first_package * num_bits_per_package_;
    StorageType result = data_[first_package];
    // Move first bits to the beginning of the result.
    result >>= offset_in_package;
    // Check if we need to go to the next package to get all the bits.
    // (Currently the class assumes that an item occupies at most two packages.)
    int_fast64_t remaining_bits =
        num_bits_per_package_ - (offset_in_package + item_size_);
    if (remaining_bits >= 0) {
      // Zero out the remaining bits
      // printf("index %lld  remaining_bits %lld  result %llu\n", index,
      //    remaining_bits, result);
      result <<= remaining_bits + offset_in_package;
      result >>= remaining_bits + offset_in_package;
      // printf("index %lld  remaining_bits %lld  result %llu\n", index,
      //    remaining_bits, result);
    } else {
      // printf("get: in else case\n");
      // remaining_bits is negative here.
      StorageType tmp = data_[first_package + 1];
      tmp <<= (num_bits_per_package_ + remaining_bits);
      tmp >>= (num_bits_per_package_ + remaining_bits);
      result |= (tmp << (item_size_ + remaining_bits));
    }
    return static_cast<DataType>(result);
  }

  // For (potential) performance reasons, set() does no bounds checking.
  void set(IndexType index, DataType value) {
    int_fast64_t first_bit = index * item_size_;
    int_fast64_t first_package = first_bit / num_bits_per_package_;
    int_fast64_t offset_in_package =
        first_bit - first_package * num_bits_per_package_;
    /*printf("set index %lld  value %lld  offset_in_package %lld\n", index,
        value, offset_in_package);
    printf("set index %lld  value %lld  data_[first_package] %llu\n",
        index, value, data_[first_package]);
    printf("set index %lld  value %lld  shift %lld  shifted %llu\n",
        index, value, num_bits_per_package_ - offset_in_package,
        (data_[first_package] << (num_bits_per_package_ - offset_in_package
            - 1)) << 1);*/
    // Avoid shift with shift_count == bit_width.
    StorageType new_package, tmp;
    if (offset_in_package != 0) {
      new_package = data_[first_package]
                    << (num_bits_per_package_ - offset_in_package);
      new_package >>= (num_bits_per_package_ - offset_in_package);
      tmp = value;
      /*printf("set index %lld  value %lld  new_package %llu  tmp %llu\n",
          index, value, new_package, tmp);*/
      new_package |= tmp << offset_in_package;
    } else {
      new_package = value;
    }
    /*StorageType new_package =
        (data_[first_package] << (num_bits_per_package_ - offset_in_package
            - 1));
    new_package <<= 1;
    new_package >>= (num_bits_per_package_ - offset_in_package - 1);
    new_package >>= 1;
    StorageType tmp = value;*/
    /*printf("set index %lld  value %lld  new_package %llu  tmp %llu\n",
          index, value, new_package, tmp);*/
    // new_package |= tmp << offset_in_package;
    /*printf("set index %lld  value %lld  new_package2 %llu  tmp %llu\n",
          index, value, new_package, tmp);*/
    int_fast64_t remaining_bits =
        num_bits_per_package_ - (offset_in_package + item_size_);
    if (remaining_bits > 0) {
      tmp = data_[first_package];
      tmp >>= (num_bits_per_package_ - remaining_bits);
      tmp <<= (num_bits_per_package_ - remaining_bits);
      /*printf("set index %lld  value %lld  remaining_bits %lld  tmp %llu\n",
          index, value, remaining_bits, tmp);*/
      data_[first_package] = new_package | tmp;
    } else if (remaining_bits == 0) {
      /*printf("set index %lld  value %lld  remaining_bits %lld  tmp %llu\n",
          index, value, remaining_bits, tmp);*/
      data_[first_package] = new_package;
    } else {
      // printf("set: in else case\n");
      data_[first_package] = new_package;
      tmp = value;
      new_package = tmp >> (item_size_ + remaining_bits);
      tmp = data_[first_package + 1];
      tmp >>= -remaining_bits;
      tmp <<= -remaining_bits;
      data_[first_package + 1] = tmp | new_package;
    }
    // printf("current state: %llx\n", data_[0]);
  }

 private:
  const int_fast64_t num_bits_per_package_ = 8 * sizeof(StorageType);

  int_fast64_t num_items_;
  int_fast64_t item_size_;
  int_fast64_t num_data_packets_;
  std::vector<StorageType> data_;
};

}  // namespace core
}  // namespace falconn

#endif
