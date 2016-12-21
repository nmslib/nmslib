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
#include "space_sparse_jaccard.h"
#include "space_sparse_vector_inter.h"
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
                string inFile, string pivotFile, unsigned maxNumPivots, string outFilePrefix,
                unsigned knn, 
                unsigned maxNumData,
                unsigned knnQueryQty) {
  ToLower(spaceType);
  vector<string> spaceDesc;

  const string descStr = spaceType;
  ParseSpaceArg(descStr, spaceType, spaceDesc);
  unique_ptr<AnyParams> spaceParams =
            unique_ptr<AnyParams>(new AnyParams(spaceDesc));

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::Instance().CreateSpace(spaceType, *spaceParams));

  SpaceSparseJaccard<dist_t>*       pJaccardSpace = dynamic_cast<SpaceSparseJaccard<dist_t>*>(space.get());
  SpaceSparseVectorInter<dist_t>*   pInterSpace   = dynamic_cast<SpaceSparseVectorInter<dist_t>*>(space.get()); 

  if (pJaccardSpace) 
    LOG(LIB_INFO) << "Sparse Jaccard space detected!";
  if (pInterSpace)
    LOG(LIB_INFO) << "Special sparse vector space detected!";

  if (NULL == space.get()) {
    LOG(LIB_FATAL) << "Cannot create the space: '" << spaceType
                   << "' (desc: '" << descStr << "')";
  } else {
    LOG(LIB_INFO) << "Created space: " << spaceType;
  }

  ObjectVector      data;
  ObjectVector      pivots;

  LOG(LIB_INFO) << "maxNumData=" << maxNumData;
  {
    vector<string>    tmp;
    unique_ptr<DataFileInputState> inpState(space->ReadDataset(data, tmp, inFile, maxNumData));
    space->UpdateParamsFromFile(*inpState);
  }
  if (!pivotFile.empty()) {
    vector<string>    tmp;
    unique_ptr<DataFileInputState> inpState(space->ReadDataset(pivots, tmp, pivotFile, maxNumPivots));
    space->UpdateParamsFromFile(*inpState);
    LOG(LIB_INFO) << "Read " << pivots.size() << " pivots";
  }

  size_t N = data.size();

  vector<char>      isQuery(N);

  string    outFilePiv = outFilePrefix + "_pivots.tsv";
  ofstream  outPiv(outFilePiv);

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

  size_t pivotQty = pivots.size();

  vector<vector<dist_t>>  outNNDistMatrix(knn);
  vector<vector<unsigned>> outNNOverlapMatrix(knn);

  vector<vector<dist_t>> outPivDistMatrix(pivotQty);

  vector<vector<unsigned>> outPivOverlapQtyMatrix(pivotQty);
  vector<vector<float>>    outPivOverlapFracMatrix(pivotQty);

  for (size_t qid = 0; qid < knnQueryQty; ++qid) {
    LOG(LIB_INFO) << "query index : " << qid << " id: " << queries[qid]->id();
    KNNQuery<dist_t> query(*space, queries[qid], knn);

    vector<dist_t> pivDist(pivotQty);
    for (size_t pid = 0; pid < pivotQty; ++pid) {
      pivDist[pid]=space->IndexTimeDistance(pivots[pid], queries[qid]); // Pivot plays the role of an object => it's a left argument
    }
    sort(pivDist.begin(),pivDist.end());
    for (size_t pid = 0; pid < pivotQty; ++pid) {
      outPivDistMatrix[pid].push_back(pivDist[pid]);
    }
    if (pJaccardSpace || pInterSpace) {
      vector<unsigned> pivOverlap(pivotQty);
      for (size_t pid = 0; pid < pivotQty; ++pid) {
        pivOverlap[pid]=pJaccardSpace ? 
                          pJaccardSpace->ComputeOverlap(pivots[pid], queries[qid]):
                          pInterSpace->ComputeOverlap(pivots[pid], queries[qid]); 
      }
      sort(pivOverlap.begin(),pivOverlap.end(), [](unsigned qty1, unsigned qty2)->bool{ return qty1 > qty2;});
      float elemQty = pJaccardSpace ? pJaccardSpace->GetElemQty(queries[qid]) : 
                                      pInterSpace->GetElemQty(queries[qid]);
      float elemQtyInv = 1.0f/elemQty;
      for (size_t pid = 0; pid < pivotQty; ++pid) {
        outPivOverlapQtyMatrix[pid].push_back(pivOverlap[pid]);
        outPivOverlapFracMatrix[pid].push_back(pivOverlap[pid]*elemQtyInv);
      }
    }

    // Brute force search
    for (size_t k = 0; k < N; ++k) 
    if (!isQuery[k]) {
      query.CheckAndAddToResult(data[k]);
    }

    unique_ptr<KNNQueue<dist_t>> knnQ(query.Result()->Clone());


    vector<pair<dist_t,unsigned>> dists_overlaps;
    dists_overlaps.reserve(knn);

    // Extracting results
    while (!knnQ->Empty()) {
      unsigned overlap = 0;
      if (pJaccardSpace || pInterSpace) {
        overlap = pJaccardSpace ? 
                          pJaccardSpace->ComputeOverlap(knnQ->TopObject(), queries[qid]):
                          pInterSpace->ComputeOverlap(knnQ->TopObject(), queries[qid]); 
      }
      dists_overlaps.insert(dists_overlaps.begin(),make_pair(knnQ->TopDistance(),overlap));
      knnQ->Pop();
    }
    for (size_t k = 0; k < min<size_t>(dists_overlaps.size(), knn); ++k) {
      outNNDistMatrix[k].push_back(dists_overlaps[k].first);
      outNNOverlapMatrix[k].push_back(dists_overlaps[k].second);
    }
  }

  string    outFileDistNN = outFilePrefix + "_dist_NN.tsv";
  ofstream  outDistNN(outFileDistNN);

  for (size_t i = 0; i < outNNDistMatrix.size(); ++i) {
    const vector<dist_t>& dists2 = outNNDistMatrix[i];
    if (!dists2.empty()) outDistNN << dists2[0];
    for (size_t k = 1; k < dists2.size(); ++k) {
      outDistNN << "\t" << dists2[k];
    }
    outDistNN << endl;
  }
  outDistNN.close();

  if (pJaccardSpace || pInterSpace) {
    string    outFileOverlapNN = outFilePrefix + "_overlap_qty_NN.tsv";
    ofstream  outOverlapNN(outFileOverlapNN);

    for (size_t i = 0; i < outNNOverlapMatrix.size(); ++i) {
      const vector<unsigned>& overlaps = outNNOverlapMatrix[i];
      if (!overlaps.empty()) outOverlapNN << overlaps[0];
      for (size_t k = 1; k < overlaps.size(); ++k) {
        outOverlapNN << "\t" << overlaps[k];
      }
      outOverlapNN << endl;
    }
    outOverlapNN.close();
  }

  for (size_t i = 0; i < pivotQty; ++i) {
    const vector<dist_t>& dists3= outPivDistMatrix[i];
    if (!dists3.empty()) outPiv << dists3[0];
    for (size_t k = 1; k < dists3.size(); ++k) {
      outPiv << "\t" << dists3[k];
    }
    outPiv << endl;
  }

  outPiv.close();

  if (pJaccardSpace || pInterSpace) {
    string outFileOverlapQty  = outFilePrefix + "_overlap_qty.tsv";
    string outFileOverlapFrac = outFilePrefix + "_overlap_frac.tsv";

    ofstream  outOverlapQty(outFileOverlapQty);
    ofstream  outOverlapFrac(outFileOverlapFrac);

    for (size_t i = 0; i < pivotQty; ++i) {
      const vector<unsigned>& qtys = outPivOverlapQtyMatrix[i];
      if (!qtys.empty()) outOverlapQty << qtys[0];
      for (size_t k = 1; k < qtys.size(); ++k) {
        outOverlapQty << "\t" << qtys[k];
      }
      outOverlapQty << endl;

      const vector<float>& fracs = outPivOverlapFracMatrix[i];
      if (!fracs.empty()) outOverlapFrac << fracs[0];
      for (size_t k = 1; k < fracs.size(); ++k) {
        outOverlapFrac << "\t" << fracs[k];
      }
      outOverlapFrac << endl;
    }
    outOverlapQty.close();
    outOverlapFrac.close();
  }

}

int main(int argc, char *argv[]) {
  string      spaceType, distType, projSpaceType;
  string      inFile, pivotFile, outFilePrefix;
  string      projType;
  string      logFile;
  unsigned    maxNumData = 0;
  unsigned    maxNumPivots = 0;
  unsigned    knnQueryQty = 0;
  unsigned    knn = 0;


  CmdOptions cmd_options;


  cmd_options.Add(new CmdParam(SPACE_TYPE_PARAM_OPT, SPACE_TYPE_PARAM_MSG,
                               &spaceType, true));
  cmd_options.Add(new CmdParam(DIST_TYPE_PARAM_OPT, DIST_TYPE_PARAM_MSG,
                               &distType, false, std::string(DIST_TYPE_FLOAT)));
  cmd_options.Add(new CmdParam("inFile,i", "input data file",
                               &inFile, true));
  cmd_options.Add(new CmdParam("outFilePrefix,o", "output file prefix",
                               &outFilePrefix, true));
  cmd_options.Add(new CmdParam("pivotFile,p", "pivot file",
                               &pivotFile, false, ""));
  cmd_options.Add(new CmdParam("maxNumPivot", "maximum number of pivots to use",
                               &maxNumPivots, false, 0));
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
      sampleDist<float>(spaceType, 
                        inFile, pivotFile, maxNumPivots, outFilePrefix,
                        knn, 
                        maxNumData,
                        knnQueryQty);
    } else if (distType == DIST_TYPE_DOUBLE) {
      sampleDist<double>(spaceType, 
                        inFile, pivotFile, maxNumPivots, outFilePrefix,
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
