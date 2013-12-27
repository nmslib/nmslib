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
#include <iostream>

#include "utils.h"
#include "params.h"
#include "logging.h"

#include <cmath>
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

void ParseCommandLine(int argc, char*argv[],
                      string&                 DistType,
                      string&                 SpaceType,
                      shared_ptr<AnyParams>&  SpaceParams,
                      unsigned&               dimension,
                      unsigned&               ThreadTestQty,
                      bool&                   AppendToResFile,
                      string&                 ResFilePrefix,
                      unsigned&               TestSetQty,
                      string&                 DataFile,
                      string&                 QueryFile,
                      unsigned&               MaxNumData,
                      unsigned&               MaxNumQuery,
                      vector<unsigned>&       knn,
                      float&                  eps,
                      string&                 RangeArg,
                      multimap<string, shared_ptr<AnyParams>>& pars) {
  knn.clear();
  RangeArg.clear();
  pars.clear();

  vector<string>  methParams;
  string          knnArg;
  // Conversion to double is due to an Intel's bug with __builtin_signbit being undefined for float
  double          epsTmp;

  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("spaceType,s",     po::value<string>(&SpaceType)->required(),
                        "space type, e.g., l1, l2, lp:p=0.5")
    ("distType",        po::value<string>(&DistType)->default_value("float"),
                        "distance value type: int, float, double")
    ("dataFile,i",      po::value<string>(&DataFile)->required(),
                        "input data file")
    ("maxNumData",      po::value<unsigned>(&MaxNumData)->default_value(0),
                        "if non-zero, only the first maxNumData elements are used")
    ("dimension,d",     po::value<unsigned>(&dimension)->default_value(0),
                        "optional dimensionality")
    ("queryFile,q",     po::value<string>(&QueryFile)->default_value(""),
                        "query file")
    ("maxNumQuery",     po::value<unsigned>(&MaxNumQuery)->default_value(1000),
                        "if non-zero, use maxNumQuery query elements"
                        "(required in the case of bootstrapping)")
    ("testSetQty,b",    po::value<unsigned>(&TestSetQty)->default_value(0),
                        "# of test sets obtained by bootstrapping;"
                        " ignored if queryFile is specified")
    ("knn,k",           po::value< string>(&knnArg),
                        "comma-separated Ks for the k-NN searches")
    ("range,r",         po::value<string>(&RangeArg),
                        "comma-separated values for the range searches")
    ("eps",             po::value<double>(&epsTmp)->default_value(0.0),
                        "the parameter for the eps-approximate k-NN search.")
    ("method,m",        po::value< vector<string> >(&methParams)->required(),
                        "list of method(s) with comma-separated parameters in the format:\n"
                        "<method name>:<param1>,<param2>,...,<paramK>")
    ("threadTestQty",   po::value<unsigned>(&ThreadTestQty)->default_value(1),
                        "# of threads")
    ("outFilePrefix,o", po::value<string>(&ResFilePrefix)->default_value(""),
                        "output file prefix")
    ("appendToResFile", po::value<bool>(&AppendToResFile)->default_value(false),
                        "do not override information in results files, append new data")

    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(FATAL) << e.what();
  }

  eps = epsTmp;

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

  ToLower(DistType);
  ToLower(SpaceType);
  
  {
    vector<string> tmp;
    if (!SplitStr(SpaceType, tmp, ':') || tmp.size() > 2  || !tmp.size()) {
      Usage(argv[0], ProgOptDesc);
      LOG(FATAL) << "Wrong format of the space argument: '" << SpaceType;
    }
    SpaceType = tmp[0];

    vector<string> SpaceDesc;
    if (tmp.size() == 2) {
      if (!SplitStr(tmp[1], SpaceDesc, ',')) {
        LOG(FATAL) << "Cannot split space arguments in: " << tmp[1];
      }
    }
    SpaceParams = shared_ptr<AnyParams>(new AnyParams(SpaceDesc));
  }

  for(const auto s: methParams) {
    vector<string> tmp;
    if (!SplitStr(s, tmp, ':') || tmp.size() > 2  || !tmp.size()) {
      Usage(argv[0], ProgOptDesc);
      LOG(FATAL) << "Wrong format of the method argument: '" << s;
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

    pars.insert(make_pair(MethName, shared_ptr<AnyParams>(new AnyParams(MethodDesc))));
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

  if (!MaxNumQuery && QueryFile.empty()) {
    LOG(FATAL) << "Set a positive # of queries or specify a query file!"; 
  }
}

};


