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
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>

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
struct RichOverlapStat {
  RichOverlapStat(dist_t dist, 
                  unsigned overlap_qty, unsigned overlap3way_qty,
                  float overlap_dotprod_norm, 
                  float overlap_mean_left, float overlap_std_left,
                  float diff_mean_left, float diff_std_left,
                  float overlap_mean_right, float overlap_std_right,
                  float diff_mean_right, float diff_std_right) : 
                    dist_(dist), 
                    overlap_qty_(overlap_qty), overlap3way_qty_(overlap3way_qty),
                    overlap_dotprod_norm_(overlap_dotprod_norm), 
                    overlap_mean_left_(overlap_mean_left), overlap_std_left_(overlap_std_left),
                    diff_mean_left_(diff_mean_left), diff_std_left_(diff_std_left), 
                    overlap_mean_right_(overlap_mean_right), overlap_std_right_(overlap_std_right), 
                    diff_mean_right_(diff_mean_right), diff_std_right_(diff_std_right) {}
  dist_t    dist_;
  uint32_t  overlap_qty_;
  uint32_t  overlap3way_qty_;
  float     overlap_dotprod_norm_;

  float     overlap_mean_left_;
  float     overlap_std_left_;
  float     diff_mean_left_ ;
  float     diff_std_left_ ;

  float     overlap_mean_right_;
  float     overlap_std_right_;
  float     diff_mean_right_;
  float     diff_std_right_;
};

template <typename elemType>
void outputMatrix(const string& fileName, const vector<vector<elemType>> & matr) {
  ofstream  out(fileName);

  for (size_t i = 0; i < matr.size(); ++i) {
    const auto matrRow = matr[i];
    if (!matrRow.empty()) out << matrRow[0];
    for (size_t k = 1; k < matrRow.size(); ++k) {
      out << "\t" << matrRow[k];
    }
    out << endl;
  }
  out.close();
}

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
    LOG(LIB_INFO) << "Read " << data.size() << " data points";
  }
  if (!pivotFile.empty()) {
    vector<string>    tmp;
    unique_ptr<DataFileInputState> inpState(space->ReadDataset(pivots, tmp, pivotFile, maxNumPivots));
    space->UpdateParamsFromFile(*inpState);
    LOG(LIB_INFO) << "Read " << pivots.size() << " pivots";
  }

  size_t N = data.size();

  vector<char>      isQuery(N);

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

  vector<vector<dist_t>>   outNNDistMatrix(knn);

  vector<vector<unsigned>> outNNOverlapQtyMatrix(knn);
  vector<vector<unsigned>> outNN3WayOverlapQtyMatrix(knn);
  vector<vector<float>>    outNNOverlapDotprodNormMatrix(knn);

  vector<vector<float>>    outNNOverlapMeanLeftMatrix(knn);
  vector<vector<float>>    outNNOverlapSTDLeftMatrix(knn);
  vector<vector<float>>    outNNDiffMeanLeftMatrix(knn);
  vector<vector<float>>    outNNDiffSTDLeftMatrix(knn);

  vector<vector<float>>    outNNOverlapMeanRightMatrix(knn);
  vector<vector<float>>    outNNOverlapSTDRightMatrix(knn);
  vector<vector<float>>    outNNDiffMeanRightMatrix(knn);
  vector<vector<float>>    outNNDiffSTDRightMatrix(knn);

  vector<vector<dist_t>>   outPivDistMatrix(pivotQty);

  vector<vector<unsigned>> outPivOverlapQtyMatrix(pivotQty);
  vector<vector<float>>    outPivOverlapFracMatrix(pivotQty);

  queue<size_t>   qidQueue; 
  vector<thread>  qThreads; 
  mutex         mQueue;
  mutex         mOut;

  for (size_t qid = 0; qid < knnQueryQty; ++qid) {
    qidQueue.push(qid);
  }


  for (unsigned tid = 0; tid < thread::hardware_concurrency(); ++tid) 
  qThreads.push_back(thread([&]() {
    while (true) {
      size_t qid = 0;
      {
        unique_lock<mutex> lock(mQueue);
        if (qidQueue.empty()) return;
        qid = qidQueue.front();
        qidQueue.pop(); 
        LOG(LIB_INFO) << "query index : " << qid << " id: " << queries[qid]->id();
      }
      KNNQuery<dist_t> query(*space, queries[qid], knn);

      vector<dist_t> pivDist(pivotQty);
      for (size_t pid = 0; pid < pivotQty; ++pid) {
        pivDist[pid]=space->IndexTimeDistance(pivots[pid], queries[qid]); // Pivot plays the role of an object => it's a left argument
      }
      sort(pivDist.begin(),pivDist.end());
      {
        unique_lock<mutex> lock(mOut);
        for (size_t pid = 0; pid < pivotQty; ++pid) {
         outPivDistMatrix[pid].push_back(pivDist[pid]);
        }
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
        {
          unique_lock<mutex> lock(mOut);
          for (size_t pid = 0; pid < pivotQty; ++pid) {
            outPivOverlapQtyMatrix[pid].push_back(pivOverlap[pid]);
            outPivOverlapFracMatrix[pid].push_back(pivOverlap[pid]*elemQtyInv);
          }
        }
      }

      // Brute force search
      for (size_t k = 0; k < N; ++k) 
      if (!isQuery[k]) {
        query.CheckAndAddToResult(data[k]);
      }

      unique_ptr<KNNQueue<dist_t>> knnQ(query.Result()->Clone());


      vector<RichOverlapStat<dist_t>> knn_overlap_stat;
      knn_overlap_stat.reserve(knn);

      // Extracting results
      while (!knnQ->Empty()) {
        OverlapInfo oinfo;

        size_t best3way_overlap_qty = 0;

        if (pJaccardSpace || pInterSpace) {
          if (pJaccardSpace) 
            oinfo.overlap_qty_ = pJaccardSpace->ComputeOverlap(knnQ->TopObject(), queries[qid]);
          if (pInterSpace) {
            oinfo = pInterSpace->ComputeOverlapInfo(knnQ->TopObject(), queries[qid]); 
          }

          for (size_t pid = 0; pid < pivotQty; ++pid) {
            uint32_t overlap3way_qty = pJaccardSpace ? 
                            pJaccardSpace->ComputeOverlap(knnQ->TopObject(), queries[qid], pivots[pid]):
                            pInterSpace->ComputeOverlap(knnQ->TopObject(), queries[qid], pivots[pid]); 
            if (overlap3way_qty > best3way_overlap_qty) best3way_overlap_qty = overlap3way_qty;
          }
        }
        knn_overlap_stat.insert(knn_overlap_stat.begin(),
                                RichOverlapStat<dist_t>(knnQ->TopDistance(), 
                                                        oinfo.overlap_qty_, best3way_overlap_qty,
                                                        oinfo.overlap_dotprod_norm_, 
                                                        oinfo.overlap_mean_left_,  oinfo.overlap_std_left_,  
                                                        oinfo.diff_mean_left_, oinfo.diff_std_left_,
                                                        oinfo.overlap_mean_right_, oinfo.overlap_std_right_, 
                                                        oinfo.diff_mean_right_, oinfo.diff_std_right_
        ));
        knnQ->Pop();
      }
      {
        unique_lock<mutex> lock(mOut);

        for (size_t k = 0; k < min<size_t>(knn_overlap_stat.size(), knn); ++k) {
          outNNDistMatrix[k].push_back(knn_overlap_stat[k].dist_);
          outNNOverlapQtyMatrix[k].push_back(knn_overlap_stat[k].overlap_qty_);
          outNN3WayOverlapQtyMatrix[k].push_back(knn_overlap_stat[k].overlap3way_qty_);

          outNNOverlapDotprodNormMatrix[k].push_back(knn_overlap_stat[k].overlap_dotprod_norm_);

          outNNOverlapMeanLeftMatrix[k].push_back(knn_overlap_stat[k].overlap_mean_left_);
          outNNOverlapSTDLeftMatrix[k].push_back(knn_overlap_stat[k].overlap_std_left_);
          outNNDiffMeanLeftMatrix[k].push_back(knn_overlap_stat[k].diff_mean_left_);
          outNNDiffSTDLeftMatrix[k].push_back(knn_overlap_stat[k].diff_std_left_);

          outNNOverlapMeanRightMatrix[k].push_back(knn_overlap_stat[k].overlap_mean_right_);
          outNNOverlapSTDRightMatrix[k].push_back(knn_overlap_stat[k].overlap_std_right_);
          outNNDiffMeanRightMatrix[k].push_back(knn_overlap_stat[k].diff_mean_right_);
          outNNDiffSTDRightMatrix[k].push_back(knn_overlap_stat[k].diff_std_right_);
        }
      }
    }
  }));

  for (auto& oneThread : qThreads) {
    oneThread.join();
  }

  outputMatrix(outFilePrefix + "_dist_NN.tsv", outNNDistMatrix);

  if (pJaccardSpace || pInterSpace) {
    outputMatrix(outFilePrefix + "_overlap_qty_NN.tsv", outNNOverlapQtyMatrix);
    outputMatrix(outFilePrefix + "_3way_overlap_qty_NN.tsv", outNN3WayOverlapQtyMatrix);
    if (pInterSpace) {
      outputMatrix(outFilePrefix + "_overlap_dotprod_norm_NN.tsv", outNNOverlapDotprodNormMatrix);

      outputMatrix(outFilePrefix + "_overlap_mean_left_NN.tsv",     outNNOverlapMeanLeftMatrix);
      outputMatrix(outFilePrefix + "_overlap_std_left_NN.tsv",      outNNOverlapSTDLeftMatrix);
      outputMatrix(outFilePrefix + "_diff_mean_left_NN.tsv",        outNNDiffMeanLeftMatrix);
      outputMatrix(outFilePrefix + "_diff_std_left_NN.tsv",         outNNDiffSTDLeftMatrix);

      outputMatrix(outFilePrefix + "_overlap_mean_right_NN.tsv",    outNNOverlapMeanRightMatrix);
      outputMatrix(outFilePrefix + "_overlap_std_right_NN.tsv",     outNNOverlapSTDRightMatrix);
      outputMatrix(outFilePrefix + "_diff_mean_right_NN.tsv",       outNNDiffMeanRightMatrix);
      outputMatrix(outFilePrefix + "_diff_std_right_NN.tsv",        outNNDiffSTDRightMatrix);
    }
    outputMatrix(outFilePrefix + "_overlap_qty_pivots.tsv", outPivOverlapQtyMatrix);
    outputMatrix(outFilePrefix + "_overlap_frac_pivots.tsv", outPivOverlapFracMatrix);
  }

  outputMatrix(outFilePrefix + "_dists_pivots.tsv", outPivDistMatrix);

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

  initLibrary(0, logFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, logFile.c_str());

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

  LOG(LIB_INFO) << "All is done!";

  return 0;
}
