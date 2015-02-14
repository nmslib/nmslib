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

#ifndef _EXPERIMENT_CONFIG_H_
#define _EXPERIMENT_CONFIG_H_

#include <string.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"

namespace similarity {

using std::string;
using std::stringstream;
using std::vector;
using std::unordered_map;

template <typename dist_t>
class ExperimentConfig {
public:
  ExperimentConfig(Space<dist_t>* space,
                   const string& datafile,
                   const string& queryfile,
                   unsigned TestSetQty, // The # of times the datafile is randomly divided into the query and the test set
                   unsigned MaxNumData,
                   unsigned MaxNumQueryToRun,
                   unsigned dimension,
                   const typename std::vector<unsigned>& knn,
                   float eps,
                   const typename std::vector<dist_t>& range)
      : space_(reinterpret_cast<Space<dist_t>*>(space)),
        datafile_(datafile),
        queryfile_(queryfile),
        noQueryFile_(queryfile.empty()),
        testSetToRunQty_(TestSetQty),
        testSetQty_(TestSetQty),
        maxNumData_(MaxNumData),
        maxNumQuery_(MaxNumQueryToRun),
        maxNumQueryToRun_(MaxNumQueryToRun),
        dimension_(dimension),
        range_(range),
        knn_(knn),
        eps_(eps) {
    if (noQueryFile_) {
      if (!testSetToRunQty_) {
        throw runtime_error(
            "Bad configuration. One should either specify a query file, "
            " or the number of test sets obtained by bootstrapping "
            "(random division into query and data files).");
      }
    }
  }

  ~ExperimentConfig() {
    delete space_;
    for (auto it = origData_.begin(); it != origData_.end(); ++it) {
      delete *it;
    }
    for (auto it = origQuery_.begin(); it != origQuery_.end(); ++it) {
      delete *it;
    }
  }

  void ReadDataset();
  void PrintInfo() const;
  void SelectTestSet(int SetNum);
  int GetTestSetToRunQty() const {
    if (!noQueryFile_) return 1;
    return testSetToRunQty_;
  }
  int GetTestSetTotalQty() const {
    if (!noQueryFile_) return 1;
    return testSetQty_;
  }
  size_t GetOrigDataQty() const { return origData_.size(); }
  const Space<dist_t>*  GetSpace() const { return space_; }
  const ObjectVector& GetDataObjects() const { return dataobjects_; }
  const ObjectVector& GetQueryObjects() const { return queryobjects_; }
  const typename std::vector<unsigned>& GetKNN() const { return knn_; }
  float GetEPS() const { return eps_; }
  const typename std::vector<dist_t>& GetRange() const { return range_; }
  int   GetDimension() const { return dimension_; }
  int   GetQueryToRunQty() const {
    return noQueryFile_ ? maxNumQueryToRun_ :
                          static_cast<unsigned>(origQuery_.size());
  }
  int   GetTotalQueryQty() const {
    return noQueryFile_ ? maxNumQuery_:
                          static_cast<unsigned>(origQuery_.size());
  }

  /*
   * Read/Write : save/retrieve some of the config information.
   */
  void Write(ostream& controlStream, ostream& binaryStream);
  void Read(istream& controlStream, istream& binaryStream, size_t& cacheDataSetQty);

private:
  Space<dist_t>*    space_;
  ObjectVector      dataobjects_;
  ObjectVector      queryobjects_;
  ObjectVector      origData_;
  ObjectVector      origQuery_;
  vector<int>       origDataAssignment_;  // >=0 denotes an index of the test set, -1 denotes data points
  unordered_map<size_t, size_t> cachedDataAssignment_;
  string            datafile_;
  string            queryfile_;
  bool              noQueryFile_;
  unsigned          testSetToRunQty_;
  unsigned          testSetQty_;
  unsigned          maxNumData_;
  unsigned          maxNumQuery_;
  unsigned          maxNumQueryToRun_;
  unsigned          dimension_;

  vector<dist_t>    range_;  // range search parameter
  vector<unsigned>  knn_;    // knn search parameters
  float             eps_;    // knn search parameter

};

}


#endif
