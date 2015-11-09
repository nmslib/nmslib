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
#include <string>
#include <fstream>

#include <boost/program_options.hpp>

#include "knnquery.h"
#include "knnqueue.h"
#include "space.h"
#include "params.h"
#include "projection.h"
#include "spacefactory.h"
#include "init.h"

using namespace similarity;
using namespace std;

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

template <class dist_t>
void benchProjection(size_t repeatQty,
                     string spaceType,
                     string inFile, string outFile,
                     string projType, unsigned knn, string projSpaceType,
                     unsigned nIntermDim,
                     unsigned nDstDim,
                     unsigned binThreshold,
                     unsigned maxNumData,
                     unsigned sampleRandPairQty, unsigned sampleKNNQueryQty, unsigned sampleKNNTotalQty) {
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

  ToLower(projSpaceType);
  vector<string> projSpaceDesc;

  const string projDescStr = projSpaceType;
  ParseSpaceArg(projDescStr, projSpaceType, projSpaceDesc);
  unique_ptr<AnyParams> projSpaceParams =
            unique_ptr<AnyParams>(new AnyParams(projSpaceDesc));

  unique_ptr<Space<float>> projSpace(SpaceFactoryRegistry<float>::
                                Instance().CreateSpace(projSpaceType, *projSpaceParams));

  if (NULL == projSpace.get()) {
    LOG(LIB_FATAL) << "Cannot create the projection space: '" << projSpaceType
                   << "' (desc: '" << projDescStr << "')";
  }

  const VectorSpaceSimpleStorage<float>*  ps =
      dynamic_cast<const VectorSpaceSimpleStorage<float>*>(projSpace.get());

  if (NULL == ps) {
    LOG(LIB_FATAL) << "The target projection space: '" << projDescStr << "' "
                   << " should be a simple-storage dense vector space, e.g., l2";
  }

  ObjectVector      data;
  vector<string>    tmp;
  LOG(LIB_INFO) << "maxNumData=" << maxNumData;
  space->ReadDataset(data, tmp, inFile, maxNumData);

  size_t N = data.size();


  unique_ptr<Projection<dist_t> > projObj;

  ofstream out(outFile);

  LOG(LIB_INFO) << "sampleRandPairQty=" << sampleRandPairQty;
  LOG(LIB_INFO) << "sampleKNNQueryQty=" << sampleKNNQueryQty;
  LOG(LIB_INFO) << "sampleKNNTotalQty=" << sampleKNNTotalQty;
  LOG(LIB_INFO) << "recreating projections #times=" << repeatQty;

  if (N > 0) 
  for (size_t rr = 0; rr < repeatQty; ++rr) {
    LOG(LIB_INFO) << "Creating projection object set " << (rr + 1) << " out of " << repeatQty;
    projObj.reset(
      Projection<dist_t>::createProjection(
                        *space,
                        data,
                        projType,
                        nIntermDim,
                        nDstDim,
                        binThreshold));

    vector<float>   v1(nDstDim), v2(nDstDim);

    vector<IdType>  vId1, vId2;
    vector<dist_t>  vOrigDist;

    for (size_t i = 0; i < sampleRandPairQty; ++i) {
      IdType id1 = RandomInt() % N;
      IdType id2 = RandomInt() % N;
      vId1.push_back(id1);
      vId2.push_back(id2);
      vOrigDist.push_back(space->IndexTimeDistance(data[id1], data[id2]));
    }

    CHECK(vId1.size() == vId2.size());
    CHECK(vId1.size() == vOrigDist.size());
  

    size_t iter = 0;
    size_t startId = vOrigDist.size();

    for (size_t i = 0; i < sampleKNNQueryQty; ++i) {
      IdType id1 = RandomInt() % N;

      KNNQuery<dist_t> query(*space, data[id1], knn);

      // Brute force search
      for (size_t i = 0; i < N; ++i) {
        query.CheckAndAddToResult(data[i]);
      }

      unique_ptr<KNNQueue<dist_t>> knnQ(query.Result()->Clone());

      // Extracting results
      while (!knnQ->Empty()) {
        // Reservoir sampling
        int selectIndex = -1;
        ++iter;
        if (iter > sampleKNNTotalQty) {
          selectIndex = RandomInt() % iter; // from 0 to iter
        } else {
          vOrigDist.push_back(-1);
          vId1.push_back(-1);
          vId2.push_back(-1);
          selectIndex = vOrigDist.size() - 1 - startId;
        }

        if (selectIndex >= 0 && selectIndex < vOrigDist.size() - startId) {
          size_t replIndex = selectIndex + startId;
          vOrigDist[replIndex] = knnQ->TopDistance(); 
          vId1[replIndex] = id1;
          vId2[replIndex] = knnQ->TopObject()->id();
        }

        knnQ->Pop(); 
      }
    }

    for (size_t i = 0; i < vOrigDist.size(); ++i) {
      projObj->compProj(NULL, data[vId1[i]], &v1[0]);
      projObj->compProj(NULL, data[vId2[i]], &v2[0]);

      CHECK(vId1[i] >= 0);
      CHECK(vId2[i] >= 0);

      unique_ptr<Object> obj1(ps->CreateObjFromVect(-1, -1, v1));
      unique_ptr<Object> obj2(ps->CreateObjFromVect(-1, -1, v2));

      float projDist = ps->IndexTimeDistance(obj1.get(), obj2.get());

      out << vOrigDist[i] << "\t" << projDist << endl;
    }
  }
}

int main(int argc, char *argv[]) {
  string      spaceType, distType, projSpaceType;
  string      inFile, outFile;
  string      projType;
  string      logFile;
  unsigned    maxNumData = 0;
  unsigned    sampleRandPairQty = 0;
  unsigned    sampleKNNQueryQty = 0;
  unsigned    sampleKNNTotalQty = 0;
  unsigned    nIntermDim   = 0;
  unsigned    binThreshold = 0;
  unsigned    nDstDim;
  unsigned    knn = 0;
  unsigned    repeatQty = 1;


  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("spaceType,s",     po::value<string>(&spaceType)->required(),
                        "space type, e.g., l1, l2, lp:p=0.5")
    ("projSpaceType",   po::value<string>(&projSpaceType)->default_value("l2"),
                        "space type in the projection space, e.g., l1, l2, lp:p=0.5. "
                        "should be a dense vector space!")
    ("distType",        po::value<string>(&distType)->default_value(DIST_TYPE_FLOAT),
                        "distance value type: float, double")
    ("inFile,i",        po::value<string>(&inFile)->required(),
                        "input data file")
    ("outFile,o",       po::value<string>(&outFile)->required(),
                        "output data file")
    ("projType,p",      po::value<string>(&projType)->required(),
                        "projection type")
    ("sampleRandPairQty",po::value<unsigned>(&sampleRandPairQty)->default_value(0),
                        "number of randomly selected pairs")
    ("sampleKNNQueryQty",po::value<unsigned>(&sampleKNNQueryQty)->default_value(0),
                        "number of randomly selected queries")
    ("sampleKNNTotalQty",po::value<unsigned>(&sampleKNNTotalQty)->default_value(0),
                        "a total number of randomly selected queries' nearest neighbors (should be >= sampleKNNQueryQty)")
    ("knn,k",           po::value<unsigned>(&knn)->default_value(0),
                        "use this number of nearest neighbors (should be > 0 if sampleKNNQueryQty > 0)")
    ("repeat,r",        po::value<unsigned>(&repeatQty)->default_value(10),
                        "recreate projections this number of times")
    ("intermDim",       po::value<unsigned>(&nIntermDim)->default_value(0),
                        "intermediate dimensionality, used only for sparse vector spaces")
    ("projDim",          po::value<unsigned>(&nDstDim)->required(),
                        "dimensionality in the target space (where we project to)")
    ("binThreshold",    po::value<unsigned>(&binThreshold)->default_value(0),
                        "binarization threshold, used only for permutations")
    ("maxNumData",      po::value<unsigned>(&maxNumData)->default_value(0),
                        "if non-zero, only the first maxNumData elements are used")
    ("logFile,l",       po::value<string>(&logFile)->default_value(""),
                        "log file")
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

  initLibrary(logFile.empty() ? LIB_LOGSTDERR:LIB_LOGFILE, logFile.c_str());

  LOG(LIB_INFO) << "Program arguments are processed";
  

  ToLower(distType);


  try {
    if (sampleKNNQueryQty) {
      if (!knn) LOG(LIB_FATAL) << "Please, specify knn > 0";
      if (sampleKNNTotalQty < sampleKNNQueryQty) {
        LOG(LIB_FATAL) << "sampleKNNTotalQty should be at least as large as sampleKNNQueryQty";
      }
    }
    if (distType == DIST_TYPE_FLOAT) {
      benchProjection<float>(repeatQty, spaceType, inFile, outFile,
                             projType, knn, projSpaceType,
                             nIntermDim, nDstDim, binThreshold,
                             maxNumData,
                             sampleRandPairQty, sampleKNNQueryQty, sampleKNNTotalQty);
    } else if (distType == DIST_TYPE_DOUBLE) {
      benchProjection<double>(repeatQty, spaceType, inFile, outFile,
                             projType, knn, projSpaceType,
                             nIntermDim, nDstDim, binThreshold,
                             maxNumData,
                             sampleRandPairQty, sampleKNNQueryQty, sampleKNNTotalQty);
    } else {
      LOG(LIB_FATAL) << "Unsupported distance type: '" << distType << "'";
    }
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }

    

  return 0;
};
