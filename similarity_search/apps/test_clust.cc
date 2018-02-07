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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <cmath>
#include <memory>
#include <limits>
#include <string>
#include <sstream>
#include <vector>
#include <fstream>
#include <map>

#include <cluster_util.h>

#include "init.h"
#include "global.h"
#include "utils.h"
#include "ztimer.h"
#include "experiments.h"
#include "tune.h"
#include "method/vptree.h"
#include "method/proj_vptree.h"
#include "method/perm_bin_vptree.h"
#include "logging.h"
#include "spacefactory.h"
#include "cluster_util.h"
#include "params_def.h"

#include "meta_analysis.h"
#include "params.h"
#include "cmd_options.h"

using namespace similarity;

const string CLUST_TYPE_PARAM_OPT="clustType,t";
const string CLUST_TYPE_PARAM_MSG="A type of cluster: " + CLUST_TYPE_CLARAN + "," + CLUST_TYPE_FIRMAL;
const string CLUST_QTY_PARAM_OPT="clustQty,c";
const string CLUST_QTY_PARAM_MSG="A # of clusters";
const IdTypeUnsign CLUST_QTY_PARAM_DEFAULT = 100;
const string IN_CLUST_SWAP_ATT_PARAM_OPT = "swapAtt,W";
const string IN_CLUST_SWAP_ATT_PARAM_MSG = "The number of in claster swap attempts (in order to find a better center)";
const IdTypeUnsign IN_CLUST_SWAP_ATT_PARAM_DEFAULT = CLARANS_SWAP_ATTEMPTS;
const string IN_CLUST_SAMPLE_QTY_PARAM_OPT = "clustSampleQty,Q";
const string IN_CLUST_SAMPLE_QTY_PARAM_MSG = "The number of sampled points inside the cluster to compute a cluster configuration cost";
const IdTypeUnsign IN_CLUST_SAMPLE_QTY_PARAM_DEFAULT = CLARANS_SAMPLE_QTY;
const string RAND_REST_QTY_PARAM_OPT = "randRestartQty,R";
const string RAND_REST_QTY_PARAM_MSG = "The number of random restarts";
const size_t RAND_REST_QTY_PARAM_DEFAULT = 5;
const string SEARCH_CLOSE_ITER_QTY_PARAM_OPT = "searchCloseIterQty,I";
const string SEARCH_CLOSE_ITER_QTY_PARAM_MSG = "A number of search iterations to find a point that is close to already selected centers";
const IdTypeUnsign SEARCH_CLOSE_ITER_QTY_PARAM_DEFAULT = 200;
const string DIST_SAMPLE_QTY_PARAM_OPT = "distSampleQty,S";
const string DIST_SAMPLE_QTY_PARAM_MSG = "A number of samples to determine the distribution of distances";
const IdTypeUnsign DIST_SAMPLE_QTY_PARAM_DEFAULT = SAMPLE_LIST_CLUST_DEFAULT_SAMPLE_QTY;
const string MAX_META_ITER_QTY_PARAM_OPT = "maxMetaIterQty,M";
const string MAX_META_ITER_QTY_PARAM_MSG = "A maximum number of meta iterations";
const IdTypeUnsign MAX_META_ITER_QTY_PARAM_DEFAULT = 10;
const string KEEP_FRAC_QTY_PARAM_OPT = "keepFrac,F";
const string KEEP_FRAC_QTY_PARAM_MSG = "Percentage of assigned points kept after a meta-iteration is finished";
const float KEEP_FRAC_QTY_PARAM_DEFAULT = 0.2;

using std::vector;
using std::map;
using std::make_pair;
using std::string;
using std::stringstream;


template <typename dist_t>
void RunExper(
    bool                   PrintProgress,
    const string&          DistType,
    const string&          SpaceType,
    shared_ptr<AnyParams>  SpaceParams,
    const string&          DataFile,
    unsigned               MaxNumData,
    const string&          ClustType,
    IdTypeUnsign           ClustQty,
    // reductive CLARANS parameters
    IdTypeUnsign           maxMetaIterQty,
    float                  keepFrac,
    // CLARANS & reductive CLARANS parameters
    IdTypeUnsign           inClusterSwapAttempts,
    IdTypeUnsign           inClusterSampleQty, // Number of random points to estimate if the swap was beneficial
    size_t                 randRestQty,
    // FIRMAL parameters
    size_t                 searchCloseIterQty, // A # of search iterations to find a point that is close to already selected centers
                                          // For good performance it should be in the order of sqrt(data.size())
    size_t                 sampleDistQty      // A # of sample to determine the distribution of distances)
) {

  unique_ptr<Space<dist_t>> space (SpaceFactoryRegistry<dist_t>::
                                   Instance().CreateSpace(SpaceType, *SpaceParams));

  ObjectVector              data;
  vector<string>            tmp;

  unique_ptr<DataFileInputState> inpState(space->ReadDataset(data, tmp, DataFile, MaxNumData));
  space->UpdateParamsFromFile(*inpState);

  LOG(LIB_INFO) << "Read: " << data.size() << " entries.";

  ObjectVector                                      vCenters;
  ObjectVector                                      vUnassigned;
  vector<shared_ptr<DistObjectPairVector<dist_t>>>  vClusterAssign;

  if (ClustType == CLUST_TYPE_CLARAN) {
    ClusterUtils<dist_t>::doCLARANS(PrintProgress, *space, data, ClustQty, vCenters, vClusterAssign,
                                    inClusterSwapAttempts, inClusterSampleQty, randRestQty);
  } else if (ClustType == CLUST_TYPE_REDUCT_CLARAN) {
    ClusterUtils<dist_t>::doReductiveCLARANS(PrintProgress, *space, data,
                                             maxMetaIterQty, keepFrac,
                                             ClustQty, vCenters, vClusterAssign, vUnassigned,
                                             inClusterSwapAttempts, inClusterSampleQty);
  } else if (ClustType == CLUST_TYPE_FIRMAL) {
    ClusterUtils<dist_t>::doFIRMAL(PrintProgress, *space, data, ClustQty, vCenters, vClusterAssign, vUnassigned,
                                   searchCloseIterQty, sampleDistQty);
  } else {
    throw runtime_error("Unsupported clustering type: " + ClustType);
  }

  LOG(LIB_INFO) << "The number of unassigned points: " << vUnassigned.size();

  ClusterUtils<dist_t>::printAndVerifyClusterStat(*space, vCenters, vClusterAssign, 1000);
}

void ParseCommandLineForClustering(int argc, char*argv[],
   bool&                   PrintProgress,
   string&                 LogFile,
   string&                 DistType,
   string&                 SpaceType,
   shared_ptr<AnyParams>&  SpaceParams,
   string&                 DataFile,
   unsigned&               MaxNumData,
   string&                 ClustType,
   IdTypeUnsign&           ClustQty,
// reductive CLARANS parameters
   IdTypeUnsign&           maxMetaIterQty,
   float&                  keepFrac,
// CLARANS parameters
   IdTypeUnsign&           inClusterSwapAttempts,
   IdTypeUnsign&           inClusterSampleQty, // Number of random points to estimate if the swap was beneficial
   IdTypeUnsign&           RandRestQty,
// FIRMAL parameters
   IdTypeUnsign&           SearchCloseIterQty,// A # of search iterations to find a point that is close to already selected centers
// For good performance it should be in the order of sqrt(data.size())
   IdTypeUnsign&           SampleDistQty      // A # of sample to determine the distribution of distances)

) {
  bool NoPrintProgress;

  CmdOptions cmd_options;

  cmd_options.Add(new CmdParam(NO_PROGRESS_PARAM_OPT, NO_PROGRESS_PARAM_MSG,
                               &NoPrintProgress, false));
  cmd_options.Add(new CmdParam(SPACE_TYPE_PARAM_OPT, SPACE_TYPE_PARAM_MSG,
                               &SpaceType, true));
  cmd_options.Add(new CmdParam(DIST_TYPE_PARAM_OPT, DIST_TYPE_PARAM_MSG,
                               &DistType, false, DIST_TYPE_FLOAT));
  cmd_options.Add(new CmdParam(DATA_FILE_PARAM_OPT, DATA_FILE_PARAM_MSG,
                               &DataFile, true));
  cmd_options.Add(new CmdParam(MAX_NUM_DATA_PARAM_OPT, MAX_NUM_QUERY_PARAM_MSG,
                               &MaxNumData, false, MAX_NUM_DATA_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(LOG_FILE_PARAM_OPT, LOG_FILE_PARAM_MSG,
                               &LogFile, false, LOG_FILE_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(CLUST_TYPE_PARAM_OPT, CLUST_TYPE_PARAM_MSG,
                               &ClustType, true));
  cmd_options.Add(new CmdParam(CLUST_QTY_PARAM_OPT, CLUST_QTY_PARAM_MSG,
                               &ClustQty, false, CLUST_QTY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(IN_CLUST_SWAP_ATT_PARAM_OPT, IN_CLUST_SWAP_ATT_PARAM_MSG,
                               &inClusterSwapAttempts, false, IN_CLUST_SWAP_ATT_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(IN_CLUST_SAMPLE_QTY_PARAM_OPT, IN_CLUST_SAMPLE_QTY_PARAM_MSG,
                               &inClusterSampleQty, false, IN_CLUST_SAMPLE_QTY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(RAND_REST_QTY_PARAM_OPT, RAND_REST_QTY_PARAM_MSG,
                               &RandRestQty, false, RAND_REST_QTY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(SEARCH_CLOSE_ITER_QTY_PARAM_OPT, SEARCH_CLOSE_ITER_QTY_PARAM_MSG,
                               &SearchCloseIterQty, false, SEARCH_CLOSE_ITER_QTY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(DIST_SAMPLE_QTY_PARAM_OPT, DIST_SAMPLE_QTY_PARAM_MSG,
                               &SampleDistQty, false, DIST_SAMPLE_QTY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(MAX_META_ITER_QTY_PARAM_OPT, MAX_META_ITER_QTY_PARAM_MSG,
                               &maxMetaIterQty, false, MAX_META_ITER_QTY_PARAM_DEFAULT));
  cmd_options.Add(new CmdParam(KEEP_FRAC_QTY_PARAM_OPT, KEEP_FRAC_QTY_PARAM_MSG,
                               &keepFrac, false, KEEP_FRAC_QTY_PARAM_DEFAULT));

  try {
    cmd_options.Parse(argc, argv);
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

  PrintProgress = !NoPrintProgress;

  ToLower(ClustType);
  ToLower(SpaceType);

  try {
    {
      vector<string> SpaceDesc;
      string str = SpaceType;
      ParseSpaceArg(str, SpaceType, SpaceDesc);
      SpaceParams = shared_ptr<AnyParams>(new AnyParams(SpaceDesc));
    }

    if (DataFile.empty()) {
      LOG(LIB_FATAL) << "data file is not specified!";
    }

    if (!DoesFileExist(DataFile)) {
      LOG(LIB_FATAL) << "data file " << DataFile << " doesn't exist";
    }
    CHECK_MSG(MaxNumData < MAX_DATASET_QTY, "The maximum number of points should not exceed" + ConvertToString(MAX_DATASET_QTY));
  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }
}

int main(int argc, char* argv[]) {
  WallClockTimer timer;
  timer.reset();

  bool                    PrintProgress;
  string                  LogFile;
  string                  DistType;
  string                  SpaceType;
  shared_ptr<AnyParams>   SpaceParams;
  string                  ClustType;
  IdTypeUnsign            ClustQty;
  string                  DataFile;
  unsigned                MaxNumData;

// reductive CLARANS parameters
  IdTypeUnsign            maxMetaIterQty;
  float                   keepFrac;
  // CLARANS parameters
  IdTypeUnsign           inClusterSwapAttempts;
  IdTypeUnsign           inClusterSampleQty; // Number of random points to estimate if the swap was beneficial
  IdTypeUnsign           RandRestQty;
  // FIRMAL parameters
  IdTypeUnsign           SearchCloseIterQty;// A # of search iterations to find a point that is close to already selected centers
  // For good performance it should be in the order of sqrt(data.size())
  IdTypeUnsign           SampleDistQty;      // A # of sample to determine the distribution of distances)

  ParseCommandLineForClustering(argc, argv,
                                PrintProgress,
                                LogFile,
                                DistType,
                                SpaceType,
                                SpaceParams,
                                DataFile,
                                MaxNumData,
                                ClustType,
                                ClustQty,
      // Reductive CLARANS parameters
                                maxMetaIterQty,
                                keepFrac,
      // CLARANS parameters
                                inClusterSwapAttempts,
                                inClusterSampleQty,
                                RandRestQty,
      // FIRMAL paramaters
                                SearchCloseIterQty,
                                SampleDistQty
  );

  initLibrary(0, LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

  ToLower(DistType);
  ToLower(ClustType);

  if (!SpaceParams) {
    LOG(LIB_FATAL) << "Failed to initialized space parameters!";
  }


  if (DIST_TYPE_INT == DistType) {
    RunExper<int>(PrintProgress,
                  DistType,
                  SpaceType,
                  SpaceParams,
                  DataFile,
                  MaxNumData,
                  ClustType,
                  ClustQty,
                  maxMetaIterQty,
                  keepFrac,
                  inClusterSwapAttempts,
                  inClusterSampleQty,
                  RandRestQty,
                  SearchCloseIterQty,
                  SampleDistQty
    );
  } else if (DIST_TYPE_FLOAT == DistType) {
    RunExper<float>(PrintProgress,
                    DistType,
                    SpaceType,
                    SpaceParams,
                    DataFile,
                    MaxNumData,
                    ClustType,
                    ClustQty,
                    maxMetaIterQty,
                    keepFrac,
                    inClusterSwapAttempts,
                    inClusterSampleQty,
                    RandRestQty,
                    SearchCloseIterQty,
                    SampleDistQty);
  } else if (DIST_TYPE_DOUBLE == DistType) {
    RunExper<double>(PrintProgress,
                     DistType,
                     SpaceType,
                     SpaceParams,
                     DataFile,
                     MaxNumData,
                     ClustType,
                     ClustQty,
                     maxMetaIterQty,
                     keepFrac,
                     inClusterSwapAttempts,
                     inClusterSampleQty,
                     RandRestQty,
                     SearchCloseIterQty,
                     SampleDistQty);
  } else {
    LOG(LIB_FATAL) << "Unknown distance value type: " << DistType;
  }

  timer.split();
  LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
  LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();

  return 0;
}
