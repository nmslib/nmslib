/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "space.h"
#include "logging.h"
#include "experimentconf.h"

namespace similarity {

template <typename dist_t>
void ExperimentConfig<dist_t>::SelectTestSet(int SetNum) {
  if (!NoQueryFile) return;
  if (SetNum <0 || static_cast<unsigned>(SetNum) >= TestSetQty) {
    LOG(FATAL) << "Invalid test set #: " << SetNum;
  }
  dataobjects.clear();
  queryobjects.clear();

  size_t OrigQty = OrigData.size();
  for (size_t i = 0; i < OrigQty; ++i) {
    int dst = OrigDataAssignment[i];

    /*
     * dst == -1  =>  the data point is always put to dataobjects
     * if dst >= 0 and dst != SetNum, the data point is designed
     * as a query in the test set # dst. If SetNum == dst, add this
     * data point to the query set. Otherwise, it goes to the data set.
     */
    if (dst == SetNum) queryobjects.push_back(OrigData[i]);
    else dataobjects.push_back(OrigData[i]);
  }
}

template <typename dist_t>
void ExperimentConfig<dist_t>::ReadDataset() {
  CHECK(space != NULL);
  CHECK(dataobjects.empty());
  CHECK(queryobjects.empty());

  space->ReadDataset(OrigData, this, datafile.c_str(), MaxNumData);

  /*
   * Note!!! 
   * The destructor of this class will delete pointers stored in OrigData & OrigQuery.
   * Applications should not delete objects whose pointers are stored in arrays:
   * 1) dataobjects 
   * 2) queryobjects.
   */
  if (!NoQueryFile) {
    dataobjects = OrigData;
    space->ReadDataset(queryobjects, this, queryfile.c_str(), MaxNumQuery);
    OrigQuery = queryobjects;
  } else {
    size_t OrigQty = OrigData.size();
    size_t MinOrigQty = (TestSetQty + 1) * MaxNumQuery;
    if (OrigQty < MinOrigQty) {
      LOG(FATAL) << "The data set is too small, expecting at least: " << MinOrigQty << " data points. " <<
                    "Try to either increase the number of data points, or to decrease parameters: " <<
                    "testSetQty and/or maxNumQuery ";
    }
    OrigDataAssignment.resize(OrigQty);
    fill(OrigDataAssignment.begin(), OrigDataAssignment.end(), -1);

    /* 
     * Test queries are selected randomly without resampling.
     * The algorithm of efficient sampling without replacement 
     * was based on conclusions of D. Lemire's:
     * https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2013/08/14/cpp/synthetic.h 
     */

    for (unsigned i = 0; i < TestSetQty; ++i) {
      size_t        card = 0;

      while (card < MaxNumQuery) {
        size_t Id = RandomInt() % OrigQty;
      
        if (-1 == OrigDataAssignment[Id]) {
          OrigDataAssignment[Id] = i;
          ++card;
        }
      }
    }
  }

  LOG(INFO) << "data & query .... ok!\n";
}

template <typename dist_t>
void ExperimentConfig<dist_t>::PrintInfo() const {
  space->PrintInfo();
  LOG(INFO) << "distance type         = " << DistTypeName<dist_t>();
  LOG(INFO) << "data file             = " << datafile;
  LOG(INFO) << "# of test sets        = " << GetTestSetQty();
  LOG(INFO) << "Use held-out queries  = " << !NoQueryFile;
  LOG(INFO) << "# of data points      = " << OrigData.size() - GetQueryQty();
  LOG(INFO) << "# of query points     = " << GetQueryQty();
}

template class ExperimentConfig<float>;
template class ExperimentConfig<double>;
template class ExperimentConfig<int>;

}

