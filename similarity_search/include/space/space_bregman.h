/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#ifndef _SPACE_BREGMAN_H_
#define _SPACE_BREGMAN_H_

#include <string>
#include <map>
#include <stdexcept>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "space_lp.h"

#define SPACE_KLDIV_FAST                        "kldivfast"
#define SPACE_KLDIV_FAST_RIGHT_QUERY            "kldivfastrq" 

#define SPACE_KLDIVGEN_FAST                     "kldivgenfast"
#define SPACE_KLDIVGEN_FAST_RIGHT_QUERY         "kldivgenfastrq" 
#define SPACE_KLDIVGEN_SLOW                     "kldivgenslow"

#define SPACE_ITAKURASAITO_FAST                 "itakurasaitofast"
#define SPACE_ITAKURASAITO_FAST_RIGHT_QUERY     "itakurasaitofastrq" 
#define SPACE_ITAKURASAITO_SLOW                 "itakurasaitoslow"

namespace similarity {

template <typename dist_t>
class BregmanDiv : public VectorSpace<dist_t> {
 public:
  virtual ~BregmanDiv() {}

  /* computes the Bregman generator function */
  virtual dist_t Function(const Object* object) const = 0;
  /* computes the gradient of the generator function at point "object" */
  virtual Object* GradientFunction(const Object* object) const = 0;
  /* computes the inverse gradient of the generator function at point "object" */
  virtual Object* InverseGradientFunction(const Object* object) const = 0;

  virtual std::string ToString() const = 0;

  /* 
   * Returns the number of vector elements stored in the object.
   * It may not be possible to get this number using Object->datalength(),
   * because the object can store additional data such as precomputed
   * logarithms
   */
  virtual size_t GetElemQty(const Object* object) const = 0;

  virtual Object* Mean(const ObjectVector& data) const;

  static inline
  const BregmanDiv<dist_t>* ConvertFrom(const Space<dist_t>* space) {
    const BregmanDiv<dist_t>* div =
        dynamic_cast<const BregmanDiv<dist_t>*>(space);
    if (div != NULL) {
      return div;
    } else {
      LOG(FATAL) << "Space " << space->ToString()
          << " is not Bregman divergence";
      return NULL;
    }
  }
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;
};

template <typename dist_t>
class KLDivAbstract : public BregmanDiv<dist_t> {
 public:
  virtual ~KLDivAbstract() {}

  virtual dist_t Function(const Object* object) const;
  virtual Object* GradientFunction(const Object* object) const;
  virtual Object* InverseGradientFunction(const Object* object) const;

  virtual std::string ToString() const = 0;
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const = 0;
  virtual size_t GetElemQty(const Object* object) const = 0;
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;
};

template <typename dist_t>
class KLDivGenSlow : public KLDivAbstract<dist_t> {
 public:
  virtual ~KLDivGenSlow() {}

  virtual std::string ToString() const { return "Generalized Kullback-Leibler divergence"; }
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;

  virtual size_t GetElemQty(const Object* object) const { return object->datalength()/ sizeof(dist_t); }
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* object1, const Object* object2) const;
};

template <typename dist_t>
class KLDivGenFast : public KLDivAbstract<dist_t> {
 public:
  virtual ~KLDivGenFast() {}

  virtual Object* InverseGradientFunction(const Object* object) const;
  virtual std::string ToString() const { return "Generalized Kullback-Leibler divergence (precomputed logs)"; }
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;
  virtual size_t GetElemQty(const Object* object) const { return object->datalength()/ sizeof(dist_t)/ 2; }
  virtual Object* Mean(const ObjectVector& data) const;
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* object1, const Object* object2) const;
};

template <typename dist_t>
class ItakuraSaitoFast : public BregmanDiv<dist_t> {
 public:
  virtual ~ItakuraSaitoFast() {}

  virtual dist_t Function(const Object* object) const;
  virtual Object* InverseGradientFunction(const Object* object) const;
  virtual Object* GradientFunction(const Object* object) const;

  virtual std::string ToString() const { return "Itakura-Saito (precomputed logs)"; }
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;
  virtual size_t GetElemQty(const Object* object) const { return object->datalength()/ sizeof(dist_t)/ 2; }
  virtual Object* Mean(const ObjectVector& data) const;
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* object1, const Object* object2) const;
};

template <typename dist_t>
class KLDivGenFastRightQuery : public VectorSpace<dist_t> {
 public:
  virtual ~KLDivGenFastRightQuery() {}

  virtual std::string ToString() const { return "Generalized Kullback-Leibler divergence, right queries (precomputed logs)"; }
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;
  virtual size_t GetElemQty(const Object* object) const { return object->datalength()/ sizeof(dist_t)/ 2; }
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* object1, const Object* object2) const;
};

template <typename dist_t>
class KLDivFast: public VectorSpace<dist_t> {
 public:
  virtual ~KLDivFast() {}

  virtual std::string ToString() const { return "Kullback-Leibler divergence (precomputed logs)"; }
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;
  virtual size_t GetElemQty(const Object* object) const { return object->datalength()/ sizeof(dist_t)/ 2; }
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* object1, const Object* object2) const;
};

template <typename dist_t>
class KLDivFastRightQuery: public VectorSpace<dist_t> {
 public:
  virtual ~KLDivFastRightQuery() {}

  virtual std::string ToString() const { return "Kullback-Leibler divergence, right queries (precomputed logs)"; }
  virtual Object* CreateObjFromVect(size_t id, const std::vector<dist_t>& InpVect) const;
  virtual size_t GetElemQty(const Object* object) const { return object->datalength()/ sizeof(dist_t)/ 2; }
 protected:
  // Should not be directly accessible
  virtual dist_t HiddenDistance(const Object* object1, const Object* object2) const;
};

}  // namespace similarity

#endif      // _METRIC_SPACE_H_
