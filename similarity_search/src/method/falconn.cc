/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2016
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"

#include <string>

using namespace std;


#include <space/space_sparse_vector_inter.h>
#include <space/space_vector.h>

#include "method/falconn.h"

#include <falconn/core/polytope_hash.h>

const string PARAM_HASH_FAMILY                = "hash_family";
const string PARAM_HASH_FAMILY_CROSS_POLYTOPE = "polytope";
const string PARAM_HASH_FAMILY_HYPERPLANE     = "hyperplane";

const string PARAM_STORAGE_HASH_TABLE = "storage_hash_table";
const string PARAM_NUM_HASH_BITS      = "num_hash_bits";
const string PARAM_NUM_HASH_TABLES    = "num_hash_tables";
const string PARAM_NUM_PROBES         = "num_probes"; // This is going to be a query-time parameter
const string PARAM_NUM_ROTATIONS      = "num_rotations";
const string PARAM_SEED               = "seed";
const string PARAM_FEATURE_HASHING_DIM = "feature_hashing_dimension";


namespace similarity {

using namespace falconn;

template <typename dist_t>
FALCONN<dist_t>::FALCONN(Space<dist_t>& space,
                              const ObjectVector& data) : data_(data), space_(space), sparse_(false),
                                                          dim_(0), num_probes_(10) {
  SpaceSparseVectorInter<dist_t>* pSparseSpace = dynamic_cast<SpaceSparseVectorInter<dist_t>*>(&space_);
  VectorSpace<dist_t>*            pDenseSpace  = dynamic_cast<VectorSpace<dist_t>*>(&space_);

  if (data_.empty()) return;
  if (pSparseSpace == nullptr && pDenseSpace == nullptr) {
    throw runtime_error("Only sparse and dense vector spaces are supported!");
  }
  if (pSparseSpace != nullptr) {
    LOG(LIB_INFO) << "Creating a sparse vector data set!";
    sparse_ = true;
    for (const Object* o: data_) {
      vector<SparseVectElem<dist_t>> target;
      UnpackSparseElements(o->data(), o->datalength(), target);
      SparseFalconnPoint p;
      for (const SparseVectElem<dist_t>& e : target)
        p.push_back(make_pair(e.id_, e.val_));
      falconn_data_sparse_.push_back(p);
    }
  }
  if (pDenseSpace != nullptr) {
    LOG(LIB_INFO) << "Creating a dense vector data set!";
    dim_ = data_[0]->datalength() / sizeof(dist_t);
    DenseFalconnPoint p(dim_);
    for (const Object* o: data_) {
      const dist_t *pVect = reinterpret_cast<const dist_t*>(o->data());
      for (size_t i = 0; i < dim_; ++i) p[i] = pVect[i];
      falconn_data_dense_.emplace_back(p);
    }
  }
}

template <typename dist_t>
void FALCONN<dist_t>::CreateIndex(const AnyParams& IndexParams)  {
  AnyParamManager  pmgr(IndexParams);

  LSHConstructionParameters params;
  params.distance_function = DistanceFunction::EuclideanSquared;

  pmgr.GetParamOptional(PARAM_SEED, params.seed, 4057218);


  {
    string lsh_family_str;
    pmgr.GetParamOptional(PARAM_HASH_FAMILY, lsh_family_str, PARAM_HASH_FAMILY_CROSS_POLYTOPE);
    ToLower(lsh_family_str);
    if (lsh_family_str == PARAM_HASH_FAMILY_CROSS_POLYTOPE) {
      params.lsh_family = LSHFamily::CrossPolytope;
    } else if (lsh_family_str == PARAM_HASH_FAMILY_HYPERPLANE) {
      params.lsh_family = LSHFamily::Hyperplane;
    } else {
      PREPARE_RUNTIME_ERR(err)
      << "Unrecognized value of " << PARAM_HASH_FAMILY << " expected one of: "
      << PARAM_HASH_FAMILY_CROSS_POLYTOPE << ", " << PARAM_HASH_FAMILY_HYPERPLANE;
      THROW_RUNTIME_ERR(err);
    }
  }

  /*
   * The number of hash tables affects the memory usage. According to FALCONN's manual,
   * the larger it is, the better (unless it's too large).
   * They also recommend to start with a small number, e.g., 10-50 tables,
   * and then increase it gradually, while observing the effect it makes.
   */
  pmgr.GetParamOptional(PARAM_NUM_HASH_TABLES, params.l, 50);
  /*
   * This parameter controls the number of buckets per table,
   * According to FALCONN's manual, it should be approx. equal to
   * the binary logarithm of the number of data points
   */
  size_t num_hash_bits = max<size_t>(2, static_cast<size_t>(ceil(log2(data_.size()))));
  pmgr.GetParamOptional(PARAM_NUM_HASH_BITS, num_hash_bits, num_hash_bits);



  /*
   *  Number of rotations controls the number of pseudo-random rotations for
   *  the cross-polytope LSH, FALCONN's manual advised to set it to 1 for the dense data,
   *  and 2 for the sparse data.
   */
  size_t num_rotations_default = sparse_ ? 2 : 1;
  pmgr.GetParamOptional(PARAM_NUM_ROTATIONS, params.num_rotations, num_rotations_default);

  if (!sparse_) {
    params.dimension = dim_;
    compute_number_of_hash_functions<DenseFalconnPoint>(num_hash_bits, &params);
  } else {
    pmgr.GetParamRequired(PARAM_FEATURE_HASHING_DIM, params.feature_hashing_dimension);
    compute_number_of_hash_functions<SparseFalconnPoint>(num_hash_bits, &params);
  }

  // Check if a user specified extra parameters, which can be also misspelled variants of existing ones
  pmgr.CheckUnused();
  // Always call ResetQueryTimeParams() to set query-time parameters to their default values
  this->ResetQueryTimeParams();
}


template <typename dist_t>
void FALCONN<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  throw runtime_error("To be implemented!");
}

template <typename dist_t>
void 
FALCONN<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);

  pmgr.GetParamOptional(PARAM_NUM_PROBES, num_probes_, num_probes_);
  LOG(LIB_INFO) << PARAM_NUM_PROBES << " = " << num_probes_;
  if (sparse_) falconn_table_sparse_->set_num_probes(num_probes_);
  else falconn_table_dense_->set_num_probes(num_probes_);
  pmgr.CheckUnused();
}

template class FALCONN<float>;
template class FALCONN<double>;

}
