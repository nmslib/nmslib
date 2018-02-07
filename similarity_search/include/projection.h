/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _PROJECTION_H_
#define _PROJECTION_H_

#include "distcomp.h"
#include "space.h"
#include "object.h"

#include <string>
#include <cstddef>

// Classic random projections using random orthonormal vectors
#define PROJ_TYPE_RAND            "rand"          
// Distance to random reference points
#define PROJ_TYPE_RAND_REF_POINT  "randrefpt"     
// FastMap (project on lines defined by two randomly selected points)
#define PROJ_TYPE_FAST_MAP        "fastmap"       
// Integer value permutations
#define PROJ_TYPE_PERM            "perm"
// Integer value truncted permutations
#define PROJ_TYPE_PERM_TRUNC      "permtrunc"
// Binarized permutations: note that the result is nevertheless saved as floating-point vector
#define PROJ_TYPE_PERM_BIN        "permbin"
// Dense vectors remain unchanged, sparsed vectors are "hashed" into dense ones
#define PROJ_TYPE_VECTOR_DENSE    "densevect"
// Trivial projection
#define PROJ_TYPE_NONE            "none"

namespace similarity {

template <class dist_t>
class Projection {
public:
  /*
   * Create a projection helper class that inherits from the current one.
   */
  static Projection* createProjection(const Space<dist_t>& space,
                        const ObjectVector& data,
                        std::string projType,
                        size_t nProjDim,
                          /*
                             For sparse vector spaces and random projections
                             is used to create an intermediate dense vector representations.
                             It can be set to zero (or otherwise) if
                              1) The source space is a dense vector space
                              2) A projection is of the following type:
                                i)   FastMap;
                                ii)  Random reference points;
                                iii) Permutations.
                        */
                        size_t nDstDim,
                        unsigned binThreshold);
  /*
   * A function to create a projection. It should be implemented in child classes.
   * Note that the following:
   *
   * 1) we only support projection to single-precision floating point vectors.
   * 2) Space objects can be used to compute distances only during indexing time.
   *    At search time, all distance computations are proxied through a query object.
   *    During the indexing time, the query parameter can be NULL, but during
   *    search time, one needs to supply the actual query.
   *
   * TODO: @leo This is actually not such a good approach, we need a separate
   *       object to proxy distance computations. It should not be the query!
   *       Then, functions like compProj will not need to use both the pObj (at
   *       index time) and the pQuery (at search time).
   *
   */
  virtual void  compProj(const Query<dist_t>* pQuery,
                         const Object* pObj,
                         float* pDstVect) const = 0;

protected:
  static dist_t DistanceObjLeft(const Space<dist_t>& space,
                                const Query<dist_t>* pQuery,
                                const Object* pRefObj, // reference object
                                const Object* pObj // the object to project
                                ) {
    /*
     * Object (in this case pivot) is the left argument.
     * At search time, the right argument of the distance will be the query point
     * and pivot again is the left argument.
     */
    return NULL == pQuery ?
        space.IndexTimeDistance(pRefObj, pObj)
        :
        pQuery->DistanceObjLeft(pRefObj);
  }
  static void fillIntermBuffer(const  Space<dist_t>& space,
                               const  Object* pObj,
                               size_t nIntermDim,
                               vector<dist_t>& intermBuffer) {

    /*
     *  For dense vector spaces CreateDenseVectFromObj does nothing useful
     *  (and intermDim_ == srcDim_),
     *  however, we introduced this function to have the
     *  uniform interface for sparse and dense vector spaces.
     */
    space.CreateDenseVectFromObj(pObj, &intermBuffer[0], nIntermDim);
  }
};

}


#endif
