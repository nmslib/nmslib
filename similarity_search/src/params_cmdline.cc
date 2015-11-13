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
#include <cstring>
#include <map>
#include <limits>
#include <iostream>
#include <stdexcept>

#include "utils.h"
#include "params_cmdline.h"
#include "params_def.h"
#include "logging.h"
#include "space.h"

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

void ParseCommandLine(int argc, char*argv[], bool& bPrintProgress,
                      string&                 LogFile,
                      string&                 LoadIndexLoc,
                      string&                 SaveIndexLoc,
                      string&                 DistType,
                      string&                 SpaceType,
                      shared_ptr<AnyParams>&  SpaceParams,
                      unsigned&               ThreadTestQty,
                      bool&                   AppendToResFile,
                      string&                 ResFilePrefix,
                      unsigned&               TestSetQty,
                      string&                 DataFile,
                      string&                 QueryFile,
                      string&                 CacheGSFilePrefix,
                      size_t&                 maxCacheGSQty,
                      unsigned&               MaxNumData,
                      unsigned&               MaxNumQuery,
                      vector<unsigned>&       knn,
                      float&                  eps,
                      string&                 RangeArg,
                      string&                 MethodName,
                      shared_ptr<AnyParams>&          IndexTimeParams,
                      vector<shared_ptr<AnyParams>>&  QueryTimeParams) {
  knn.clear();
  RangeArg.clear();
  QueryTimeParams.clear();

  string          indexTimeParamStr;
  vector<string>  vQueryTimeParamStr;
  string          spaceParamStr;
  string          knnArg;
  // Conversion to double is due to an Intel's bug with __builtin_signbit being undefined for float
  double          epsTmp;

  bPrintProgress  = true;

  bool bSuppressPrintProgress;

  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    (HELP_PARAM_OPT,          HELP_PARAM_MSG)
    (SPACE_TYPE_PARAM_OPT,    po::value<string>(&spaceParamStr)->required(),                SPACE_TYPE_PARAM_MSG)
    (DIST_TYPE_PARAM_OPT,     po::value<string>(&DistType)->default_value(DIST_TYPE_FLOAT), DIST_TYPE_PARAM_MSG)
    (DATA_FILE_PARAM_OPT,     po::value<string>(&DataFile)->required(),                     DATA_FILE_PARAM_MSG)
    (MAX_NUM_DATA_PARAM_OPT,  po::value<unsigned>(&MaxNumData)->default_value(MAX_NUM_DATA_PARAM_DEFAULT), MAX_NUM_DATA_PARAM_MSG)
    (QUERY_FILE_PARAM_OPT,    po::value<string>(&QueryFile)->default_value(QUERY_FILE_PARAM_DEFAULT),  QUERY_FILE_PARAM_MSG)
    (LOAD_INDEX_PARAM_OPT,    po::value<string>(&LoadIndexLoc)->default_value(LOAD_INDEX_PARAM_DEFAULT),   LOAD_INDEX_PARAM_MSG)
    (SAVE_INDEX_PARAM_OPT,    po::value<string>(&SaveIndexLoc)->default_value(SAVE_INDEX_PARAM_DEFAULT),   SAVE_INDEX_PARAM_MSG)
    (CACHE_PREFIX_GS_PARAM_OPT,  po::value<string>(&CacheGSFilePrefix)->default_value(CACHE_PREFIX_GS_PARAM_DEFAULT),  CACHE_PREFIX_GS_PARAM_MSG)
    (MAX_CACHE_GS_QTY_PARAM_OPT, po::value<size_t>(&maxCacheGSQty)->default_value(MAX_CACHE_GS_QTY_PARAM_DEFAULT),    MAX_CACHE_GS_QTY_PARAM_MSG)
    (LOG_FILE_PARAM_OPT,      po::value<string>(&LogFile)->default_value(LOG_FILE_PARAM_DEFAULT),          LOG_FILE_PARAM_MSG)
    (MAX_NUM_QUERY_PARAM_OPT, po::value<unsigned>(&MaxNumQuery)->default_value(MAX_NUM_QUERY_PARAM_DEFAULT), MAX_NUM_QUERY_PARAM_MSG)
    (TEST_SET_QTY_PARAM_OPT,  po::value<unsigned>(&TestSetQty)->default_value(TEST_SET_QTY_PARAM_DEFAULT),   TEST_SET_QTY_PARAM_MSG)
    (KNN_PARAM_OPT,           po::value< string>(&knnArg),                                  KNN_PARAM_MSG)
    (RANGE_PARAM_OPT,         po::value<string>(&RangeArg),                                 RANGE_PARAM_MSG)
    (EPS_PARAM_OPT,           po::value<double>(&epsTmp)->default_value(EPS_PARAM_DEFAULT), EPS_PARAM_MSG)
    (QUERY_TIME_PARAMS_PARAM_OPT, po::value< vector<string> >(&vQueryTimeParamStr)->multitoken()->zero_tokens(), QUERY_TIME_PARAMS_PARAM_MSG)
    (INDEX_TIME_PARAMS_PARAM_OPT, po::value<string>(&indexTimeParamStr)->default_value(""), INDEX_TIME_PARAMS_PARAM_MSG)
    (METHOD_PARAM_OPT,        po::value<string>(&MethodName)->default_value(METHOD_PARAM_DEFAULT), METHOD_PARAM_MSG)
    (THREAD_TEST_QTY_PARAM_OPT,po::value<unsigned>(&ThreadTestQty)->default_value(THREAD_TEST_QTY_PARAM_DEFAULT), THREAD_TEST_QTY_PARAM_MSG)
    (OUT_FILE_PREFIX_PARAM_OPT, po::value<string>(&ResFilePrefix)->default_value(OUT_FILE_PREFIX_PARAM_DEFAULT), OUT_FILE_PREFIX_PARAM_MSG)
    (APPEND_TO_RES_FILE_PARAM_OPT, po::bool_switch(&AppendToResFile), APPEND_TO_RES_FILE_PARAM_MSG)
    (NO_PROGRESS_PARAM_OPT,   po::bool_switch(&bSuppressPrintProgress), NO_PROGRESS_PARAM_MSG)
    ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << e.what();
  }

  bPrintProgress = !bSuppressPrintProgress;

  eps = epsTmp;

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

  ToLower(DistType);
  ToLower(spaceParamStr);
  ToLower(MethodName);
  
  try {
    {
      vector<string>     desc;
      ParseSpaceArg(spaceParamStr, SpaceType, desc);
      SpaceParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }


    {
      vector<string>     desc;
      ParseArg(indexTimeParamStr, desc);
      IndexTimeParams = shared_ptr<AnyParams>(new AnyParams(desc));
    }

    if (!indexTimeParamStr.empty() && !LoadIndexLoc.empty()) {
      Usage(argv[0], ProgOptDesc);
      LOG(LIB_FATAL) << "You cannot specify both the indexing parameters and the location to load the index!";
    }

    if (vQueryTimeParamStr.empty())
      vQueryTimeParamStr.push_back("");

    for(const auto s: vQueryTimeParamStr) {
      vector<string>  desc;

      ParseArg(s, desc);
      QueryTimeParams.push_back(shared_ptr<AnyParams>(new AnyParams(desc)));
    }

    if (vm.count("knn")) {
      if (!SplitStr(knnArg, knn, ',')) {
        Usage(argv[0], ProgOptDesc);
        LOG(LIB_FATAL) << "Wrong format of the KNN argument: '" << knnArg;
      }
    }

    if (DataFile.empty()) {
      LOG(LIB_FATAL) << "data file is not specified!";
    }

    if (!DoesFileExist(DataFile)) {
      LOG(LIB_FATAL) << "data file " << DataFile << " doesn't exist";
    }

    if (!QueryFile.empty() && !DoesFileExist(QueryFile)) {
      LOG(LIB_FATAL) << "query file " << QueryFile << " doesn't exist";
    }

    if (!MaxNumQuery && QueryFile.empty()) {
      LOG(LIB_FATAL) << "Set a positive # of queries or specify a query file!"; 
    }
  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }
}

};


