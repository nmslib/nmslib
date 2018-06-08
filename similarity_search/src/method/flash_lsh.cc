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
#include <thread>
#include <algorithm>

#include "space.h"
#include "rangequery.h"
#include "knnquery.h"
#include "method/flash_lsh.h"


namespace similarity {

template <typename dist_t>
void FlashLSH<dist_t>::copyData() {
  size_t elemQty = 0;

  vector<SparseVectElem<dist_t>> target;

  dataMarkers_.reserve(this->data_.size() + 1);
  dataIds_.reserve(this->data_.size());
  dataVals_.reserve(this->data_.size());

  for (size_t i = 0; i < this->data_.size(); ++i) {
    target.clear();
    UnpackSparseElements(this->data_[i]->data(), this->data_[i]->datalength(), target);
    for (const auto& e : target) {
      dataIds_.push_back(e.id_);
      dataVals_.push_back(e.val_);
    }
    dataMarkers_.push_back(elemQty);
    elemQty += target.size();
  }

  dataMarkers_.push_back(elemQty);

  // This value should be > than the average value
  flashDim_ = elemQty/ this->data_.size() + 2;

  LOG(LIB_INFO) << "Data copied: the total number of non-zero vector elements: " << elemQty;
}

template <typename dist_t>
void  FlashLSH<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  if (this->data_.empty()) return;
  AnyParamManager  pmgr(IndexParams);
  /*    pmgr.GetParamOptional("doSeqSearch",
                        bDoSeqSearch_,
                        false
                        );*/
  // Check if a user specified extra parameters, which can be also misspelled variants of existing ones

  pmgr.GetParamOptional("numTables", numTables_, 128);
  pmgr.GetParamOptional("lshK", lshK_, 4);
  // numSecHash is hardcoded as RANGE_ROW_U in benchmarking.h of the original FLASH code
  pmgr.GetParamOptional("numSecHash", numSecHash_, 15);
  // numHashPerFamily is hardcoded as RANGE_POW in benchmarking.h of the original FLASH code
  pmgr.GetParamOptional("numHashPerFamily", numHashPerFamily_, 15);
  pmgr.GetParamOptional("reservoirSize", reservoirSize_, 32);
  pmgr.GetParamOptional("queryProbes", queryProbes_, 1);
  // maxSamples is hardcoded as NUMBASE in benchmarking.h of the original FLASH code
  pmgr.GetParamOptional("maxSamples", maxSamples_, 2386130);
  pmgr.GetParamOptional("hashingProbes", hashingProbes_, 1);
  pmgr.GetParamOptional("occupancy", occupancy_, 1.0);
  pmgr.GetParamOptional("numHashBatch", numHashBatch_, 50);

  lshHash_.reset(new LSH(2, lshK_, numTables_, numHashPerFamily_));
  lshReservoir_.reset(new
                        LSHReservoirSampler(lshHash_.get(), numHashPerFamily_, numTables_, reservoirSize_,
                                            flashDim_, numSecHash_, maxSamples_,
                                            queryProbes_, hashingProbes_,
                                            occupancy_));

  pmgr.CheckUnused();
  // Always call ResetQueryTimeParams() to set query-time parameters to their default values
  this->ResetQueryTimeParams();

  lshReservoir_->showParams();

  size_t chunkSize = (this->data_.size() + numHashBatch_ - 1) / numHashBatch_;

  for (uint_fast32_t i1 = 0; i1 < this->data_.size(); i1 += chunkSize) {
    LOG(LIB_INFO) << "Indexing chunk: " << (i1/chunkSize + 1) << " out of " << numHashBatch_;
    auto i2 = std::min(this->data_.size() - 1, i1 + chunkSize);
    lshReservoir_->add(i2 - i1, &dataIds_[0], &dataVals_[0], &dataMarkers_[i1]);
  }
}


template <typename dist_t>
void FlashLSH<dist_t>::Search(RangeQuery<dist_t>* query, IdType) const {
  throw runtime_error("Range search is not supported!");
}

template <typename dist_t>
void FlashLSH<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  if (this->data_.empty()) return;
  auto k = query->GetK();

  vector<unsigned> queryOutputs(k);

  vector<SparseVectElem<dist_t>> target;

  UnpackSparseElements(query->QueryObject()->data(), query->QueryObject()->datalength(), target);

  int markers[2] = {0, target.size()};

  vector<int> ids(target.size());
  vector<float> vals(target.size());

  for (uint_fast32_t i = 0; i < target.size(); ++i) {
    ids[i] = target[i].id_;
    vals[i] = target[i].val_;
  }
  lshReservoir_->ann(1, &ids[0], &vals[0], markers, &queryOutputs[0], k);

  // This step as well as the conversion of the input vector
  // introduce a tiny overhead, but it's tiny
  for (unsigned resId : queryOutputs) {
    //cout << resId << " ";
    if (resId < this->data_.size())
      query->CheckAndAddToResult(this->data_[resId]);
  }
  cout << endl;
}


template class FlashLSH<float>;

}
