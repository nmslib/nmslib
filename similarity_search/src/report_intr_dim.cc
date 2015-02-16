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

#include <boost/program_options.hpp>

#include "space.h"
#include "params.h"
#include "init.h"
#include "spacefactory.h"

using namespace similarity;
using namespace std;

namespace po = boost::program_options;

const unsigned defaultSampleQty = 1000000;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    std::cout << prog << std::endl
              << desc << std::endl;
}

template <typename dist_t>
void TestSpace(
                string spaceDesc,
                string dataFile,
                unsigned maxNumData,
                unsigned sampleQty
               ) {
  string          spaceType;
  vector<string>  vSpaceArgs;

  ParseSpaceArg(spaceDesc, spaceType, vSpaceArgs);
  AnyParams   spaceParams({AnyParams(vSpaceArgs)});

  unique_ptr<Space<dist_t>> space(SpaceFactoryRegistry<dist_t>::
                                Instance().CreateSpace(spaceType, spaceParams));

  ObjectVector data;
  space->ReadDataset(data, NULL, dataFile.c_str(), maxNumData);

  // Prints the report
  ReportIntrinsicDimensionality("********", *space, data, sampleQty);
}

int main(int argc, char* argv[]) {
  string    spaceDesc, distType;
  string    dataFile;
  unsigned  maxNumData;
  unsigned  sampleQty;

  po::options_description ProgOptDesc("Allowed options");
  ProgOptDesc.add_options()
    ("help,h", "produce help message")
    ("spaceType,s",     po::value<string>(&spaceDesc)->required(),
                        "space type, e.g., l1, l2, lp:p=0.5")
    ("distType",        po::value<string>(&distType)->default_value(DIST_TYPE_FLOAT),
                        "distance value type: int, float, double")
    ("dataFile,i",      po::value<string>(&dataFile)->required(),
                        "input data file")
    ("maxNumData",      po::value<unsigned>(&maxNumData)->default_value(0),
                        "if non-zero, only the first maxNumData elements are used")
    ("sampleQty",       po::value<unsigned>(&sampleQty)->default_value(defaultSampleQty),
                        "a number of samples (a sample is a pair of data points)")
  ;

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);

    if (!DoesFileExist(dataFile)) {
      LOG(LIB_FATAL) << "data file " << dataFile << " doesn't exist";
    }

    initLibrary(LIB_LOGSTDERR);

    if (DIST_TYPE_INT == distType) {
      TestSpace<int>(
                  spaceDesc,
                  dataFile,
                  maxNumData,
                  sampleQty
                 );
    } else if (DIST_TYPE_FLOAT == distType) {
      TestSpace<float>(
                  spaceDesc,
                  dataFile,
                  maxNumData,
                  sampleQty
                 );
    } else if (DIST_TYPE_DOUBLE == distType) {
      TestSpace<double>(
                  spaceDesc,
                  dataFile,
                  maxNumData,
                  sampleQty
                 );
    }

  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    LOG(LIB_FATAL) << e.what();
  }

  return 0;
}
