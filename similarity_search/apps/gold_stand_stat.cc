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
#include <string>

#include "init.h"
#include "spacefactory.h"
#include "space.h"
#include "logging.h"
#include "gold_standard.h"
#include "experimentconf.h"
#include "cmd_options.h"
#include "params_def.h"

using namespace similarity;
using namespace std;

template <class dist_t>
void getCacheStat(
  string cacheGSFilePrefix,
  string outFile,
  string spaceType,
  string dataFile, string queryFile, unsigned testSetQty,
  unsigned maxNumData, unsigned maxNumQuery,
  vector<unsigned>      knn
  ) {
  // possible TODO we don't use eps here
  float eps = 0;
  // possible TODO we don't do range search here
  vector<dist_t>   range;

  const string& cacheGSControlName = cacheGSFilePrefix + "_ctrl.txt";
  const string& cacheGSBinaryName  = cacheGSFilePrefix + "_data.bin";

  unique_ptr<fstream>      cacheGSControl;
  unique_ptr<fstream>      cacheGSBinary;

  ToLower(spaceType);
  vector<string> spaceDesc;

  const string descStr = spaceType;
  ParseSpaceArg(descStr, spaceType, spaceDesc);
  unique_ptr<AnyParams> spaceParams = unique_ptr<AnyParams>(new AnyParams(spaceDesc));

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(spaceType, *spaceParams));

  ExperimentConfig<dist_t> config(*space,
                                  dataFile, queryFile, testSetQty,
                                  maxNumData, maxNumQuery,
                                  knn, eps, range);

  cacheGSControl.reset(new fstream(cacheGSControlName.c_str(),
                                   std::ios::in));
  cacheGSBinary.reset(new fstream(cacheGSBinaryName.c_str(),
                                  std::ios::in
                                  // On Windows you don't get a proper binary stream without ios::binary!
                                  | ios::binary));

  cacheGSControl->exceptions(std::ios::badbit);
  cacheGSBinary->exceptions(std::ios::badbit);

  size_t cacheDataSetQty = 0;

  config.Read(*cacheGSControl, *cacheGSBinary, cacheDataSetQty);

  GoldStandardManager<dist_t> managerGS(config);

  LOG(LIB_INFO) << "Read the config file!";

  for (int testSetId = 0; testSetId < config.GetTestSetToRunQty(); ++testSetId) {
    config.SelectTestSet(testSetId);

    size_t cacheTestId = 0, savedThreadQty = 0;
    managerGS.Read(*cacheGSControl, *cacheGSBinary,
                   config.GetTotalQueryQty(),
                   cacheTestId, savedThreadQty);
    if (cacheTestId != testSetId) {
      stringstream err;
      err << "Perhaps, the input file is corrput (or is incompatible with "
          << "program parameters), expect test set id=" << testSetId
          << "but obtained " << cacheTestId;
      throw runtime_error(err.str());
    }
    LOG(LIB_INFO) << "Test set: " << testSetId << " # of threads: " << savedThreadQty;
    vector<dist_t> allDists, allDists1;
    for (unsigned i = 0; i < knn.size(); ++i) {
      LOG(LIB_INFO) << "k-NN set id:" << i;
      const vector<unique_ptr<GoldStandard<dist_t>>>& gsKNN = managerGS.GetKNNGS(i);



      for(const unique_ptr<GoldStandard<dist_t>>& oneGS : gsKNN) {
        bool bFirst = true;
        for(const ResultEntry<dist_t>& e : oneGS->GetSortedEntries()) {
          allDists.push_back(e.mDist);
          if (bFirst) {
            bFirst = false;
            allDists1.push_back((e.mDist));
          }
        }
      }
    }
    dist_t meanDist =  Mean(&allDists[0], allDists.size());
    dist_t meanDist1 =  Mean(&allDists1[0], allDists1.size());
    LOG(LIB_INFO) << "Average distance is: " << meanDist;
    LOG(LIB_INFO) << "Average distance to the 1st neighbor is: " << meanDist1;
    ofstream out(outFile.c_str());
    out << meanDist1 << "\t" << meanDist << endl;
  }
}

int main(int argc, char *argv[]) {
  string      cacheGSFilePrefix;
  string      spaceType, distType;
  string      dataFile;
  string      outFile;
  string      projType;
  string      logFile;
  unsigned    maxNumData = 0;
  unsigned    maxNumQuery = 0;
  unsigned    testSetQty = 0;
  string      queryFile;
  string      knnArg;

  vector<unsigned>    knn;

  CmdOptions cmd_options;

  cmd_options.Add(new CmdParam(CACHE_PREFIX_GS_PARAM_OPT, CACHE_PREFIX_GS_PARAM_MSG,
                               &cacheGSFilePrefix, false, CACHE_PREFIX_GS_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(SPACE_TYPE_PARAM_OPT, SPACE_TYPE_PARAM_MSG,
                               &spaceType, true));
  cmd_options.Add(new CmdParam(DIST_TYPE_PARAM_OPT, DIST_TYPE_PARAM_MSG,
                               &distType, false, std::string(DIST_TYPE_FLOAT)));
  cmd_options.Add(new CmdParam(DATA_FILE_PARAM_OPT, DATA_FILE_PARAM_MSG,
                               &dataFile, true));
  cmd_options.Add(new CmdParam("outFile,o", "output file",
                               &outFile, true));

  cmd_options.Add(new CmdParam(QUERY_FILE_PARAM_OPT, QUERY_FILE_PARAM_MSG,
                               &queryFile, false, QUERY_FILE_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(MAX_NUM_QUERY_PARAM_OPT, MAX_NUM_QUERY_PARAM_MSG,
                               &maxNumQuery, false, MAX_NUM_QUERY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(TEST_SET_QTY_PARAM_OPT, TEST_SET_QTY_PARAM_MSG,
                               &testSetQty, false, TEST_SET_QTY_PARAM_DEFAULT));

  cmd_options.Add(new CmdParam(KNN_PARAM_OPT, KNN_PARAM_MSG,
                               &knnArg, false));
  cmd_options.Add(new CmdParam(MAX_NUM_DATA_PARAM_OPT, MAX_NUM_DATA_PARAM_MSG,
                               &maxNumData, false, 0));
  cmd_options.Add(new CmdParam(LOG_FILE_PARAM_OPT, LOG_FILE_PARAM_MSG,
                               &logFile, false, ""));

  try {
    cmd_options.Parse(argc, argv);

    if (knnArg.empty() || !SplitStr(knnArg, knn, ',')) {
      LOG(LIB_FATAL) << "Wrong format of the KNN argument: '" << knnArg;
    }

    if (dataFile.empty()) {
      LOG(LIB_FATAL) << DATA_FILE_PARAM_OPT << " is not specified!";
    }

    if (cacheGSFilePrefix.empty()) {
      LOG(LIB_FATAL) << CACHE_PREFIX_GS_PARAM_OPT << " is not specified!";
    }
  } catch (const CmdParserException& e) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (const std::exception& e) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << "Failed to parse cmd arguments";
  }

  initLibrary(0, logFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, logFile.c_str());

  LOG(LIB_INFO) << "Program arguments are processed";

  ToLower(distType);

  try {

    if (distType == DIST_TYPE_FLOAT) {
      // Possible TODO: we currently support on the float distance type
      getCacheStat<float>(
        cacheGSFilePrefix,
        outFile,
        spaceType,
        dataFile, queryFile, testSetQty,
        maxNumData, maxNumQuery,
        knn
      );
    } else {
      LOG(LIB_FATAL) << "Unsupported distance type: '" << distType << "'";
    }

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << e.what();
  } catch (...) {

    LOG(LIB_FATAL) << "Failed to process gold-standard cache!";
  }

}