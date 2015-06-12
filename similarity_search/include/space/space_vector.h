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

#ifndef _SPACE_VECTOR_H_
#define _SPACE_VECTOR_H_

#include <string>
#include <map>
#include <stdexcept>
#include <sstream>

#include <string.h>
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"

namespace similarity {

template <typename dist_t>
class VectorSpace : public Space<dist_t> {
 public:
  virtual ~VectorSpace() {}

  virtual void ReadDataset(ObjectVector& dataset,
                      const ExperimentConfig<dist_t>* config,
                      const char* inputfile,
                      const int MaxNumObjects) const;
  virtual void WriteDataset(const ObjectVector& dataset,
                            const char* outputfile) const;
  virtual Object* CreateObjFromVect(IdType id, LabelType label, const std::vector<dist_t>& InpVect) const;
  virtual size_t GetElemQty(const Object* object) const = 0;
  virtual void CreateVectFromObj(const Object* obj, dist_t* pVect,
                                 size_t nElem) const = 0;
 protected:
  virtual Space<dist_t>* HiddenClone() const = 0;
  virtual dist_t HiddenDistance(const Object* obj1, const Object* obj2) const = 0;
  void ReadVec(std::string line, LabelType& label, std::vector<dist_t>& v) const;
  void CreateVectFromObjSimpleStorage(const char *pFuncName,
                                 const Object* obj, dist_t* pDstVect,
                                 size_t nElem) const {
    const dist_t* pSrcVec = reinterpret_cast<const dist_t*>(obj->data());
    const size_t len = GetElemQty(obj);
    if (nElem > len) {
      std::stringstream err;
      err << pFuncName << " The number of requested elements "
          << nElem << " is larger than the actual number of elements " << len;
      throw runtime_error(err.str());
    }
    for (size_t i = 0; i < nElem; ++i) pDstVect[i]=pSrcVec[i];
  }
};

template <typename dist_t>
class VectorSpaceSimpleStorage : public VectorSpace<dist_t> {
 public:
  virtual ~VectorSpaceSimpleStorage() {}
  virtual size_t GetElemQty(const Object* object) const {
    return object->datalength()/ sizeof(dist_t);
  }
  virtual void CreateVectFromObj(const Object* obj, dist_t* pDstVect,
                                 size_t nElem) const {
    return VectorSpace<dist_t>::
                CreateVectFromObjSimpleStorage(__func__, obj, pDstVect, nElem);
  }
};

}  // namespace similarity

#endif
