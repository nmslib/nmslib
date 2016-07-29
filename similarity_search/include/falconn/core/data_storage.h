#ifndef __DATA_STORAGE_H__
#define __DATA_STORAGE_H__

#include <cstdint>
#include <type_traits>
#include <utility>
#include <vector>

#include "../falconn_global.h"
#include "prefetchers.h"

// TODO: add tests

namespace falconn {
namespace core {

class DataStorageError : public FalconnError {
 public:
  DataStorageError(const char* msg) : FalconnError(msg) {}
};

// A class for providing access to points stored in a std::vector.
// Using a DataStorage class in NearestNeighborQuery (as opposed to a
// std::vector) allows us to use the same implementation of
// NearestNeighborQuery for points stored in std::vectors, arbitrary memory
// locations (keys are pointers), and contiguous data in an Eigen matrix.
// TODO: implement EigenMatrixDataStorage and PointerDataStorage.
template <typename PointType, typename KeyType = int32_t>
class ArrayDataStorage {
 public:
  class FullSequenceIterator {
   public:
    FullSequenceIterator(const ArrayDataStorage& parent) : parent_(&parent) {
      if (parent_->data_.size() == 0) {
        parent_ = nullptr;
        index_ = -1;
      } else {
        index_ = 0;
        // TODO: try different prefetching steps
        prefetcher_.prefetch(parent_->data_, 0);
        if (parent_->data_.size() >= 2) {
          prefetcher_.prefetch(parent_->data_, 1);

          if (parent_->data_.size() >= 3) {
            prefetcher_.prefetch(parent_->data_, 2);
          }
        }
      }
    }

    FullSequenceIterator() {}

    const PointType& get_point() const { return parent_->data_[index_]; }

    const KeyType& get_key() const { return index_; }

    bool is_valid() const { return parent_ != nullptr; }

    void operator++() {
      if (index_ >= 0 &&
          index_ + 1 < static_cast<int_fast64_t>(parent_->data_.size())) {
        index_ += 1;
        if (index_ + 2 < static_cast<int_fast64_t>(parent_->data_.size())) {
          // TODO: try different prefetching steps
          prefetcher_.prefetch(parent_->data_, index_ + 2);
        }
      } else {
        if (index_ == -1) {
          throw DataStorageError("Advancing invalid FullSequenceIterator.");
        } else {
          parent_ = nullptr;
          index_ = -1;
        }
      }
    }

   private:
    int_fast64_t index_ = -1;
    const ArrayDataStorage* parent_ = nullptr;
    StdVectorPrefetcher<PointType> prefetcher_;
  };

  class SubsequenceIterator {
   public:
    SubsequenceIterator(const std::vector<KeyType>& keys,
                        const ArrayDataStorage& parent)
        : keys_(&keys), parent_(&parent) {
      if (keys_->size() == 0) {
        keys_ = nullptr;
        parent_ = nullptr;
        index_ = -1;
      } else {
        index_ = 0;
        // TODO: try different prefetching steps
        prefetcher_.prefetch(parent_->data_, (*keys_)[0]);
        if (keys_->size() >= 2) {
          prefetcher_.prefetch(parent_->data_, (*keys_)[1]);

          if (keys_->size() >= 3) {
            prefetcher_.prefetch(parent_->data_, (*keys_)[2]);
          }
        }
      }
    }

    SubsequenceIterator() {}

    // Not using STL-style iterators for now because the custom format below
    // makes more sense in NearestNeighborQuery.
    /*const PointType& operator * () const {
      return parent->data[(*keys)[index_]];
    }*/

    const PointType& get_point() const {
      return parent_->data_[(*keys_)[index_]];
    }

    const KeyType& get_key() const { return (*keys_)[index_]; }

    bool is_valid() const { return parent_ != nullptr; }

    /*bool operator != (const Iterator& rhs) const {
      if (parent_ != rhs.parent_) {
          return true;
      } else if (keys_ != rhs.keys_) {
          return true;
      } else if (index_ != rhs.index_) {
        return true;
      }
      return false;
    }*/

    void operator++() {
      if (index_ >= 0 &&
          index_ + 1 < static_cast<int_fast64_t>(keys_->size())) {
        index_ += 1;
        if (index_ + 2 < static_cast<int_fast64_t>(keys_->size())) {
          // TODO: try different prefetching steps
          prefetcher_.prefetch(parent_->data_, (*keys_)[index_ + 2]);
        }
      } else {
        if (index_ == -1) {
          throw DataStorageError("Advancing invalid SubsequenceIterator.");
        }
        keys_ = nullptr;
        parent_ = nullptr;
        index_ = -1;
      }
    }

   private:
    int_fast64_t index_ = -1;
    const std::vector<KeyType>* keys_ = nullptr;
    const ArrayDataStorage* parent_ = nullptr;
    StdVectorPrefetcher<PointType> prefetcher_;
  };

  ArrayDataStorage(const std::vector<PointType>& data) : data_(data) {}

  /*std::pair<Iterator, Iterator> get_sequence(const std::vector<KeyType>& keys)
      const {
    return std::make_pair(Iterator(keys, *this), Iterator());
  }*/
  /*const PointType& operator [] (int_fast64_t index) const {
    return data_[index];
  }*/

  int_fast64_t size() const { return data_.size(); }

  SubsequenceIterator get_subsequence(const std::vector<KeyType>& keys) const {
    return SubsequenceIterator(keys, *this);
  }

  FullSequenceIterator get_full_sequence() const {
    return FullSequenceIterator(*this);
  }

 private:
  const std::vector<PointType>& data_;
};

template <typename PointType, typename KeyType = int32_t>
class PlainArrayDataStorage {
  PlainArrayDataStorage() {
    static_assert(FalseStruct<PointType>::value, "Point type not supported.");
  }

  template <typename PT>
  struct FalseStruct : std::false_type {};
};

template <typename CoordinateType, typename KeyType>
class PlainArrayDataStorage<DenseVector<CoordinateType>, KeyType> {
 public:
  typedef Eigen::Map<const DenseVector<CoordinateType>> ConstVectorMap;
  typedef Eigen::Map<DenseVector<CoordinateType>> VectorMap;

  class FullSequenceIterator {
   public:
    FullSequenceIterator(const PlainArrayDataStorage& parent)
        : parent_(&parent), tmp_map_(nullptr, 0) {
      if (parent_->size() == 0) {
        parent_ = nullptr;
        index_ = -1;
      } else {
        index_ = 0;
        // TODO: try different prefetching steps
        prefetcher_.prefetch(parent_->data_);
        if (parent_->size() >= 2) {
          prefetcher_.prefetch(parent_->data_ + parent_->dim_);

          if (parent_->size() >= 3) {
            prefetcher_.prefetch(parent_->data_ + 2 * parent_->dim_);
          }
        }
      }
    }

    FullSequenceIterator() {}

    const ConstVectorMap& get_point() {
      new (&tmp_map_) ConstVectorMap(&(parent_->data_[index_ * parent_->dim_]),
                                     static_cast<int>(parent_->dim_));
      return tmp_map_;
    }

    const KeyType& get_key() const { return index_; }

    bool is_valid() const { return parent_ != nullptr; }

    void operator++() {
      if (index_ >= 0 &&
          index_ + 1 < static_cast<int_fast64_t>(parent_->size())) {
        index_ += 1;
        if (index_ + 2 < static_cast<int_fast64_t>(parent_->size())) {
          // TODO: try different prefetching steps
          prefetcher_.prefetch(parent_->data_ + (index_ + 2) * parent_->dim_);
        }
      } else {
        if (index_ == -1) {
          throw DataStorageError("Advancing invalid FullSequenceIterator.");
        } else {
          parent_ = nullptr;
          index_ = -1;
        }
      }
    }

   private:
    int_fast64_t index_ = -1;
    const PlainArrayDataStorage* parent_ = nullptr;
    ConstVectorMap tmp_map_;
    PlainArrayPrefetcher<CoordinateType> prefetcher_;
  };

  class SubsequenceIterator {
   public:
    SubsequenceIterator(const std::vector<KeyType>& keys,
                        const PlainArrayDataStorage& parent)
        : keys_(&keys), parent_(&parent), tmp_map_(nullptr, 0) {
      if (keys_->size() == 0) {
        keys_ = nullptr;
        parent_ = nullptr;
        index_ = -1;
      } else {
        index_ = 0;
        // TODO: try different prefetching steps
        prefetcher_.prefetch(parent_->data_ + (*keys_)[0] * parent_->dim_);
        if (keys_->size() >= 2) {
          prefetcher_.prefetch(parent_->data_ + (*keys_)[1] * parent_->dim_);

          if (keys_->size() >= 3) {
            prefetcher_.prefetch(parent_->data_ + (*keys_)[2] * parent_->dim_);
          }
        }
      }
    }

    SubsequenceIterator() {}

    const ConstVectorMap& get_point() {
      new (&tmp_map_)
          ConstVectorMap(&(parent_->data_[(*keys_)[index_] * parent_->dim_]),
                         static_cast<int>(parent_->dim_));
      return tmp_map_;
    }

    const KeyType& get_key() const { return (*keys_)[index_]; }

    bool is_valid() const { return parent_ != nullptr; }

    void operator++() {
      if (index_ >= 0 &&
          index_ + 1 < static_cast<int_fast64_t>(keys_->size())) {
        index_ += 1;
        if (index_ + 2 < static_cast<int_fast64_t>(keys_->size())) {
          // TODO: try different prefetching steps
          prefetcher_.prefetch(parent_->data_ +
                               (*keys_)[index_ + 2] * parent_->dim_);
        }
      } else {
        if (index_ == -1) {
          throw DataStorageError("Advancing invalid SubsequenceIterator.");
        }
        keys_ = nullptr;
        parent_ = nullptr;
        index_ = -1;
      }
    }

   private:
    int_fast64_t index_ = -1;
    const std::vector<KeyType>* keys_ = nullptr;
    const PlainArrayDataStorage* parent_ = nullptr;
    ConstVectorMap tmp_map_;
    PlainArrayPrefetcher<CoordinateType> prefetcher_;
  };

  PlainArrayDataStorage(const CoordinateType* data, int_fast64_t num_points,
                        int_fast64_t dim)
      : data_(data), num_points_(num_points), dim_(dim) {}

  int_fast64_t size() const { return num_points_; }

  SubsequenceIterator get_subsequence(const std::vector<KeyType>& keys) const {
    return SubsequenceIterator(keys, *this);
  }

  FullSequenceIterator get_full_sequence() const {
    return FullSequenceIterator(*this);
  }

 private:
  const CoordinateType* data_;
  int_fast64_t num_points_;
  int_fast64_t dim_;
};

template <typename PointType, typename Transformation,
          typename InnerDataStorage, typename KeyType = int32_t>
class TransformedDataStorage {
 public:
  class FullSequenceIterator {
   public:
    FullSequenceIterator(const TransformedDataStorage& parent)
        : parent_(&parent), iter_(parent.storage_.get_full_sequence()) {}

    FullSequenceIterator() {}

    const PointType& get_point() {
      tmp_point_ = iter_.get_point();
      parent_->transformation_.apply(&tmp_point_);
      return tmp_point_;
    }

    const KeyType& get_key() const { return iter_.get_key(); }

    bool is_valid() const { return iter_.is_valid(); }

    void operator++() { ++iter_; }

   private:
    const TransformedDataStorage* parent_ = nullptr;
    typename InnerDataStorage::FullSequenceIterator iter_;
    PointType tmp_point_;
  };

  class SubsequenceIterator {
   public:
    SubsequenceIterator(const TransformedDataStorage& parent,
                        const std::vector<KeyType>& keys)
        : parent_(&parent), iter_(parent.storage_.get_sub_sequence(keys)) {}

    SubsequenceIterator() {}

    const PointType& get_point() {
      tmp_point_ = iter_.get_point();
      parent_->transformation_->apply(&tmp_point_);
      return tmp_point_;
    }

    const KeyType& get_key() const { return iter_.get_key(); }

    bool is_valid() const { return iter_.is_valid(); }

    void operator++() { ++iter_; }

   private:
    const TransformedDataStorage* parent_ = nullptr;
    typename InnerDataStorage::SubsequenceIterator iter_;
    PointType tmp_point_;
  };

  TransformedDataStorage(const Transformation& transformation,
                         const InnerDataStorage& storage)
      : transformation_(transformation), storage_(storage) {}

  int_fast64_t size() const { return storage_.size(); }

  SubsequenceIterator get_subsequence(const std::vector<KeyType>& keys) const {
    return SubsequenceIterator(keys, *this);
  }

  FullSequenceIterator get_full_sequence() const {
    return FullSequenceIterator(*this);
  }

 private:
  const Transformation& transformation_;
  const InnerDataStorage& storage_;
};

}  // namespace core
}  // namespace falconn

#endif
