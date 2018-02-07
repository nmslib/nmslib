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
#ifndef _FALCONN_METHOD_H_
#define _FALCONN_METHOD_H_

#include <string>
#include <sstream>
#include <memory>

#include <falconn/core/data_storage.h>
#include <falconn/lsh_nn_table.h>

#include "index.h"

#define METH_FALCONN             "falconn"

namespace similarity {

using std::string;

/*
 * This is a wrapper for FALCONN library: https://github.com/FALCONN-LIB/FALCONN
 *
 * Practical and Optimal LSH for Angular Distance
 * Alexandr Andoni, Piotr Indyk, Thijs Laarhoven, Ilya Razenshteyn, Ludwig Schmidt
 * NIPS 2015
 *
 */

template <typename dist_t>
class FALCONN : public Index<dist_t> {
 public:
  FALCONN(Space<dist_t>& space, const ObjectVector& data);

  virtual void CreateIndex(const AnyParams& IndexParams) override;

  /*
   * One can implement functions for index serialization and reading.
   * However, this is not required.
   */
  // SaveIndex is not necessarily implemented
  virtual void SaveIndex(const string& location) override {
    throw runtime_error("SaveIndex is not implemented for method: " + StrDesc());
  }
  // LoadIndex is not necessarily implemented
  virtual void LoadIndex(const string& location) override {
    throw runtime_error("LoadIndex is not implemented for method: " + StrDesc());
  }

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;

  ~FALCONN(){};

  const std::string StrDesc() const override { return METH_FALCONN; }

  void Search(RangeQuery<dist_t>* query, IdType) const override {
    throw runtime_error("Range search is not supported!");
  }
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  // We do duplicate the data set here
  virtual bool DuplicateData() const override { return true; }

 private:

  typedef falconn::DenseVector<dist_t>    DenseFalconnPoint;
  typedef falconn::SparseVector<dist_t>   SparseFalconnPoint;

  void copyData(bool normData, bool centerData, size_t max_sparse_dim_to_center);
  void createSparseDataPoint(const Object* o, SparseFalconnPoint& p, bool normData) const;
  // createDenseDataPoint assumes that p was initialized using dim_ as the number of elements.
  void createDenseDataPoint(const Object* o, DenseFalconnPoint& p, bool normData) const;

  Space<dist_t>&          space_;
  bool                    sparse_;
  size_t                  dim_; // only for dense vector spaces
  size_t                  num_probes_;
  bool                    norm_data_;
  bool                    center_data_;
  size_t                  max_sparse_dim_to_center_;
  bool                    use_falconn_dist_;

  vector<DenseFalconnPoint> falconn_data_dense_;
  vector<SparseFalconnPoint> falconn_data_sparse_;

  unique_ptr<DenseFalconnPoint>  center_point_;

  std::unique_ptr<falconn::LSHNearestNeighborTable<DenseFalconnPoint, int32_t>>   falconn_table_dense_;
  std::unique_ptr<falconn::LSHNearestNeighborTable<SparseFalconnPoint, int32_t>>  falconn_table_sparse_;

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(FALCONN);


};

}   // namespace similarity

#endif     // _FALCONN_METHOD_H_
