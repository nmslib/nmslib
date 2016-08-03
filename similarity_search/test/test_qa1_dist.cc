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
#include <sstream>
#include <iomanip>
#include <limits>
#include <string>
#include <algorithm>

#include <boost/program_options.hpp>

#include "init.h"
#include "global.h"
#include "utils.h"
#include "ztimer.h"

#include "logging.h"
#include "spacefactory.h"
#include "qa/space_qa1.h"
#include "meta_analysis.h"
#include "params.h"
#include "params_def.h"

//#define USE_CLOCK_GETTIME


using namespace std;


#define PIVOT_FILE_PARAM_OPT      "pivotFile,p"
#define PIVOT_FILE_PARAM_MSG      "A pivot file"

#define PIVOT_QTY_PARAM_OPT       "numPivot,P"
#define PIVOT_QTY_PARAM_MSG       "Number of pivots"

#define SAMPLE_QTY_OPT         "sampleQty,S"
#define SAMPLE_QTY_MSG         "A sample size"

#define FIELD_ID_OPT           "fieldId,f"
#define FIELD_ID_MSG           "an Id of a field to collect statistics (the zero-based index of the field in the **HEADER** file"

#define REP_MEASURE_QTY_TOP    "repQty,R"
#define REP_MEASURE_QTY_MSG    "a number of times a single measurement is repeated (to work around timing imprecision)."
#define REP_MEASURE_QTY_DEFAULT 1

#define COMPUTE_STAT_OPT        "computeStat,c"
#define COMPUTE_STAT_MSG        "compute additional statistics"

#define CHECK_ALTERN_COMP_CORRECT_OPT "checkAlternDist,d"
#define CHECK_ALTERN_COMP_CORRECT_MSG "Check correctness of the bulk pivot-distance computation and computation of the proxy distance"

#define COMPARE_DIGIT_QTY_OPT   "compDigitQty"
#define COMPARE_DIGIT_QTY_MSG   "The number of significant digits used in verifying correctness of the bulk pivot-distance computation"
#define COMPARE_DIGIT_QTY_DEFAULT 5


using namespace similarity;
using namespace std;

// digits should be > 0
bool compareApprox(float a, float b, unsigned digits) {
  float maxMod = max(fabs(a), fabs(b));
  float scale =  pow(10.0f, digits);
  float lead  = pow(10.f, round(log10(maxMod)));

  float minSign = numeric_limits<float>::min() * scale;
  // These guys are just too small for us to bother about their differences
  if (maxMod < minSign) return true;
  float delta = lead / scale;
  float diff = fabs(a - b);
  return diff <= delta;
}

#ifdef USE_CLOCK_GETTIME
// High-resolution timer borrowed from stack-overflow http://stackoverflow.com/a/19898211
// Also fixed a bug in that SO implementation
#include <time.h>

struct timespec timer_start(){
  struct timespec start_time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
  return start_time;
}

double timer_end(struct timespec start_time){
  struct timespec end_time;
  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time);
  double diffInNanos = 1e9*(end_time.tv_sec - start_time.tv_sec) + end_time.tv_nsec - start_time.tv_nsec;
  return diffInNanos;
}
#endif

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

void printStat(const string &Name, vector<IdTypeUnsign> &vals) {
  LOG(LIB_INFO) << "Statistics for: " << Name;
  CHECK(vals.size() > 0);
  sort(vals.begin(), vals.end());
  size_t q25 = min(vals.size() - 1, size_t(vals.size()*.25));
  size_t q50 = min(vals.size() - 1, size_t(vals.size()*.5));
  size_t q75 = min(vals.size() - 1, size_t(vals.size()*.75));
  size_t q90 = min(vals.size() - 1, size_t(vals.size()*.9));
  float mean = 0;
  for (IdTypeUnsign v: vals) mean += v;
  if (vals.size()) mean /= vals.size();
  LOG(LIB_INFO) << "25% : " << vals[q25] << " 50%: " << vals[q50] << " 75%: " << vals[q75] << " 90%: " << vals[q90] << " mean: " << mean;
}

void RunTest(
    bool                   ComputeStat,
    bool                   CheckAlternDistComp,
    unsigned               CompareDigitQty,
    IdTypeUnsign           fieldId,
    const string&          DataFile,
    IdTypeUnsign           MaxNumData,
    const string&          PivotFile,
    IdTypeUnsign           NumPivot,
    const string&          QueryFile,
    IdTypeUnsign           MaxNumQuery,
    IdTypeUnsign           SampleQty,
    IdTypeUnsign           RepTimeQty
) {

  unique_ptr<Space<float>> space(SpaceFactoryRegistry<float>::
                                 Instance().CreateSpace(SPACE_QA1, AnyParams()));

  ObjectVector data;
  ObjectVector pivots;
  ObjectVector queries;
  vector<string> tmp;

  unique_ptr<DataFileInputState> inpState(space->ReadDataset(data, tmp, DataFile, MaxNumData));
  space->UpdateParamsFromFile(*inpState);

  LOG(LIB_INFO) << "Read: " << data.size() << " entries.";

  space->ReadDataset(queries, tmp, QueryFile, MaxNumQuery);

  LOG(LIB_INFO) << "Read: " << queries.size() << " queries.";

  SpaceQA1 *pSpaceQA1 = dynamic_cast<SpaceQA1 *>(space.get());
  CHECK_MSG(pSpaceQA1 != nullptr, "Wrong space type? Expecting to have created SpaceQA1");

  pSpaceQA1->setDontPrecomputeFlag(true);
  space->ReadDataset(pivots, tmp, PivotFile, NumPivot);
  LOG(LIB_INFO) << "Read: " << pivots.size() << " pivots.";



  LOG(LIB_INFO) << "Started computing pivot indices";
  PivotInvIndexHolder pivotIndx(pSpaceQA1->computeCosinePivotIndex(pivots),
                                pSpaceQA1->computeBM25PivotIndex(pivots),
                                pSpaceQA1->computeModel1PivotIndex(pivots), pivots.size());
  LOG(LIB_INFO) << "Pivot indices are computed!";

  {
    size_t qtyMismatchPivot = 0, qtyMismatchProxy = 0;
    LOG(LIB_INFO) << "Pivot-query statistics (using pivot indices) " <<
    "=============================================================";

    WallClockTimer timer, timerAdd;

    vector<double> timings;

    timer.reset();

    float sum = 0;

    for (IdTypeUnsign iq = 0; iq < queries.size(); ++iq) {
#ifdef USE_CLOCK_GETTIME
      struct timespec vartime = timer_start();
#else
      timerAdd.reset();
#endif

      vector<float> vDst;
      for (IdTypeUnsign r = 0; r < RepTimeQty; ++r) {
        pSpaceQA1->computePivotDistances(queries[iq], pivotIndx, vDst);
        if (!vDst.empty()) sum += vDst[0];
      }

#ifdef USE_CLOCK_GETTIME
      long time_elapsed_nanos = timer_end(vartime);
      timings.push_back(time_elapsed_nanos/1000.0/float(RepTimeQty));
#else
      timerAdd.split();
      timings.push_back(timerAdd.elapsed()/float(RepTimeQty));
#endif

      if (CheckAlternDistComp) {
        for (IdTypeUnsign pivotId = 0; pivotId < pivots.size(); ++pivotId) {
          float enMassDist = vDst[pivotId];
          float recompDistPivot = pSpaceQA1->IndexTimeDistance(pivots[pivotId], queries[iq]);
          bool cmpa = compareApprox(recompDistPivot, enMassDist, CompareDigitQty);
          //LOG(LIB_INFO) << " ### emMass vs indextime: " << enMassDist << " <-> " << recompDistPivot << " Cmpa status: " << cmpa;
          if (!cmpa) {
            LOG(LIB_INFO) << "Seems like a mismatch between IndexTimeDistance and distance computed in bulk=" << enMassDist
                          << " index-time distance recomputed individually: " << recompDistPivot;
            qtyMismatchPivot++;
          }
          float recompDistProxy = pSpaceQA1->ProxyDistance(pivots[pivotId], queries[iq]);
          //LOG(LIB_INFO) << " ### enMassDist vs Proxy << " << enMassDist << " <-> " << recompDistProxy << " Cmpa status: " << cmpa;
          cmpa = compareApprox(recompDistProxy, enMassDist, CompareDigitQty);
          if (!cmpa) {
            LOG(LIB_INFO) << "Seems like a mismatch between ProxyDistance and distance computed in bulk=" << enMassDist
                          << " proxy distance recomputed individually: " << recompDistProxy;
            qtyMismatchProxy++;
          }
        }
      }
    }

    timer.split();


    if (CheckAlternDistComp) {
      LOG(LIB_INFO) << "*******************************";

      LOG(LIB_INFO) << "Number of potential mismatches between pivot en-mass and index-time distance: " << qtyMismatchPivot << " out of " << queries.size() * pivots.size();
      LOG(LIB_INFO) << "Number of potential mismatches between pivot en-mass and proxy distance: "      << qtyMismatchProxy << " out of " << queries.size() * pivots.size();
    }

    LOG(LIB_INFO) << "*******************************";

    CHECK(timings.size() == queries.size());
    double mean = Mean(&timings[0], timings.size());
    double std  = StdDev(&timings[0], timings.size());
    double sigma = std / max(sqrt(timings.size()),1.0);

    LOG(LIB_INFO) << "Pivot-query en-MASS " << queries.size() << " comparisons with " << RepTimeQty << " repetitions took " << timer.elapsed()/1000.0  << " MILLI-seconds "
    << " or " << timer.elapsed() / float(queries.size())/float(RepTimeQty) << " MICROSECONDS PER COMPARISON (MAY INCLUDE OBJECT STAT) ";
    LOG(LIB_INFO) << " One comparision (in MICROSECONDS) mean/std/sigma: " << mean << "/" << std << "/" << sigma;

    LOG(LIB_INFO) << "ignore: " << sum;

  }

  {
    LOG(LIB_INFO) << "Document-query statistics" << "=============================================================";

    float sum = 0;

    WallClockTimer timer, timerAdd;

    IdTypeUnsign    docWordQty, queryWordQty, intersectSize,
                    queryTranRecsQty, queryTranObjIntersectSize;

    vector<IdTypeUnsign>    vDocWordQty, vQueryWordQty, vIntersectSize,
        vQueryTranRecsQty, vQueryTranRecsPerWordQty, vQueryTranObjIntersectSize, vLookupQty;

    vector<double> timings;

    timer.reset();

    for (IdTypeUnsign i = 0; i < SampleQty; ++i) {
      IdType iq = RandomInt() % queries.size();
      IdType id = RandomInt() % data.size();

#ifdef USE_CLOCK_GETTIME
      struct timespec vartime = timer_start();
#else
      timerAdd.reset();
#endif

      for (IdTypeUnsign r = 0; r < RepTimeQty; ++r)
        sum += pSpaceQA1->IndexTimeDistance(data[id], queries[iq])*(r+1) + r; // Let compiler not optimize function call.

#ifdef USE_CLOCK_GETTIME
      long time_elapsed_nanos = timer_end(vartime);
      timings.push_back(time_elapsed_nanos/1000.0/float(RepTimeQty));
#else
      timerAdd.split();
      timings.push_back(timerAdd.elapsed()/float(RepTimeQty));
#endif


      if (ComputeStat) {
        pSpaceQA1->getObjStat(data[id], queries[iq], fieldId,
                              docWordQty, queryWordQty, intersectSize,
                              queryTranRecsQty, queryTranObjIntersectSize);

        vDocWordQty.push_back(docWordQty);
        vQueryWordQty.push_back(queryWordQty);
        vIntersectSize.push_back(intersectSize);
        vQueryTranRecsQty.push_back(queryTranRecsQty);
        vQueryTranRecsPerWordQty.push_back(queryTranRecsQty/max(IdTypeUnsign(1),queryWordQty));
        vQueryTranObjIntersectSize.push_back(queryTranObjIntersectSize);
        vLookupQty.push_back(queryWordQty*docWordQty);
      }

    }

    timer.split();

    LOG(LIB_INFO) << "*******************************";

    CHECK(timings.size() == SampleQty);
    double mean = Mean(&timings[0], timings.size());
    double std  = StdDev(&timings[0], timings.size());
    double sigma = std / max(sqrt(timings.size()),1.0);

    LOG(LIB_INFO) << "Document-query " << SampleQty << " comparisons with " << RepTimeQty << " repetitions took " << timer.elapsed()/1000.0  << " MILLI-seconds "
                  << " or " << timer.elapsed() / float(SampleQty)/float(RepTimeQty) << " MICROSECONDS PER ENTRY (MAY INCLUDE OBJECT STAT) ";
    LOG(LIB_INFO) << " One comparision (in MICROSECONDS) mean/std/sigma: " << mean << "/" << std << "/" << sigma;

    LOG(LIB_INFO) << "*******************************";

    if (ComputeStat) {
      printStat("# of document words", vDocWordQty);
      printStat("# of query words", vQueryWordQty);
      printStat("query-document intersection size", vIntersectSize);
      printStat("# of query tran. records", vQueryTranRecsQty);
      printStat("# of query tran. records per word ", vQueryTranRecsPerWordQty);
      printStat("# of query tran. records shared with documents", vQueryTranObjIntersectSize);
      printStat("# of lookups", vLookupQty);
    }


    LOG(LIB_INFO) << "ignore: " << sum;

  }

  {
    LOG(LIB_INFO) << "Pivot-query statistics" << "=============================================================";
    float sum = 0;

    WallClockTimer timer, timerAdd;

    vector<double> timings;

    timer.reset();

    IdTypeUnsign    docWordQty, queryWordQty, intersectSize,
                    queryTranRecsQty, queryTranObjIntersectSize;

    vector<IdTypeUnsign>    vDocWordQty, vQueryWordQty, vIntersectSize,
                            vQueryTranRecsQty, vQueryTranRecsPerWordQty, vQueryTranObjIntersectSize,
                            vLookupQty;

    for (IdTypeUnsign i = 0; i < SampleQty; ++i) {
      IdType iq = RandomInt() % queries.size();
      IdType ip = RandomInt() % pivots.size();

#ifdef USE_CLOCK_GETTIME
      struct timespec vartime = timer_start();
#else
      timerAdd.reset();
#endif

      for (IdTypeUnsign r = 0; r < RepTimeQty; ++r) {
        sum += pSpaceQA1->IndexTimeDistance(pivots[ip], queries[iq]) * (r + 1) +
               r; // Let compiler not optimize function call.
      }

#ifdef USE_CLOCK_GETTIME
      long time_elapsed_nanos = timer_end(vartime);
      timings.push_back(time_elapsed_nanos/1000.0/float(RepTimeQty));
#else
      timerAdd.split();
      timings.push_back(timerAdd.elapsed()/float(RepTimeQty));
#endif

      if (ComputeStat) {
        pSpaceQA1->getObjStat(pivots[ip], queries[iq], fieldId,
                              docWordQty, queryWordQty, intersectSize,
                              queryTranRecsQty, queryTranObjIntersectSize);

        vDocWordQty.push_back(docWordQty);
        vQueryWordQty.push_back(queryWordQty);
        vIntersectSize.push_back(intersectSize);
        vQueryTranRecsQty.push_back(queryTranRecsQty);
        vQueryTranRecsPerWordQty.push_back(queryTranRecsQty/max(IdTypeUnsign(1),queryWordQty));
        vQueryTranObjIntersectSize.push_back(queryTranObjIntersectSize);
        vLookupQty.push_back(queryWordQty*docWordQty);
      }
    }

    timer.split();
    CHECK(timings.size() == SampleQty);

    LOG(LIB_INFO) << "*******************************";

    CHECK(timings.size() == SampleQty);
    double mean = Mean(&timings[0], timings.size());
    double std  = StdDev(&timings[0], timings.size());
    double sigma = std / max(sqrt(timings.size()),1.0);

    LOG(LIB_INFO) << "Pivot-query " << SampleQty << " comparisons with " << RepTimeQty << " repetitions took " << timer.elapsed()/1000.0  << " MILLI-seconds "
                  << " or " << timer.elapsed() / float(SampleQty)/float(RepTimeQty) << " MICROSECONDS PER ENTRY (MAY INCLUDE OBJECT STAT) ";
    LOG(LIB_INFO) << " One comparision (in MICROSECONDS) mean/std/sigma: " << mean << "/" << std << "/" << sigma;

    LOG(LIB_INFO) << "*******************************";

    if (ComputeStat) {
      printStat("# of pivot words", vDocWordQty);
      printStat("# of query words", vQueryWordQty);
      printStat("query-pivot intersection size", vIntersectSize);
      printStat("# of query tran. records", vQueryTranRecsQty);
      printStat("# of query tran. records per word ", vQueryTranRecsPerWordQty);
      printStat("# of query tran. records shared with pivots", vQueryTranObjIntersectSize);
      printStat("# of lookups", vLookupQty);
    }

    LOG(LIB_INFO) << "ignore: " << sum;

    for (auto o : data) delete o;
    for (auto q : queries) delete q;
    for (auto p : pivots) delete p;
  }

  // NEXT,
  // 1) create translation entry # and keyword # statistics!
  // 2) make it possible to compute an overlap as a fraction of the original array sizes!

}

void ParseCommandLineForDistanceTesting(int argc, char*argv[],
    string&           LogFile,
    bool&             ComputeStat,
    bool&CheckAlternDistComp,
    unsigned&         CompareDigitQty,
    IdTypeUnsign&     FieldId,
    string&           DataFile,
    IdTypeUnsign&     MaxNumData,
    string&           PivotFile,
    IdTypeUnsign&     NumPivot,
    string&           QueryFile,
    IdTypeUnsign&     MaxNumQuery,
    IdTypeUnsign&     SampleQty,
    IdTypeUnsign&     RepTimeQty
) {
  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
      (HELP_PARAM_OPT,          HELP_PARAM_MSG)
      (LOG_FILE_PARAM_OPT,      po::value<string>(&LogFile)->default_value(LOG_FILE_PARAM_DEFAULT),           LOG_FILE_PARAM_MSG)

      (COMPUTE_STAT_OPT,        po::bool_switch(&ComputeStat),                                                COMPUTE_STAT_MSG)
      (CHECK_ALTERN_COMP_CORRECT_OPT, po::bool_switch(&CheckAlternDistComp), CHECK_ALTERN_COMP_CORRECT_MSG)
      (COMPARE_DIGIT_QTY_OPT,   po::value<unsigned>(&CompareDigitQty)->default_value(COMPARE_DIGIT_QTY_DEFAULT), COMPUTE_STAT_MSG)

      (FIELD_ID_OPT,            po::value<IdTypeUnsign>(&FieldId)->required(),                                FIELD_ID_MSG)

      (DATA_FILE_PARAM_OPT,     po::value<string>(&DataFile)->required(),                                     DATA_FILE_PARAM_MSG)
      (MAX_NUM_DATA_PARAM_OPT,  po::value<unsigned>(&MaxNumData)->default_value(MAX_NUM_DATA_PARAM_DEFAULT),  MAX_NUM_DATA_PARAM_MSG)

      (QUERY_FILE_PARAM_OPT,    po::value<string>(&QueryFile)->required(),                                     QUERY_FILE_PARAM_MSG)
      (MAX_NUM_QUERY_PARAM_OPT, po::value<unsigned>(&MaxNumQuery)->default_value(MAX_NUM_QUERY_PARAM_DEFAULT), MAX_NUM_QUERY_PARAM_MSG)

      (PIVOT_FILE_PARAM_OPT,    po::value<string>(&PivotFile)->required(),                                     PIVOT_FILE_PARAM_MSG)
      (PIVOT_QTY_PARAM_OPT,     po::value<unsigned>(&NumPivot)->required(),                                    PIVOT_QTY_PARAM_MSG)

      (SAMPLE_QTY_OPT,          po::value<unsigned>(&SampleQty)->required(),                                   SAMPLE_QTY_MSG)
      (REP_MEASURE_QTY_TOP,     po::value<unsigned>(&RepTimeQty)->default_value(REP_MEASURE_QTY_DEFAULT),      REP_MEASURE_QTY_MSG)
      ;

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

  try {
    if (!DoesFileExist(DataFile)) {
      LOG(LIB_FATAL) << "data file " << DataFile << " doesn't exist";
    }
    if (!DoesFileExist(QueryFile)) {
      LOG(LIB_FATAL) << "query file " << QueryFile << " doesn't exist";
    }
    if (!DoesFileExist(PivotFile)) {
      LOG(LIB_FATAL) << "pivot file " << PivotFile << " doesn't exist";
    }

    CHECK_MSG(MaxNumData < MAX_DATASET_QTY, "The maximum number of points should not exceed" + ConvertToString(MAX_DATASET_QTY));
  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }
}

int main(int argc, char* argv[]) {
  WallClockTimer timer;
  timer.reset();

  string           LogFile;
  bool             ComputeStat;
  bool             CheckAlternDistComp;
  unsigned         CompareDigitQty;
  string           DataFile;
  IdTypeUnsign     FieldId;
  IdTypeUnsign     MaxNumData;
  string           PivotFile;
  IdTypeUnsign     NumPivot;
  string           QueryFile;
  IdTypeUnsign     MaxNumQuery;
  IdTypeUnsign     SampleQty;
  IdTypeUnsign     RepTimeQty;

  try {

    ParseCommandLineForDistanceTesting(argc, argv,
                                      LogFile,
                                      ComputeStat,
                                      CheckAlternDistComp,
                                      CompareDigitQty,
                                      FieldId,
                                      DataFile,
                                      MaxNumData,
                                      PivotFile,
                                      NumPivot,
                                      QueryFile,
                                      MaxNumQuery,
                                      SampleQty,
                                      RepTimeQty);

    initLibrary(LogFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, LogFile.c_str());

    LOG(LIB_INFO) << "Program arguments are processed";

    RunTest(ComputeStat,
            CheckAlternDistComp,
            CompareDigitQty,
            FieldId,
            DataFile,
            MaxNumData,
            PivotFile,
            NumPivot,
            QueryFile,
            MaxNumQuery,
            SampleQty,
            RepTimeQty);

    timer.split();
    LOG(LIB_INFO) << "Time elapsed = " << timer.elapsed() / 1e6;
    LOG(LIB_INFO) << "Finished at " << LibGetCurrentTime();
  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception";
  }

  return 0;
}
