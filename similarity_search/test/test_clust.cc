/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2015
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

#include <boost/program_options.hpp>
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

using namespace similarity;

#define CLUST_TYPE_PARAM_OPT "clustType,t"
const string CLUST_TYPE_PARAM_MSG="A type of cluster: " + CLUST_TYPE_CLARAN + "," + CLUST_TYPE_FIRMAL;
#define CLUST_QTY_PARAM_OPT "clustQty,c"
#define CLUST_QTY_PARAM_MSG "A # of clusters"
const IdTypeUnsign CLUST_QTY_PARAM_DEFAULT = 100;
#define IN_CLUST_SWAP_ATT_PARAM_OPT "swapAtt,W"
#define IN_CLUST_SWAP_ATT_PARAM_MSG "The number of in claster swap attempts (in order to find a better center)"
const IdTypeUnsign IN_CLUST_SWAP_ATT_PARAM_DEFAULT = CLARANS_SWAP_ATTEMPTS;
#define IN_CLUST_SAMPLE_QTY_PARAM_OPT "clustSampleQty,Q"
#define IN_CLUST_SAMPLE_QTY_PARAM_MSG "The number of sampled points inside the cluster to compute a cluster configuration cost"
const IdTypeUnsign IN_CLUST_SAMPLE_QTY_PARAM_DEFAULT = CLARANS_SAMPLE_QTY;
#define RAND_REST_QTY_PARAM_OPT "randRestartQty,R"
#define RAND_REST_QTY_PARAM_MSG "The number of random restarts"
const size_t RAND_REST_QTY_PARAM_DEFAULT = 5;
#define SEARCH_CLOSE_ITER_QTY_PARAM_OPT "searchCloseIterQty,I"
#define SEARCH_CLOSE_ITER_QTY_PARAM_MSG "A number of search iterations to find a point that is close to already selected centers"
const IdTypeUnsign SEARCH_CLOSE_ITER_QTY_PARAM_DEFAULT = 200;
#define DIST_SAMPLE_QTY_PARAM_OPT "distSampleQty,S"
#define DIST_SAMPLE_QTY_PARAM_MSG "A number of samples to determine the distribution of distances"
const IdTypeUnsign DIST_SAMPLE_QTY_PARAM_DEFAULT = SAMPLE_LIST_CLUST_DEFAULT_SAMPLE_QTY;
#define MAX_META_ITER_QTY_PARAM_OPT "maxMetaIterQty,M"
#define MAX_META_ITER_QTY_PARAM_MSG "A maximum number of meta iterations"
const IdTypeUnsign MAX_META_ITER_QTY_PARAM_DEFAULT = 10;
#define KEEP_FRAC_QTY_PARAM_OPT "keepFrac,F"
#define KEEP_FRAC_QTY_PARAM_MSG "Percentage of assigned points kept after a meta-iteration is finished"
const float KEEP_FRAC_QTY_PARAM_DEFAULT = 0.2;

using std::vector;
using std::map;
using std::make_pair;
using std::string;
using std::stringstream;

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

template <typename dist_t>
void RunExper(
    bool                   PrintProgress,
    const string&          DistType,
    const string&          SpaceType,
    shared_ptr<AnyParams>  SpaceParams,
    const string&          DataFile,
    IdTypeUnsign           MaxNumData,
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


  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
      (HELP_PARAM_OPT,          HELP_PARAM_MSG)
      (NO_PROGRESS_PARAM_OPT,   po::bool_switch(&NoPrintProgress), NO_PROGRESS_PARAM_MSG)
      (SPACE_TYPE_PARAM_OPT,    po::value<string>(&SpaceType)->required(),                    SPACE_TYPE_PARAM_MSG)
      (DIST_TYPE_PARAM_OPT,     po::value<string>(&DistType)->default_value(DIST_TYPE_FLOAT), DIST_TYPE_PARAM_MSG)
      (DATA_FILE_PARAM_OPT,     po::value<string>(&DataFile)->required(),                     DATA_FILE_PARAM_MSG)
      (MAX_NUM_DATA_PARAM_OPT,  po::value<unsigned>(&MaxNumData)->default_value(MAX_NUM_DATA_PARAM_DEFAULT), MAX_NUM_DATA_PARAM_MSG)
      (LOG_FILE_PARAM_OPT,      po::value<string>(&LogFile)->default_value(LOG_FILE_PARAM_DEFAULT),          LOG_FILE_PARAM_MSG)
      (CLUST_TYPE_PARAM_OPT,    po::value<string>(&ClustType)->required(),      CLUST_TYPE_PARAM_MSG.c_str())
      (CLUST_QTY_PARAM_OPT,    po::value<IdTypeUnsign>(&ClustQty)->default_value(CLUST_QTY_PARAM_DEFAULT), CLUST_QTY_PARAM_MSG)
      (IN_CLUST_SWAP_ATT_PARAM_OPT, po::value<IdTypeUnsign>(&inClusterSwapAttempts)->default_value(IN_CLUST_SWAP_ATT_PARAM_DEFAULT), IN_CLUST_SWAP_ATT_PARAM_MSG)
      (IN_CLUST_SAMPLE_QTY_PARAM_OPT, po::value<IdTypeUnsign>(&inClusterSampleQty)->default_value(IN_CLUST_SAMPLE_QTY_PARAM_DEFAULT), IN_CLUST_SAMPLE_QTY_PARAM_MSG)
      (RAND_REST_QTY_PARAM_OPT, po::value<IdTypeUnsign>(&RandRestQty)->default_value(RAND_REST_QTY_PARAM_DEFAULT), RAND_REST_QTY_PARAM_MSG)
      (SEARCH_CLOSE_ITER_QTY_PARAM_OPT,po::value<IdTypeUnsign>(&SearchCloseIterQty)->default_value(SEARCH_CLOSE_ITER_QTY_PARAM_DEFAULT), SEARCH_CLOSE_ITER_QTY_PARAM_MSG)
      (DIST_SAMPLE_QTY_PARAM_OPT, po::value<IdTypeUnsign>(&SampleDistQty)->default_value(DIST_SAMPLE_QTY_PARAM_DEFAULT), DIST_SAMPLE_QTY_PARAM_MSG)
      (MAX_META_ITER_QTY_PARAM_OPT, po::value<IdTypeUnsign>(&maxMetaIterQty)->default_value(MAX_META_ITER_QTY_PARAM_DEFAULT), MAX_META_ITER_QTY_PARAM_MSG)
      (KEEP_FRAC_QTY_PARAM_OPT, po::value<float>(&keepFrac)->default_value(KEEP_FRAC_QTY_PARAM_DEFAULT), KEEP_FRAC_QTY_PARAM_MSG)
      ;

  PrintProgress = !NoPrintProgress;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << e.what();
  }

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

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
  IdTypeUnsign            MaxNumData;

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

  initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

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
