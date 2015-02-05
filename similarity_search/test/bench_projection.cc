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
void benchProjection(string spaceType,
                     string inFile, string outFile,
                     string projType, string projSpaceType,
                     unsigned nIntermDim,
                     unsigned nDstDim,
                     unsigned maxNumData,
                     unsigned sampleQty) {
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

  unique_ptr<Space<float>> projSpace(SpaceFactoryRegistry<dist_t>::
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

  ObjectVector data;
  space->ReadDataset(data, NULL, inFile.c_str(), maxNumData);

  size_t N = data.size();

  unique_ptr<Projection<dist_t> > projObj(
      Projection<dist_t>::createProjection(
                        space.get(),
                        data,
                        projType,
                        nIntermDim,
                        nDstDim));

  ofstream out(outFile);

  if (N > 0) {
    vector<float> v1(nDstDim), v2(nDstDim);

    for (size_t i = 0; i < sampleQty; ++i) {
      IdType id1 = RandomInt() % N;
      IdType id2 = RandomInt() % N;

      dist_t origDist = space->IndexTimeDistance(data[id1], data[id2]);

      projObj->compProj(NULL, data[id1], &v1[0]);
      projObj->compProj(NULL, data[id2], &v2[0]);

      unique_ptr<Object> obj1(ps->CreateObjFromVect(-1, -1, v1));
      unique_ptr<Object> obj2(ps->CreateObjFromVect(-1, -1, v2));

      float projDist = ps->IndexTimeDistance(obj1.get(), obj2.get());

      out << origDist << "\t" << projDist << endl;
    }
  }
}

int main(int argc, char *argv[]) {
  string      spaceType, distType, projSpaceType;
  string      inFile, outFile;
  string      projType;
  string      logFile;
  unsigned    maxNumData;
  unsigned    sampleQty;
  unsigned    nIntermDim = 0;
  unsigned    nDstDim;


  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("spaceType,s",     po::value<string>(&spaceType)->required(),
                        "space type, e.g., l1, l2, lp:p=0.5")
    ("projSpaceType",   po::value<string>(&projSpaceType)->default_value("l2"),
                        "space type in the projection space, e.g., l1, l2, lp:p=0.5. "
                        "should be a dense vector space!")
    ("distType",        po::value<string>(&distType)->default_value("float"),
                        "distance value type: int, float, double")
    ("inFile,i",        po::value<string>(&inFile)->required(),
                        "input data file")
    ("outFile,o",       po::value<string>(&outFile)->required(),
                        "output data file")
    ("projType,p",      po::value<string>(&projType)->required(),
                        "projection type")
    ("sampleQty,q",     po::value<unsigned>(&sampleQty)->required(),
                        "number of samples")
    ("intermDim",       po::value<unsigned>(&nIntermDim)->default_value(0),
                        "intermediate dimensionality, used only for sparse vector spaces")
    ("projDim",          po::value<unsigned>(&nDstDim)->required(),
                        "dimensionality in the target space (where we project to)")
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
    if (distType == "float") {
      benchProjection<float>(spaceType, inFile, outFile,
                             projType, projSpaceType,
                             nIntermDim, nDstDim,
                             maxNumData,
                             sampleQty);
    } else if (distType == "double") {
      benchProjection<float>(spaceType, inFile, outFile,
                             projType, projSpaceType,
                             nIntermDim, nDstDim,
                             maxNumData,
                             sampleQty);
    } else {
      LOG(LIB_FATAL) << "Unsupported distance type: '" << distType << "'";
    }
  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << "Exception: " << e.what();
  }

    

  return 0;
};
