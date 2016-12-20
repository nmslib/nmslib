/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2016
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#include <string>
#include <fstream>

#include "knnquery.h"
#include "knnqueue.h"
#include "space.h"
#include "params.h"
#include "projection.h"
#include "spacefactory.h"
#include "init.h"
#include "cmd_options.h"
#include "params_def.h"


using namespace similarity;
using namespace std;

template <class dist_t>
void sampleDist(string spaceType,
                string inFile, string outFile,
                unsigned knn, 
                unsigned maxNumData,
                unsigned knnQueryQty) {
  ToLower(spaceType);
  vector<string> spaceDesc;

  const string descStr = spaceType;
  ParseSpaceArg(descStr, spaceType, spaceDesc);
  unique_ptr<AnyParams> spaceParams =
            unique_ptr<AnyParams>(new AnyParams(spaceDesc));

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                Instance().CreateSpace(spaceType, *spaceParams));

  if (NULL == space.get()) {
    LOG(LIB_FATAL) << "Cannot create the space: '" << spaceType
                   << "' (desc: '" << descStr << "')";
  }

  ObjectVector      data;
  vector<string>    tmp;
  LOG(LIB_INFO) << "maxNumData=" << maxNumData;
  unique_ptr<DataFileInputState> inpState(space->ReadDataset(data, tmp, inFile, maxNumData));
  space->UpdateParamsFromFile(*inpState);

  size_t N = data.size();

  vector<char>      isQuery(N);

  ofstream out(outFile);

  LOG(LIB_INFO) << "knnQueryQty=" << knnQueryQty;

  if (knnQueryQty >= N/2) {
    LOG(LIB_FATAL) << "knnQueryQty is too large: should not exceed the number of data points / 2";
  }

  ObjectVector      queries;
  for (size_t i = 0; i < knnQueryQty; ++i) {
    IdType iSel=0;
    do {
      iSel = RandomInt() % N;
     // Should terminate quickly, b/c the probability of finding a data point, which is not previously selected, is >= 1/2
    } while (isQuery[iSel]); 
    isQuery[iSel]=1;
    queries.push_back(data[iSel]); 
  }

  vector<vector<dist_t>> outMatrix(knn);

  for (size_t qid = 0; qid < knnQueryQty; ++qid) {
    LOG(LIB_INFO) << "query index : " << qid << " id: " << queries[qid]->id();
    KNNQuery<dist_t> query(*space, queries[qid], knn);

    // Brute force search
    for (size_t k = 0; k < N; ++k) 
    if (!isQuery[k]) {
      query.CheckAndAddToResult(data[k]);
    }

    unique_ptr<KNNQueue<dist_t>> knnQ(query.Result()->Clone());


    vector<dist_t> dists1;
    dists1.reserve(knn);

    // Extracting results
    while (!knnQ->Empty()) {
      dists1.insert(dists1.begin(),knnQ->TopDistance());
      knnQ->Pop();
    }
    for (size_t k = 0; k < min<size_t>(dists1.size(), knn); ++k) {
      outMatrix[k].push_back(dists1[k]);
    }
  }

  for (size_t i = 0; i < outMatrix.size(); ++i) {
    const vector<dist_t>& dists2 = outMatrix[i];
    if (!dists2.empty()) out << dists2[0];
    for (size_t k = 1; k < dists2.size(); ++k) {
      out << "\t" << dists2[k];
    }
    out << endl;
  }

  out.close();
}

int main(int argc, char *argv[]) {
  string      spaceType, distType, projSpaceType;
  string      inFile, outFile;
  string      projType;
  string      logFile;
  unsigned    maxNumData = 0;
  unsigned    knnQueryQty = 0;
  unsigned    knn = 0;


  CmdOptions cmd_options;


  cmd_options.Add(new CmdParam(SPACE_TYPE_PARAM_OPT, SPACE_TYPE_PARAM_MSG,
                               &spaceType, true));
  cmd_options.Add(new CmdParam(DIST_TYPE_PARAM_OPT, DIST_TYPE_PARAM_MSG,
                               &distType, false, std::string(DIST_TYPE_FLOAT)));
  cmd_options.Add(new CmdParam("inFile,i", "input data file",
                               &inFile, true));
  cmd_options.Add(new CmdParam("outFile,o", "output data file",
                               &outFile, true));
  cmd_options.Add(new CmdParam("knnQueryQty", "number of randomly selected queries",
                               &knnQueryQty, false, 0));
  cmd_options.Add(new CmdParam(KNN_PARAM_OPT, "use this number of nearest neighbors",
                               &knn, 0));
  cmd_options.Add(new CmdParam(MAX_NUM_DATA_PARAM_OPT, MAX_NUM_DATA_PARAM_MSG,
                               &maxNumData, false, 0));
  cmd_options.Add(new CmdParam(LOG_FILE_PARAM_OPT, LOG_FILE_PARAM_MSG,
                               &logFile, false, ""));

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

  initLibrary(logFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, logFile.c_str());

  LOG(LIB_INFO) << "Program arguments are processed";


  ToLower(distType);


  try {
    if (knnQueryQty <= 0) LOG(LIB_FATAL) << "Please, specify knnQueryQty > 0";
    if (knn <= 0)         LOG(LIB_FATAL) << "Please, specify knn > 0";
    if (distType == DIST_TYPE_FLOAT) {
      sampleDist<float>(spaceType, inFile, outFile,
                        knn, 
                        maxNumData,
                        knnQueryQty);
    } else if (distType == DIST_TYPE_DOUBLE) {
      sampleDist<double>(spaceType, inFile, outFile,
                         knn, 
                         maxNumData,
                         knnQueryQty);
    } else {
      LOG(LIB_FATAL) << "Unsupported distance type: '" << distType << "'";
    }
  } catch (const exception& e) {
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }

  return 0;
}
