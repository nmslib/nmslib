/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <cstring>
#include <map>
#include <limits>

#include "utils.h"
#include "params.h"
#include "logging.h"

#include <boost/program_options.hpp>

using namespace std;

namespace similarity {

using std::multimap;
using std::string;

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

/*
void CollectMethodParams(const vector<string>& MethodDesc, MethodParams& par) {
  for (unsigned i = 0; i < MethodDesc.size(); ++i) {
    vector<string>  OneParamPair;
    if (!SplitStr(MethodDesc[i], OneParamPair, '=') ||
        OneParamPair.size() != 2) {
      LOG(FATAL) << "Wrong format of the method argument: '" << MethodDesc[i] << "' should be in the format: <Name>=<Value>";
    }
    const string& Name = OneParamPair[0];
    const string& sVal = OneParamPair[1];
    const char*   pVal = OneParamPair[1].c_str();
    const int     iVal = atoi(pVal);
    const float   fVal = atof(pVal);

    if (Name == "indexQty") {
      par.IndexQty = iVal;
    } else if (Name == "bucketSize") {
      par.BucketSize = iVal;
    } else if (Name == "useBucketSize") {
      par.UseBucketSize = iVal;
    } else if (Name == "radius") {
      par.Radius = fVal;
    } else if (Name == "doRandSample") {
      par.DoRandSample = iVal;
    } else if (Name == "numPivot") {
      par.NumPivot = iVal;
    } else if (Name == "prefixLength") {
      par.PrefixLength = iVal;
    } else if (Name == "maxPathLength") {
      par.MaxPathLength = iVal;
    } else if (Name == "minCandidate") {
      par.MinCandidate = iVal;
    } else if (Name == "maxK") {
      par.MaxK = iVal;
    } else if (Name == "dbScanFrac") {
      par.DbScanFrac = fVal;
    } else if (Name == "distLearnThresh") {
      par.DistLearnThreshold = fVal;
    } else if (Name == "alphaLeft") {
      par.AlphaLeft = fVal;
    } else if (Name == "alphaRight") {
      par.AlphaRight = fVal;
    } else if (Name == "saveHistFileName") {
      par.SaveHistFileName = sVal;
    } else if (Name == "quantileStepPivot") {
      par.QuantileStepPivot = fVal;
    } else if (Name == "quantileStepPseudoQuery") {
      par.QuantileStepPseudoQuery = fVal;
    } else if (Name == "numOfPseudoQueriesInQuantile") {
      par.NumOfPseudoQueriesInQuantile = fVal;
    } else if (Name == "M") {
      par.LSH_M = iVal;
    } else if (Name == "L") {
      par.LSH_L = iVal;
    } else if (Name == "H") {
      par.LSH_H = iVal;
    } else if (Name == "T") {
      par.LSH_T = iVal;
    } else if (Name == "tuneK") {
      par.LSH_TuneK = iVal;
    } else if (Name == "chunkBucket") {
      par.ChunkBucket = iVal;
    } else if (Name == "maxLeavesToVisit") {
      par.MaxLeavesToVisit = iVal;
    } else if (Name == "W") {
      par.LSH_W = fVal;
    } else if (Name == "desiredRecall") {
      par.DesiredRecall = fVal;
    } else if (Name == "strategy") {
      if (sVal == "random") {
        par.LCStrategy =  ListClustersStrategy::kRandom;
      } else if (sVal == "closestPrevCenter") {
        par.LCStrategy = ListClustersStrategy::kClosestPrevCenter;
      } else if (sVal == "farthestPrevCenter") {
        par.LCStrategy = ListClustersStrategy::kFarthestPrevCenter;
      } else if (sVal == "minSumDistPrevCenters") {
        par.LCStrategy = ListClustersStrategy::kMinSumDistPrevCenters;
      } else if (sVal == "maxSumDistPrevCenters") {
        par.LCStrategy = ListClustersStrategy::kMaxSumDistPrevCenters;
      } else {
        LOG(FATAL) << "Incorrect value for parameter : " << Name;
      }
    } else {
      LOG(FATAL) << "Incorrect parameter name: " << Name;
    }
  }
};
*/

void ParseCommandLine(int argc, char*argv[],
                      string& DistType,
                      string& SpaceType,
                      unsigned& dimension,
                      unsigned& ThreadTestQty,
                      bool& AppendToResFile,
                      string& ResFilePrefix,
                      unsigned&  TestSetQty,
                      string& DataFile,
                      string& QueryFile,
                      unsigned& MaxNumData,
                      unsigned& MaxNumQuery,
                      vector<unsigned>& knn,
                      float& eps,
                      string& RangeArg,
                      multimap<string, AnyParams*>& pars) {
  knn.clear();
  RangeArg.clear();
  pars.clear();

  vector<string>  methParams;
  string          knnArg;

  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("dataFile,i",      po::value<string>(&DataFile)->required(),
                        "input data file")
    ("method,m",        po::value< vector<string> >(&methParams)->required(),
                        "list of method(s) with comma-separated parameters in the format:\n"
                        "<method name>:<param1>,<param2>,...,<paramK>")
    ("knn,k",           po::value< string>(&knnArg),
                        "comma-separated Ks for the k-NN searches")
    ("range,r",         po::value<string>(&RangeArg),
                        "comma-separated values for the range searches")
    ("spaceType,s",     po::value<string>(&SpaceType)->required(),
                        "space type, e.g., l1, l2")
    ("dimension,d",     po::value<unsigned>(&dimension)->default_value(0),
                        "dimensionality (required for vector spaces)")
    ("distType",        po::value<string>(&DistType)->default_value("float"),
                        "distance type: int, float, double")
    ("outFilePrefix,o", po::value<string>(&ResFilePrefix)->default_value(""),
                        "output file prefix")
    ("queryFile,q",     po::value<string>(&QueryFile)->default_value(""),
                        "query file")
    ("testSetQty,b",    po::value<unsigned>(&TestSetQty)->default_value(0),
                        "# of test sets obtained by bootstrapping;"
                        " ignored if queryFile is specified")
    ("maxNumData",      po::value<unsigned>(&MaxNumData)->default_value(0),
                        "if non-zero, only the first maxNumData elements are used")
    ("maxNumQuery",     po::value<unsigned>(&MaxNumQuery)->default_value(0),
                        "if non-zero, use maxNumQuery query elements"
                        "(required in the case of bootstrapping)")
    ("threadTestQty",   po::value<unsigned>(&ThreadTestQty)->default_value(1),
                        "# of threads")
    ("appendToResFile", po::value<bool>(&AppendToResFile)->default_value(false),
                        "do not override information in results files, append new data")
    ("eps",             po::value<float>(&eps)->default_value(0.0),
                        "the parameter for the eps-approximate k-NN search.")

    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(FATAL) << e.what();
  }

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

  ToLower(DistType);
  ToLower(SpaceType);

  for(const auto s: methParams) {
    vector<string> tmp;
    if (!SplitStr(s, tmp, ':') || tmp.size() > 2  || !tmp.size()) {
      Usage(argv[0], ProgOptDesc);
      LOG(FATAL) << "Wrong format of the method argument: '" << ProgOptDesc;
    }
    string         MethName = tmp[0];

    vector<string> MethodDesc;
    if (tmp.size() == 2) {
      if (!SplitStr(tmp[1], MethodDesc, ',')) {
        LOG(FATAL) << "Cannot split method arguments in: " << tmp[1];
      }
    }

    if (pars.count(MethName)) {
      LOG(FATAL) << "Duplicate method name: " << MethName << endl;
    }

    pars.insert(make_pair(MethName, new AnyParams(MethodDesc)));
  }
  if (vm.count("knn")) {
    if (!SplitStr(knnArg, knn, ',')) {
      Usage(argv[0], ProgOptDesc);
      LOG(FATAL) << "Wrong format of the KNN argument: '" << knnArg;
    }
  }

  LOG(INFO) << "program arguments processed.";

  if (DataFile.empty()) {
    LOG(FATAL) << "data file is not specified!";
  }

  if (!IsFileExists(DataFile)) {
    LOG(FATAL) << "data file " << DataFile << " doesn't exist";
  }

  if (!QueryFile.empty() && !IsFileExists(QueryFile)) {
    LOG(FATAL) << "query file " << QueryFile << " doesn't exist";
  }
}

};


