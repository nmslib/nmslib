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
#include <cmath>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"

#include <string>

using namespace std;


#include <space/space_sparse_vector_inter.h>
#include <space/space_vector.h>

#include "method/falconn.h"

using namespace falconn::wrapper;

const string PARAM_HASH_FAMILY                = "hash_family";
const string PARAM_HASH_FAMILY_CROSS_POLYTOPE = "cross_polytope";
const string PARAM_HASH_FAMILY_HYPERPLANE     = "hyperplane";

const string PARAM_STORAGE_HASH_TABLE   = "storage_hash_table";
const string PARAM_NUM_HASH_BITS        = "num_hash_bits";
const string PARAM_NUM_HASH_TABLES      = "num_hash_tables";
const string PARAM_NUM_PROBES           = "num_probes"; // This is going to be a query-time parameter
const string PARAM_NUM_ROTATIONS        = "num_rotations";
const string PARAM_SEED                 = "seed";
const string PARAM_FEATURE_HASHING_DIM  = "feature_hashing_dimension";
const string PARAM_NORM_DATA            = "norm_data";
const string PARAM_CENTER_DATA          = "center_data";
const string PARAM_MAX_SPARSE_DIM_TO_CENTER = "max_sparse_dim_to_center";
const string PARAM_USE_FALCONN_DIST     = "use_falconn_dist";

const size_t MAX_DIM_TO_PRINT                 = 1000;
const size_t MAX_SPARSE_DIM_TO_CENTER_DEFAULT = 10000;

namespace similarity {

using namespace falconn;

// storage_hash_table_from_string is a modification of FALCONN's python_wrapper.h
StorageHashTable storage_hash_table_from_string(string str) {
  ToLower(str);
  for (int ii = 0; ii < static_cast<int>(kStorageHashTableStrings.size());
       ++ii) {
    if (str == kStorageHashTableStrings[ii]) {
      return StorageHashTable(ii);
    }
  }
  stringstream err;
  err << "Unknown value of the parameter " << PARAM_STORAGE_HASH_TABLE << " valid values are: ";
  for (int ii = 0; ii < static_cast<int>(kStorageHashTableStrings.size()); ++ii) {
    if (ii) err << ",";
    err << kStorageHashTableStrings[ii];
  }
  throw runtime_error(err.str());
}

template <typename dist_t>
 void FALCONN<dist_t>::createDenseDataPoint(const Object* o, DenseFalconnPoint& p) const {
  CHECK(sizeof(dist_t)*dim_ == o->datalength());
  const dist_t *pVect = reinterpret_cast<const dist_t*>(o->data());
  for (size_t i = 0; i < dim_; ++i) {
    p[i] = pVect[i];
  }
}

template <typename dist_t>
void FALCONN<dist_t>::createSparseDataPoint(const Object* o, SparseFalconnPoint& p) const {
  p.clear();
  vector<SparseVectElem<dist_t>> target;
  UnpackSparseElements(o->data(), o->datalength(), target);

  for (const SparseVectElem<dist_t>& e : target) {
    p.push_back(make_pair(e.id_, e.val_));
  }
}

template <typename dist_t>
void FALCONN<dist_t>::copyData() {
  SpaceSparseVectorInter<dist_t>* pSparseSpace = dynamic_cast<SpaceSparseVectorInter<dist_t>*>(&space_);
  VectorSpace<dist_t>*            pDenseSpace  = dynamic_cast<VectorSpace<dist_t>*>(&space_);

  if (this->data_.empty()) return;
  if (pSparseSpace == nullptr && pDenseSpace == nullptr) {
    throw runtime_error("Only dense vector spaces and sparse vector spaces are supported!");
  }
  if (pSparseSpace != nullptr) {
    LOG(LIB_INFO) << "Copying a sparse vector data set.";
    sparse_ = true;
    SparseFalconnPoint p;
    dim_ = 0;
    for (const Object* o: this->data_) {
      createSparseDataPoint(o, p);
      falconn_data_sparse_.push_back(p);
      if (p.size())
        dim_ = max<size_t>(dim_, p.back().first+1);
    }
  }
  if (pDenseSpace != nullptr) {
    LOG(LIB_INFO) << "Copying a dense vector data set.";
    dim_ = this->data_[0]->datalength() / sizeof(dist_t);
    DenseFalconnPoint p(dim_);
    for (const Object* o: this->data_) {
      createDenseDataPoint(o, p);
      falconn_data_dense_.emplace_back(p);
    }
  }
  LOG(LIB_INFO) << "Dataset is copied.";
}


template <typename dist_t>
FALCONN<dist_t>::FALCONN(Space<dist_t>& space,
                         const ObjectVector& data) : Index<dist_t>(data), space_(space), sparse_(false),
                                                     dim_(0), num_probes_(10) {
}


template <typename dist_t>
void FALCONN<dist_t>::CreateIndex(const AnyParams& IndexParams)  {
  copyData();

  AnyParamManager  pmgr(IndexParams);

  LSHConstructionParameters params;
  params.distance_function = DistanceFunction::EuclideanSquared;
  // we want to use all the available threads to set up
  params.num_setup_threads = 0;

  pmgr.GetParamOptional(PARAM_SEED, params.seed, 4057218);

  {
    string hash_table_storage_type;
    ToLower(hash_table_storage_type);
    // The flat hash table seems to be the most efficient
    pmgr.GetParamOptional(PARAM_STORAGE_HASH_TABLE, hash_table_storage_type, "flat_hash_table");
    params.storage_hash_table = storage_hash_table_from_string(hash_table_storage_type);
  }

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
   * the binary logarithm of the number of data points.
   *
   * The current formula mimics the Glove setup in ann_benchmarks,
   * where a roughly 2^20 entry data sets uses 16-bit tables.
   */
  size_t num_hash_bits = max<size_t>(2, static_cast<size_t>(floor(log2(this->data_.size()>>4))));
  pmgr.GetParamOptional(PARAM_NUM_HASH_BITS, num_hash_bits, num_hash_bits);

  /*
   *  Number of rotations controls the number of pseudo-random rotations for
   *  the cross-polytope LSH, FALCONN's manual advised to set it to 1 for the dense data,
   *  and 2 for the sparse data.
   */
  size_t num_rotations_default = sparse_ ? 2 : 1;
  pmgr.GetParamOptional(PARAM_NUM_ROTATIONS, params.num_rotations, num_rotations_default);

  params.dimension = dim_;

  LOG(LIB_INFO) << "#dim:                           " << params.dimension;

  if (!sparse_) {
    compute_number_of_hash_functions<DenseFalconnPoint>(num_hash_bits, &params);
  } else {
    pmgr.GetParamRequired(PARAM_FEATURE_HASHING_DIM, params.feature_hashing_dimension);
    compute_number_of_hash_functions<SparseFalconnPoint>(num_hash_bits, &params);
  }

  LOG(LIB_INFO) << "#of feature-hash dim:           " << params.feature_hashing_dimension;

  LOG(LIB_INFO) << "Hash family:                    " << kLSHFamilyStrings[(size_t)params.lsh_family];
  LOG(LIB_INFO) << "Table storage type:             " << kStorageHashTableStrings[(size_t)params.storage_hash_table];

  LOG(LIB_INFO) << "#of hash tables:                " << params.l;
  LOG(LIB_INFO) << "#of hash func. per table:       " << params.k;
  LOG(LIB_INFO) << "#of hash bits:                  " << num_hash_bits;
  LOG(LIB_INFO) << "#of rotations:                  " << params.num_rotations;

  LOG(LIB_INFO) << "seed:                           " << params.seed;

  // Check if a user specified extra parameters, which can be also misspelled variants of existing ones
  pmgr.CheckUnused();

  if (!sparse_) {
    falconn_table_dense_
        = construct_table<DenseFalconnPoint, IdType>(falconn_data_dense_, params);
  } else {
    falconn_table_sparse_
        = construct_table<SparseFalconnPoint, IdType>(falconn_data_sparse_, params);
  }


  // Always call ResetQueryTimeParams() to set query-time parameters to their default values
  this->ResetQueryTimeParams();
}


template <typename dist_t>
void FALCONN<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  vector<IdType> ids;

  if (sparse_) {
    SparseFalconnPoint sparseQ;
    createSparseDataPoint(query->QueryObject(), sparseQ);

    unique_ptr<LSHNearestNeighborQuery<SparseFalconnPoint>> falconnQuery =
      falconn_table_sparse_->construct_query_object(num_probes_);

    falconnQuery->find_k_nearest_neighbors(sparseQ, query->GetK(), &ids);
  } else {
    DenseFalconnPoint denseQ(dim_);
    createDenseDataPoint(query->QueryObject(), denseQ);

    unique_ptr<LSHNearestNeighborQuery<DenseFalconnPoint>> falconnQuery =
      falconn_table_dense_->construct_query_object(num_probes_);

    falconnQuery->find_k_nearest_neighbors(denseQ, query->GetK(), &ids);
  }
  // Recomputing distances for k nearest neighbors should have a very small impact on overall performance
  for (IdType ii : ids) {
    query->CheckAndAddToResult(this->data_[ii]);
  }

}

template <typename dist_t>
void 
FALCONN<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  AnyParamManager pmgr(QueryTimeParams);

  pmgr.GetParamOptional(PARAM_NUM_PROBES, num_probes_, num_probes_);
  LOG(LIB_INFO) << PARAM_NUM_PROBES << " = " << num_probes_;

  pmgr.CheckUnused();
}

template class FALCONN<float>;
template class FALCONN<double>;

}
