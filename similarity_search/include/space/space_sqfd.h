/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2014
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _SPACE_SQFD_H_
#define _SPACE_SQFD_H_

#include <string>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "distcomp.h"

#define SPACE_SQFD_HEURISTIC_FUNC "sqfd_heuristic_func"
#define SPACE_SQFD_MINUS_FUNC     "sqfd_minus_func"
#define SPACE_SQFD_GAUSSIAN_FUNC  "sqfd_gaussian_func"

namespace similarity {

template <typename dist_t>
class SqfdFunction {
 public:
  virtual ~SqfdFunction() {}
  virtual dist_t f(const dist_t* p1, const dist_t* p2, const int sz) const = 0;
  virtual std::string ToString() const = 0;
  virtual SqfdFunction<dist_t>* Clone() const = 0;
};

template <typename dist_t>
class SqfdMinusFunction : public SqfdFunction<dist_t> {
 public:
  dist_t f(const dist_t* p1, const dist_t* p2, const int sz) const {
    return -L2NormSIMD(p1, p2, sz);
  }
  std::string ToString() const {
    return "minus function";
  }
  virtual SqfdFunction<dist_t>* Clone() const {
    return new SqfdMinusFunction<dist_t>(); // No parameters!!!!
  }
};

template <typename dist_t>
class SqfdHeuristicFunction : public SqfdFunction<dist_t> {
 public:
  SqfdHeuristicFunction(float alpha) : alpha_(alpha) {}
  dist_t f(const dist_t* p1, const dist_t* p2, const int sz) const {
    return 1.0 / (alpha_ + L2NormSIMD(p1, p2, sz));
  }
  std::string ToString() const {
    std::stringstream stream;
    stream << "heuristic function alpha=" << alpha_;
    return stream.str();
  }
  virtual SqfdFunction<dist_t>* Clone() const {
    return new SqfdHeuristicFunction<dist_t>(alpha_);
  }
 private:
  float alpha_;
};

template <typename dist_t>
class SqfdGaussianFunction : public SqfdFunction<dist_t> {
 public:
  SqfdGaussianFunction(float alpha) : alpha_(alpha) {}
  dist_t f(const dist_t* p1, const dist_t* p2, const int sz) const {
    const dist_t d = L2NormSIMD(p1, p2, sz);
    return exp(-alpha_ * d * d);
  }
  std::string ToString() const {
    std::stringstream stream;
    stream << "gaussian function alpha=" << alpha_;
    return stream.str();
  }
  virtual SqfdFunction<dist_t>* Clone() const {
    return new SqfdGaussianFunction<dist_t>(alpha_);
  }
 private:
  float alpha_;
};

template <typename dist_t>
class SpaceSqfd : public Space<dist_t> {
 public:
  SpaceSqfd(SqfdFunction<dist_t>* func);
  virtual ~SpaceSqfd();

  std::string ToString() const;

  void ReadDataset(
      ObjectVector& dataset,
      const ExperimentConfig<dist_t>* config,
      const char* inputfile,
      const int MaxNumObjects) const;
  /*
   * CreateVectFromObj and GetElemQty() are only needed, if
   * one wants to use methods with random projections.
   */
  virtual void CreateVectFromObj(const Object* obj, dist_t* pVect,
                                   size_t nElem) const {
    throw runtime_error("Cannot create vector for the space: " + ToString());
  }
  virtual size_t GetElemQty(const Object* object) const {return 0;}
 protected:
  virtual Space<dist_t>* HiddenClone() const {
    return new SpaceSqfd<dist_t>(func_->Clone());
  }
  dist_t HiddenDistance(
      const Object* obj1,
      const Object* obj2) const;
 private:
  SqfdFunction<dist_t>* func_;
};

}  // namespace similarity

#endif
