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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <stdexcept>

#include "space.h"
#include "logging.h"
#include "experimentconf.h"



#define DATA_FILE         "DataFile"
#define DATA_FILE_QTY     "DataFileQty"

// The query file can be empty
#define QUERY_FILE        "QueryFile"
#define TEST_SET_QTY      "TestSetQty"

#define RANGE_QTY         "RangeQty"
#define KNN_QTY           "KNNQty"
// This is either
#define QUERY_QTY         "QueryQty"


namespace similarity {

using namespace std;

template <typename dist_t>
void ExperimentConfig<dist_t>::Read(istream& controlStream,
                                    istream& binaryStream,
                                    size_t& dataFileQty) {
  string s;
  size_t i;

  ReadField(controlStream, DATA_FILE, s);

  if (s != datafile_) {
    stringstream err;
    err << "The specified data file ('" << datafile_ << "' "
        << " doesn't match the data file ('"
        << s << ") in the gold standard cache (must be char-by-char equal).";
    throw runtime_error(err.str());
  }

  ReadField(controlStream, DATA_FILE_QTY, s);
  ConvertFromString(s, dataFileQty);

  ReadField(controlStream, QUERY_FILE, s);

  if (s != queryfile_) {
    stringstream err;
    err << "The specified query file ('" << queryfile_ << "' "
        << " doesn't match the query file ('"
        << s << ") in the gold standard cache (must be char-by-char equal).";
    throw runtime_error(err.str());
  }

  ReadField(controlStream, TEST_SET_QTY, s);
  ConvertFromString(s, i);

  if (i != testSetQty_) {
    stringstream err;
    err << "The specified # of test sets (" << testSetQty_ << ") "
        << " doesn't match the value (" << i << ") "
        << " in the gold standard cache.";
    throw runtime_error(err.str());
  }

  ReadField(controlStream, RANGE_QTY, s);
  ConvertFromString(s, i);

  if (i != range_.size()) {
    stringstream err;
    err << "The specified # of range searches (" << range_.size() << ") "
        << " doesn't match the value (" << i << ") "
        << " in the gold standard cache.";
    throw runtime_error(err.str());
  }

  ReadField(controlStream, KNN_QTY, s);
  ConvertFromString(s, i);

  if (i != knn_.size()) {
    stringstream err;
    err << "The specified # of KNN searches (" << knn_.size() << ") "
        << " doesn't match the value (" << i << ") "
        << " in the gold standard cache.";
    throw runtime_error(err.str());
  }

  dist_t val;
  for (size_t i = 0; i < range_.size(); ++i) {
    binaryStream.read(reinterpret_cast<char*>(&val), sizeof val);
    if (!ApproxEqual<dist_t>(range_[i], val)) {
      stringstream err;
      err << "The specified range value #" << (i+1) << " (" << range_[i] << ") "
          << " isn't equal to the value (" << val << ") "
          << " in the gold standard cache.";
      throw runtime_error(err.str());
    }
  }

  // Note that the type of eps_ is not necessarily dist_t!!!
  decltype(eps_) epsVal;

  binaryStream.read(reinterpret_cast<char*>(&epsVal), sizeof epsVal);
  if (!ApproxEqual<dist_t>(eps_, epsVal)) {
    stringstream err;
    err << "The specified eps (" << eps_ << ") "
        << " isn't equal to the value (" << epsVal << ") "
        << " in the gold standard cache.";
    throw runtime_error(err.str());
  }

  unsigned kVal;

  if (sizeof(kVal)!=sizeof(knn_[0])) {
    stringstream err;
    err << "Bug: types apparently changed, sizeof(kVal) = " << sizeof(kVal) <<
           " which is different from sizeof(knn_[0]) = " << sizeof(knn_[0]);
    throw runtime_error(err.str());
  }

  for (size_t i = 0; i < knn_.size(); ++i) {
    binaryStream.read(reinterpret_cast<char*>(&kVal), sizeof kVal);
    // We can use a smaller KNN value, but not the other way around!
    if (kVal < knn_[i]) {
      stringstream err;
      err << "The specified KNN value #" << (i+1) << " (" << knn_[i] << ") "
          << " isn't >= than the value (" << kVal << ") "
          << " in the gold standard cache.";
      throw runtime_error(err.str());
    }
  }

  ReadField(controlStream, QUERY_QTY, s);
  ConvertFromString(s, maxNumQuery_);

  /*
   * The number of queries specified by the user can be smaller than
   * the number of GS entries in the file, but not the other
   * way around.
   */

  if (maxNumQuery_ < maxNumQueryToRun_) {
    stringstream err;
    err << "The specified # queries (" << maxNumQueryToRun_ << ") "
        << " exceeds the value (" << maxNumQuery_ << ") "
        << " in the gold standard cache.";
    throw runtime_error(err.str());
  }

  if (noQueryFile_) {
    for (size_t TestSetId = 0; TestSetId < testSetQty_; ++TestSetId) {
      vector<int> vTmp;
      if (!getline(controlStream, s)) {
        throw runtime_error("Error reading from the control/text cache file!");
      }

      SplitStr(s, vTmp, ' ');
      for (int id : vTmp) {
        cachedDataAssignment_.insert(make_pair(id, TestSetId));
      }
    }
  }
}

template <typename dist_t>
void ExperimentConfig<dist_t>::Write(ostream& controlStream, ostream& binaryStream) {
  WriteField(controlStream, DATA_FILE, datafile_);
  WriteField(controlStream, DATA_FILE_QTY, ConvertToString(origData_.size()));
  WriteField(controlStream, QUERY_FILE, queryfile_);
  WriteField(controlStream, TEST_SET_QTY, ConvertToString(testSetQty_));
  WriteField(controlStream, RANGE_QTY, ConvertToString(range_.size()));
  WriteField(controlStream, KNN_QTY, ConvertToString(knn_.size()));
  // Let's write range and knn-query parameters in the binary format
  for (size_t i = 0; i < range_.size(); ++i)
    binaryStream.write(reinterpret_cast<const char*>(&range_[i]), sizeof range_[i]);

  binaryStream.write(reinterpret_cast<const char*>(&eps_), sizeof eps_);

  for (size_t i = 0; i < knn_.size(); ++i) {
    binaryStream.write(reinterpret_cast<const char*>(&knn_[i]), sizeof knn_[i]);
  }
  unsigned queryQty = origQuery_.size();

  if (noQueryFile_) {
    if (!testSetQty_) {
      throw runtime_error("Bug: zero number of test sets!");
    }
    /*
     *  Let's obtain the number of queries + double-check
     *  that each subset has the same number of queries
     */
    vector<size_t> qtys(testSetQty_);
    for (size_t i = 0; i < origDataAssignment_.size(); ++i) {
      int dst = origDataAssignment_[i];
      if (dst >= 0) {
        if (dst >= testSetQty_) {
          stringstream err;
          err << "Bug: an assignment id (" << dst <<
              ") is > # of sets (" << testSetQty_ << ")";
          throw runtime_error(err.str());
        }
        qtys[dst]++;
      }
    }
    queryQty = qtys[0]; // see the check above queryQty should be > 0
    if (!queryQty) {
      throw runtime_error("Bug: zero number of queries!");
    }
    for (size_t i = 1; i < testSetQty_; ++i)
    if (qtys[i] != queryQty) {
      stringstream err;
      err << "Bug, different # of queries in the subsets, " <<
             "id=0, qty=" << queryQty <<
             "id=" << i <<", qty=" << qtys[i];
      throw runtime_error(err.str());
    }

  }

  WriteField(controlStream, QUERY_QTY, ConvertToString(queryQty));

  if (noQueryFile_) {
  // Let's save test set assignments
    size_t OrigQty = origData_.size();
    for (size_t SetNum = 0; SetNum < testSetQty_; ++SetNum) {
      stringstream  line;
      bool bFirst = true;
      for (size_t i = 0; i < OrigQty; ++i) {
        int dst = origDataAssignment_[i];

        if (dst == SetNum) {
          if (!bFirst) line << " ";
          bFirst = false;
          line << i;
        }
      }
      controlStream << line.str() << endl;
    }
  }

}

template <typename dist_t>
void ExperimentConfig<dist_t>::SelectTestSet(int SetNum) {
  if (!noQueryFile_) return;
  if (SetNum <0 || static_cast<unsigned>(SetNum) >= testSetQty_) {
    stringstream err;
    err << "Invalid test set #: " << SetNum;
    throw runtime_error(err.str());
  }
  dataobjects_.clear();
  queryobjects_.clear();

  size_t OrigQty = origData_.size();
  for (size_t i = 0; i < OrigQty; ++i) {
    int dst = origDataAssignment_[i];

    /*
     * dst == -1  =>  the data point is always put to dataobjects
     * if dst >= 0 and SetNum == dst, add this
     * data point to the current query set (with the ID=SetNum).
     * Otherwise, it goes to the data set.
     */
    if (dst == SetNum) {
      /*
       * There can be more queries than we need to run.
       * One typical scenario: the user saves cache for 1000 queries,
       * next time she runs a test using the saved cache she asks
       * to evaluate using only 100 queries.
       */
      if (queryobjects_.size() < maxNumQueryToRun_)
        queryobjects_.push_back(origData_[i]);
    }
    else dataobjects_.push_back(origData_[i]);
  }
}

template <typename dist_t>
void ExperimentConfig<dist_t>::ReadDataset() {
  if (space_ == NULL) throw runtime_error("Space pointer should not be NULL!");
  if (!dataobjects_.empty())
    throw runtime_error(
        "The set of data objects in non-empty, did you read the data set already?");

  if (!queryobjects_.empty())
    throw runtime_error(
        "The set of query objects in non-empty, did you read the data set already?");

  space_->ReadDataset(origData_, this, datafile_.c_str(), maxNumData_);

  /*
   * Note!!! 
   * The destructor of this class will delete pointers stored in OrigData & OrigQuery.
   * Applications should not delete objects whose pointers are stored in arrays:
   * 1) dataobjects 
   * 2) queryobjects.
   */
  if (!noQueryFile_) {
    dataobjects_ = origData_;
    space_->ReadDataset(queryobjects_, this, queryfile_.c_str(), maxNumQuery_);
    origQuery_ = queryobjects_;
  } else {
    size_t OrigQty = origData_.size();
    size_t MinOrigQty = (testSetQty_ + 1) * maxNumQuery_;
    if (OrigQty < MinOrigQty) {
      stringstream err;
      err << "The data set is too small, expecting at least: " << MinOrigQty << " data points. " <<
                    "Try to either increase the number of data points, or to decrease parameters: " <<
                    "testSetQty and/or maxNumQuery ";
      throw runtime_error(err.str());
    }
    origDataAssignment_.resize(OrigQty);
    fill(origDataAssignment_.begin(), origDataAssignment_.end(), -1);

    if (!cachedDataAssignment_.empty()) {
      for (auto elem: cachedDataAssignment_)
        origDataAssignment_[elem.first] = elem.second;
    } else {
      /*
       * Test queries are selected randomly without resampling.
       * The algorithm of efficient sampling without replacement
       * was based on conclusions of D. Lemire's:
       * https://github.com/lemire/Code-used-on-Daniel-Lemire-s-blog/blob/master/2013/08/14/cpp/synthetic.h
       */

      for (unsigned i = 0; i < testSetQty_; ++i) {
        size_t        card = 0;

        while (card < maxNumQuery_) {
          size_t Id = RandomInt() % OrigQty;

          if (-1 == origDataAssignment_[Id]) {
            origDataAssignment_[Id] = i;
            ++card;
          }
        }
      }
    }
  }

  LOG(LIB_INFO) << "data & query .... ok!\n";
}

template <typename dist_t>
void ExperimentConfig<dist_t>::PrintInfo() const {
  space_->PrintInfo();
  LOG(LIB_INFO) << "distance type         = " << DistTypeName<dist_t>();
  LOG(LIB_INFO) << "data file             = " << datafile_;
  LOG(LIB_INFO) << "# of test sets        = " << GetTestSetQty();
  LOG(LIB_INFO) << "Use held-out queries  = " << !noQueryFile_;
  LOG(LIB_INFO) << "# of data points      = " << origData_.size() - GetQueryToRunQty();
  LOG(LIB_INFO) << "# of query points     = " << GetQueryToRunQty();
}

template class ExperimentConfig<float>;
template class ExperimentConfig<double>;
template class ExperimentConfig<int>;

}

