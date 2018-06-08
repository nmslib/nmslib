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
#ifndef FLASH_LSH_H
#define FLASH_LSH_H

#include <string>
#include <sstream>
#include <memory>


#include "space/space_sparse_vector_inter.h"
#include "flash_lsh/LSH.h"
#include "flash_lsh/LSHReservoirSampler.h"
#include "index.h"

#define METH_FLASH_LSH             "lsh_flash"

namespace similarity {

using std::string;

template <typename dist_t>
class FlashLSH : public Index<dist_t> {
 public:
  /*
   * The constructor here space and data-objects' references,
   * which are guaranteed to be be valid during testing.
   * So, we can memorize them safely.
   */
  FlashLSH(Space<dist_t>& space, 
              const ObjectVector& data) : Index<dist_t>(data), space_(space) {
     pSparseSpace_ = dynamic_cast<SpaceSparseVectorInter<dist_t>*>(&space_);

    if (pSparseSpace_ == nullptr) {
      throw runtime_error("Only dense vector spaces and FAST sparse vector spaces are supported!");
    }
    copyData();
  }


  void CreateIndex(const AnyParams& IndexParams) override;

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

  ~FlashLSH(){};

  /* 
   * Just the name of the method, consider printing crucial parameter values
   */
  const std::string StrDesc() const override { 
    return METH_FLASH_LSH;
  }

  /* 
   *  One needs to implement two search functions.
   */
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  virtual bool DuplicateData() const override { return true; }

  virtual void SetQueryTimeParams(const AnyParams& params) {}

 private:
  bool                            data_duplicate_;
  Space<dist_t>&                  space_;
  SpaceSparseVectorInter<dist_t>* pSparseSpace_;
  std::vector<int>                dataIds_;
  std::vector<float>              dataVals_;
  std::vector<int>                dataMarkers_;

  uint_fast32_t                   flashDim_;
  uint_fast32_t                   numTables_;
  uint_fast32_t                   lshK_;
  uint_fast32_t                   numHashPerFamily_;
  uint_fast32_t                   numSecHash_;
  uint_fast32_t                   reservoirSize_;
  uint_fast32_t                   queryProbes_;
  uint_fast32_t                   hashingProbes_;
  uint_fast32_t                   maxSamples_;
  uint_fast32_t                   numHashBatch_;
  float                           occupancy_;

  std::unique_ptr<::LSH>                  lshHash_;
  std::unique_ptr<::LSHReservoirSampler>  lshReservoir_;


  // Copy and store data in the format the FLASH can use
  void copyData();

  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(FlashLSH);
};

}   // namespace similarity

#endif     // _DUMMY_METHOD_H_
