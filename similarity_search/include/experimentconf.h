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
#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"

namespace similarity {

using std::string;

template <typename dist_t>
class ExperimentConfig {
public:
  ExperimentConfig(Space<dist_t>* space,
                   const string& datafile,
                   const string& queryfile,
                   unsigned TestSetQty, // The # of times the datafile is randomly divided into the query and the test set
                   unsigned MaxNumData,
                   unsigned MaxNumQuery,
                   unsigned dimension,
                   const typename std::vector<unsigned>& knn,
                   float eps,
                   const typename std::vector<dist_t>& range)
      : space(reinterpret_cast<Space<dist_t>*>(space)),
        datafile(datafile),
        queryfile(queryfile),
        NoQueryFile(queryfile.empty()),
        TestSetQty(TestSetQty),
        MaxNumData(MaxNumData),
        MaxNumQuery(MaxNumQuery),
        dimension(dimension),
        knn(knn),
        eps(eps),
        range(range)
{
    if (NoQueryFile) {
      if (!TestSetQty) {
        LOG(FATAL) << "Bad configuration. One should either specify a query file, " <<
                      " or the number of test sets obtained by bootstrapping (random division into query and data files).";
      }
    }
  }

  ~ExperimentConfig() {
    delete space;
    for (auto it = OrigData.begin(); it != OrigData.end(); ++it) {
      delete *it;
    }
    for (auto it = OrigQuery.begin(); it != OrigQuery.end(); ++it) {
      delete *it;
    }
  }

  void ReadDataset();
  void PrintInfo() const;
  void SelectTestSet(int SetNum);
  int GetTestSetQty() const {
    if (!NoQueryFile) return 1;
    return TestSetQty;
  }
  const Space<dist_t>*  GetSpace() const { return space; }
  const ObjectVector& GetDataObjects() const { return dataobjects; }
  const ObjectVector& GetQueryObjects() const { return queryobjects; }
  const typename std::vector<unsigned>& GetKNN() const { return knn; }
  float GetEPS() const { return eps; }
  const typename std::vector<dist_t>& GetRange() const { return range; }
  int   GetDimension() const { return dimension; }
  int   GetQueryQty() const { return NoQueryFile ? MaxNumQuery : static_cast<unsigned>(OrigQuery.size()); }
private:
  Space<dist_t>*    space;
  ObjectVector      dataobjects;
  ObjectVector      queryobjects;
  ObjectVector      OrigData;
  ObjectVector      OrigQuery;
  std::vector<int>  OrigDataAssignment;  // >=0 denotes an index of the test set, -1 denotes data points
  string      datafile;
  string      queryfile;
  bool        NoQueryFile;
  unsigned TestSetQty;
  unsigned MaxNumData;
  unsigned MaxNumQuery;
  unsigned dimension;
  dist_t mindistance;
  dist_t maxdistance;

  const typename std::vector<unsigned>& knn;       // knn search
  float eps;
  const typename std::vector<dist_t>& range;  // range search
};

}


#endif
