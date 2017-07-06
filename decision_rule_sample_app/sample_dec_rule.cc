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

#include <iostream>
#include <string>

#include "space.h"
#include "params.h"
#include "init.h"
#include "report_intr_dim.h"
#include "spacefactory.h"
#include "cmd_options.h"
#include "searchoracle_old.h"
#include "method/vptree_utils.h"

using namespace similarity;
using namespace std;

template <typename dist_t>
void SampleDecisionRule(
                string spaceDesc,
                string dataFile,
                string outFile,
                unsigned maxNumData,
                string sampleParamDesc
               ) {
  string          spaceType;
  vector<string>  vSpaceArgs;

  ParseSpaceArg(spaceDesc, spaceType, vSpaceArgs);
  AnyParams   spaceParams({AnyParams(vSpaceArgs)});

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                Instance().CreateSpace(spaceType, spaceParams));

  ObjectVector    data;
  vector<string>  tmp;
  unique_ptr<DataFileInputState> inpState(space->ReadDataset(data, tmp, dataFile, maxNumData));
  space->UpdateParamsFromFile(*inpState);

  vector<string>  vOracleArgs;
  ParseArg(sampleParamDesc, vOracleArgs);
  AnyParams   sampleParams({AnyParams(vOracleArgs)});

  bool      DoRandSample                  = true;
  unsigned  MaxK                          = 100;
  float     QuantileStepPivot             = 0.005f;
  float     QuantileStepPseudoQuery       = 0.001f;
  size_t    NumOfPseudoQueriesInQuantile  = 5;
  float     DistLearnThreshold            = 0.05f;
  bool      SquareRootTransf              = false;

  AnyParamManager pmgr(sampleParams);

  pmgr.GetParamOptional("doRandSample",            DoRandSample, DoRandSample);
  pmgr.GetParamOptional("maxK",                    MaxK, MaxK);
  pmgr.GetParamOptional("quantileStepPivot",       QuantileStepPivot, QuantileStepPivot);
  pmgr.GetParamOptional("quantileStepPseudoQuery", QuantileStepPseudoQuery, QuantileStepPseudoQuery);
  pmgr.GetParamOptional("numOfPseudoQueriesInQuantile", NumOfPseudoQueriesInQuantile, NumOfPseudoQueriesInQuantile);
  pmgr.GetParamOptional("distLearnThresh",         DistLearnThreshold, DistLearnThreshold);
  pmgr.GetParamOptional("squareRootTransf",        SquareRootTransf, SquareRootTransf);

  pmgr.CheckUnused();

  LOG(LIB_INFO) << "doRandSample                =" << DoRandSample;
  LOG(LIB_INFO) << "maxK                        =" << MaxK;
  LOG(LIB_INFO) << "quantileStepPivot           =" << QuantileStepPivot;
  LOG(LIB_INFO) << "quantileStepPseudoQuery     =" << QuantileStepPseudoQuery;
  LOG(LIB_INFO) << "numOfPseudoQueriesInQuantile=" << NumOfPseudoQueriesInQuantile;
  LOG(LIB_INFO) << "distLearnThresh             =" << DistLearnThreshold;
  LOG(LIB_INFO) << "SquareRootTransf            =" << SquareRootTransf;

  SamplingOracleCreator<dist_t> oracleCreator(space.get(), data,
                                              DoRandSample,
                                              MaxK,
                                              QuantileStepPivot,
                                              QuantileStepPseudoQuery,
                                              NumOfPseudoQueriesInQuantile,
                                              DistLearnThreshold);

  const size_t index = SelectVantagePoint(data, true);
  const Object* pivot = data[index];

  DistObjectPairVector<dist_t> dp;
  for (size_t i = 0; i < data.size(); ++i) {
    if (i == index) {
      continue;
    }
    dist_t d = space->IndexTimeDistance(pivot, data[i]);
    if (SquareRootTransf)  d = sqrt(d);
    dp.push_back(std::make_pair(d, data[i]));
  }

  sort(dp.begin(), dp.end(), DistObjectPairAscComparator<dist_t>());
  DistObjectPair<dist_t>  medianDistObj = GetMedian(dp);
  dist_t mediandist = medianDistObj.first;

  unique_ptr<SamplingOracle<dist_t>> oracle(oracleCreator.Create(0, pivot, dp));

  ofstream out(outFile, std::ios::trunc | std::ios::out);

  out << oracle->Dump() <<  mediandist << endl ;

  out.close();
}

int main(int argc, char* argv[]) {
  string    spaceDesc, distType;
  string    dataFile;
  string    outFile;
  unsigned  maxNumData = 0;
  string    sampleParams;

  CmdOptions cmd_options;

  cmd_options.Add(new CmdParam("spaceType,s", "space type, e.g., l1, l2, lp:p=0.5",
                               &spaceDesc, true));
  cmd_options.Add(new CmdParam("distType", "distance value type: int, float, double",
                               &distType, false, DIST_TYPE_FLOAT));
  cmd_options.Add(new CmdParam("dataFile,i", "input data file",
                               &dataFile, true));
  cmd_options.Add(new CmdParam("maxNumData", "if non-zero, only the first maxNumData elements are used",
                               &maxNumData, false, 0));
  cmd_options.Add(new CmdParam("sampleParams,p", "sampling parameters",
                               &sampleParams, false, ""));
  cmd_options.Add(new CmdParam("outFile,o", "output file",
                               &outFile, true));

  try {
    cmd_options.Parse(argc, argv);

    if (!DoesFileExist(dataFile)) {
      PREPARE_RUNTIME_ERR(err) << "data file " << dataFile << " doesn't exist";
      THROW_RUNTIME_ERR(err);
    }

    initLibrary(LIB_LOGSTDERR);

    if (DIST_TYPE_INT == distType) {
      SampleDecisionRule<int>(
                  spaceDesc,
                  dataFile,
                  outFile,
                  maxNumData,
                  sampleParams
                );
    } else if (DIST_TYPE_FLOAT == distType) {
      SampleDecisionRule<float>(
                  spaceDesc,
                  dataFile,
                  outFile,
                  maxNumData,
                  sampleParams
                 );
    } else if (DIST_TYPE_DOUBLE == distType) {
      SampleDecisionRule<double>(
                  spaceDesc,
                  dataFile,
                  outFile,
                  maxNumData,
                  sampleParams
                 );
    }

  } catch (const CmdParserException& e) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (const exception& e) {
    cmd_options.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  }

  return 0;
}
