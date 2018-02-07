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
#include <vector>
#include <stdexcept>
#include <limits>
#include <cmath>

#include "utils.h"
#include "projection.h"

#include "randproj_util.h"
#include "permutation_utils.h"

#include "space/space_vector.h"

namespace similarity {

using namespace std;

/*
 * Convert to a dense vector. If the space is already a dense-vector
 * space, this should be an identity transformation.
 */

template <class dist_t>
class ProjectionVectDense : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    if (NULL == pObj) pObj = pQuery->QueryObject();
    vector<dist_t> intermBuffer(dstDim_);
    Projection<dist_t>::fillIntermBuffer(space_,
                                         pObj,
                                         dstDim_,
                                         intermBuffer);
    for (size_t i = 0; i < dstDim_; ++i)
      pDstVect[i] = static_cast<float>(intermBuffer[i]);
  }

  friend class Projection<dist_t>;
private:
  ProjectionVectDense(const Space<dist_t>& space, size_t nDstDim) :
    space_(space), dstDim_(nDstDim) {
  }

  const Space<dist_t>& space_;
  size_t dstDim_;

};


/*
 * Classic random projections.
 */

template <class dist_t>
class ProjectionRand : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    if (NULL == pObj) pObj = pQuery->QueryObject();
    /*
     * For dense vector spaces, we ignore the specified "projection"
     * dimensionality and use an actual number of vector elements. 
     * For sparse vector spaces, we obtain an intermediate dense vector 
     * with projDim elements.
     */
    size_t nDim = space_.GetElemQty(pObj);
    if (!nDim) nDim = projDim_;
    vector<dist_t> intermBuffer(nDim);
    Projection<dist_t>::fillIntermBuffer(space_,
                                         pObj,
                                         nDim,
                                         intermBuffer);

    vector<dist_t> dstBuffer(dstDim_);
    compRandProj<dist_t>(_projMatr, &intermBuffer[0],
                         nDim,
                         &dstBuffer[0],
                         dstDim_);
    for (size_t i = 0; i < dstDim_; ++i)
      pDstVect[i] = static_cast<float>(dstBuffer[i]);
  }

  friend class Projection<dist_t>;
private:
  ProjectionRand(const Space<dist_t>& space, const ObjectVector& data,
                 size_t nProjDim, size_t nDstDim, bool bDoOrth) :
    space_(space), projDim_(nProjDim), dstDim_(nDstDim) {
    if (data.empty()) {
      stringstream err;
      err << "Cannot initialize projection type '" <<
             PROJ_TYPE_RAND << "'" <<
             " without a single data point";
      throw runtime_error(err.str());
    }
    size_t nDim = space.GetElemQty(data[0]);
    if (nDim == 0) {
      if (!projDim_) {
        throw runtime_error("Specify a non-zero value for the intermediate dimensionaity.");
      }
      nDim = projDim_;
    }

    initRandProj(nDim, dstDim_, bDoOrth, _projMatr);
  }

  vector<vector<dist_t>>    _projMatr;
  const Space<dist_t>& space_;
  size_t projDim_;
  size_t dstDim_;

};

/*
 * Distances to random reference points.
 */
template <class dist_t>
class ProjectionRandRefPoint : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    for (size_t i = 0; i < dstDim_; ++i) {
      pDstVect[i] = static_cast<float>(
          Projection<dist_t>::DistanceObjLeft(space_, pQuery,
                                              ref_pts_[i], pObj));
    }
  }

  friend class Projection<dist_t>;
private:
  ProjectionRandRefPoint(const Space<dist_t>& space,
                         const ObjectVector& data,
                         size_t nDstDim) :
                         space_(space), data_(data),
                         dstDim_(nDstDim) {


    CHECK(data_.size() > dstDim_);

    GetPermutationPivot(data_, space_, dstDim_, &ref_pts_);

  }

  const Space<dist_t>&  space_;
  const ObjectVector&   data_;
  ObjectVector          ref_pts_;
  size_t dstDim_;

};

/*
 * Permutations.
 * See, Edgar Chávez et al., Effective Proximity Retrieval by Ordering Permutations.
 *                           IEEE Trans. Pattern Anal. Mach. Intell. (2008)
 *
 */
template <typename dist_t>
class ProjectionPermutation : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    Permutation perm;

    if (NULL == pQuery) {
      GetPermutation(ref_pts_, space_, pObj, &perm);
    } else {
      GetPermutation(ref_pts_, pQuery, &perm);
    }

    for (size_t i = 0; i < dstDim_; ++i) {
      pDstVect[i] = static_cast<float>(perm[i]);
    }

  }

  friend class Projection<dist_t>;

private:
  ProjectionPermutation(const Space<dist_t>& space,
                         const ObjectVector& data,
                         size_t nDstDim) : space_(space), data_(data),
                                           dstDim_(nDstDim) {
    GetPermutationPivot(data_, space_, nDstDim, &ref_pts_);
  }
  const Space<dist_t>&        space_;
  const ObjectVector&         data_;
  ObjectVector                ref_pts_;
  size_t dstDim_;
};

template <typename dist_t>
class ProjectionPermutationTrunc : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    Permutation perm;

    if (NULL == pQuery) {
      GetPermutation(ref_pts_, space_, pObj, &perm);
    } else {
      GetPermutation(ref_pts_, pQuery, &perm);
    }

    for (size_t i = 0; i < dstDim_; ++i) {
      pDstVect[i] = perm[i] <= trunc_threshold_ ? static_cast<float>(perm[i]) : 0;
    }

  }

  friend class Projection<dist_t>;

private:
  ProjectionPermutationTrunc(const Space<dist_t>& space,
                             const ObjectVector& data,
                             size_t nDstDim,
                             unsigned trunc_threshold) : space_(space), data_(data),
                                           dstDim_(nDstDim), 
                                           trunc_threshold_(trunc_threshold) {
    GetPermutationPivot(data_, space_, nDstDim, &ref_pts_);
  }
  const Space<dist_t>&        space_;
  const ObjectVector&         data_;
  ObjectVector                ref_pts_;
  size_t                      dstDim_;
  unsigned                    trunc_threshold_;
};

// Binarized permutations
template <typename dist_t>
class ProjectionPermutationBin : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    Permutation perm;

    if (NULL == pQuery) {
      GetPermutation(ref_pts_, space_, pObj, &perm);
    } else {
      GetPermutation(ref_pts_, pQuery, &perm);
    }

    for (size_t i = 0; i < dstDim_; ++i) {
      // See also the function binarize
      pDstVect[i] = static_cast<float>(perm[i] >= binThreshold_ ? 1:0);
    }

  }

  friend class Projection<dist_t>;

private:
  ProjectionPermutationBin(const Space<dist_t>& space,
                         const ObjectVector& data,
                         size_t nDstDim,
                         unsigned binThreshold)
                                          : space_(space), data_(data),
                                           dstDim_(nDstDim),
                                           binThreshold_(binThreshold) {
    GetPermutationPivot(data_, space_, nDstDim, &ref_pts_);
  }
  const Space<dist_t>&        space_;
  const ObjectVector&         data_;
  ObjectVector                ref_pts_;
  size_t dstDim_;
  unsigned binThreshold_;
};

/*
 * FastMap, see.
 *
 * Christos Faloutsos & King-Ip (David) Lin.
 * FastMap: A Fast Algorithm for Indexing, Data-Mining and
 * Visualization of Traditional and Multimedia Datasets,
 *
 */
template <typename dist_t>
class ProjectionFastMap : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {


    for (size_t i = 0; i < dstDim_; ++i) {
      dist_t distAI = Projection<dist_t>::DistanceObjLeft(space_, pQuery,
                                                          ref_pts_a_[i], pObj);
      dist_t distBI = Projection<dist_t>::DistanceObjLeft(space_, pQuery,
                                                          ref_pts_b_[i], pObj);
      dist_t distAB = dist_ab_[i];
      pDstVect[i] = static_cast<float>(
                                  (
                                  (distAI*distAI) - (distBI*distBI) + (distAB*distAB)
                                  )/(distAB * dist_t(2))
                                  );
    }

  }

  friend class Projection<dist_t>;

private:
  ProjectionFastMap(const Space<dist_t>& space,
                    const ObjectVector& data,
                    size_t nDstDim) : space_(space), data_(data),
                                      dstDim_(nDstDim) {
    ref_pts_a_.resize(nDstDim);
    ref_pts_b_.resize(nDstDim);
    dist_ab_.resize(nDstDim);

    dist_t eps = numeric_limits<dist_t>::epsilon() * 2; /* two is a bit ad-hoc */

    for (size_t i = 0; i < nDstDim; ++i) {
      std::unordered_set<int> pivot_idx;

      for (size_t rep = 0; ; ++rep) {
        if (rep > MAX_RAND_ITER_BEFORE_GIVE_UP) {
          throw runtime_error("Cannot find the next pair of pivot, perhaps, the data set is too small.");
        }
        int pa = RandomInt() % data.size(),
            pb = RandomInt() % data.size();
        dist_ab_[i] = space_.IndexTimeDistance(data[pa], data[pb]);
        if (pivot_idx.count(pa) !=0 || pivot_idx.count(pb) !=0 ||
            fabs((double)dist_ab_[i]) < eps) continue;
        pivot_idx.insert(pa);
        pivot_idx.insert(pb);
        ref_pts_a_[i] = data_[pa];
        ref_pts_b_[i] = data_[pb];
        break;
      }
    }
  }
  const Space<dist_t>&        space_;
  const ObjectVector&         data_;
  ObjectVector                ref_pts_a_;
  ObjectVector                ref_pts_b_;
  vector<dist_t>              dist_ab_;
  size_t dstDim_;
};

template <class dist_t>
class ProjectionNone : public Projection<dist_t> {
public:
  virtual void compProj(const Query<dist_t>* pQuery,
                        const Object* pObj,
                        float* pDstVect) const {
    if (NULL == pObj) pObj = pQuery->QueryObject();
    /*
     * For dense vector spaces, we ignore the specified "projection"
     * dimensionality and use an actual number of vector elements. 
     * For sparse vector spaces, we obtain an intermediate dense vector 
     * with projDim elements.
     */
    size_t nDim = space_.GetElemQty(pObj);
    if (!nDim) nDim = projDim_;
    vector<dist_t> intermBuffer(nDim);
    Projection<dist_t>::fillIntermBuffer(space_,
                                         pObj,
                                         nDim,
                                         intermBuffer);

    CHECK(nDim == dstDim_);

    for (size_t i = 0; i < dstDim_; ++i)
      pDstVect[i] = static_cast<float>(intermBuffer[i]);
  }

  friend class Projection<dist_t>;
private:
  ProjectionNone(const Space<dist_t>& space, const ObjectVector& data,
                 size_t nProjDim, size_t nDstDim) :
    space_(space), projDim_(nProjDim), dstDim_(nDstDim) {
    if (data.empty()) {
      stringstream err;
      err << "Cannot initialize projection type '" <<
             PROJ_TYPE_RAND << "'" <<
             " without a single data point";
      throw runtime_error(err.str());
    }
    size_t nDim = space.GetElemQty(data[0]);
    if (nDim == 0) {
      if (!projDim_) {
        throw runtime_error("Specify a non-zero value for the intermediate dimensionaity.");
      }
      nDim = projDim_;
    }
    if (nDim != dstDim_) {
      throw runtime_error("The dimensionality of the projected space should be equal to either source or to the intermediate dimensionality!");
    }
  }

  const Space<dist_t>& space_;
  size_t projDim_;
  size_t dstDim_;

};


/*
 * Projection factory function.
 */

template <class dist_t>
Projection<dist_t>*
Projection<dist_t>::createProjection(const Space<dist_t>& space,
                                     const ObjectVector& data,
                                     string projType,
                                     size_t nProjDim,
                                     size_t nDstDim,
                                     unsigned binThreshold) {
  ToLower(projType);

  if (PROJ_TYPE_RAND == projType) {
    return new ProjectionRand<dist_t>(space, data, nProjDim, nDstDim, true);
  } else if (PROJ_TYPE_RAND_REF_POINT == projType) {
    return new ProjectionRandRefPoint<dist_t>(space, data, nDstDim);
  } else if (PROJ_TYPE_PERM == projType) {
    return new ProjectionPermutation<dist_t>(space, data, nDstDim);
  } else if (PROJ_TYPE_PERM_TRUNC == projType) {
    return new ProjectionPermutationTrunc<dist_t>(space, data, nDstDim, binThreshold);
  } else if (PROJ_TYPE_PERM_BIN == projType) {
    return new ProjectionPermutationBin<dist_t>(space, data, nDstDim, binThreshold);
  } else if (PROJ_TYPE_VECTOR_DENSE == projType) {
    return new ProjectionVectDense<dist_t>(space, nDstDim);
  } else if (PROJ_TYPE_FAST_MAP == projType) {
    return new ProjectionFastMap<dist_t>(space, data, nDstDim);
  } else if (PROJ_TYPE_NONE == projType) {
    return new ProjectionNone<dist_t>(space, data, nProjDim, nDstDim);
  }

  throw runtime_error("Unknown projection type '" + projType + "'");

  return NULL;
}


template class Projection<float>;
template class Projection<double>;
template class Projection<int>;

template class ProjectionRand<float>;
template class ProjectionRand<double>;
template class ProjectionRand<int>;

template class ProjectionRandRefPoint<float>;
template class ProjectionRandRefPoint<double>;
template class ProjectionRandRefPoint<int>;

template class ProjectionPermutation<float>;
template class ProjectionPermutation<double>;
template class ProjectionPermutation<int>;

template class ProjectionFastMap<float>;
template class ProjectionFastMap<double>;
template class ProjectionFastMap<int>;

template class ProjectionNone<float>;
template class ProjectionNone<double>;
template class ProjectionNone<int>;

}

