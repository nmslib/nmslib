/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2010--2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef VECTOR_POOL_H
#define VECTOR_POOL_H

#include <vector>
#include <deque>
#include <memory>
#include <mutex>

namespace similarity {

template <typename T>
class VectorPool {
public:
  VectorPool(size_t initPoolSize, size_t initVectSize = 0) : initVectSize_(initVectSize) {
    for (size_t i = 0; i <initPoolSize; ++i) {
      pool_.push_back(newElem());
    }
  }
  std::vector<T>* loan () {
    std::unique_lock<std::mutex> lock(mutex_);

    if (pool_.empty()) {
      return newElem();
    }
    std::vector<T>* e  = pool_.front();
    pool_.pop_front();
    return e;
  }
  void release(std::vector<T>* e) {
    std::unique_lock<std::mutex> lock(mutex_);

    pool_.push_back(e);
  }
  ~VectorPool() {
    std::unique_lock<std::mutex> lock(mutex_);

    for (auto e : pool_)
      delete e;
  }
private:
  std::deque<std::vector<T>*> pool_;
  size_t                      initVectSize_ = 0;
  std::mutex                  mutex_;

  std::vector<T>* newElem() {
    return new std::vector<T>(initVectSize_);
  }
};

}

#endif
