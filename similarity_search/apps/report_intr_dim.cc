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
#include <iostream>
#include <string>

#include "space.h"
#include "params.h"
#include "init.h"
#include "report_intr_dim.h"
#include "spacefactory.h"
#include "cmd_options.h"
#include "my_isnan_isinf.h"

using namespace similarity;
using namespace std;

const unsigned defaultSampleQty = 1000000;

template <typename dist_t>
void ComputeMuDeffect(const Space<dist_t>& space,
                    const ObjectVector& dataset,
                    double & dleft, double & dright,
                    size_t SampleQty = 1000000) {
  dleft = dright = -1;
  for (size_t n = 0; n < SampleQty; ++n) {
    size_t r1 = RandomInt() % dataset.size();
    size_t r2 = RandomInt() % dataset.size();
    size_t r3 = RandomInt() % dataset.size();
    CHECK(r1 < dataset.size());
    CHECK(r2 < dataset.size());
    CHECK(r3 < dataset.size());
    const Object* q = dataset[r1];
    const Object* a = dataset[r2];
    const Object* b = dataset[r3];
    {
      dist_t d1 = space.IndexTimeDistance(q, a);
      dist_t d2 = space.IndexTimeDistance(q, b);
      dist_t d3 = space.IndexTimeDistance(a, b);
      if (my_isnan(d1) || my_isnan(d2) || my_isnan(d3)) {
        throw runtime_error("!!! Bug: a distance returned NAN!");
      }
      if (d3 != 0) {
        dright = max(dright, (double)fabs(double(d1)-double(d2))/double(d3));
      }
    }
    {
      dist_t d1 = space.IndexTimeDistance(a, q);
      dist_t d2 = space.IndexTimeDistance(b, q);
      dist_t d3 = space.IndexTimeDistance(b, a);
      if (my_isnan(d1) || my_isnan(d2) || my_isnan(d3)) {
        throw runtime_error("!!! Bug: a distance returned NAN!");
      }
      if (d3 != 0) {
        dleft = max(dleft, (double)fabs(double(d1)-double(d2))/double(d3));
      }
    }
  }
}

template <typename dist_t>
void TestSpace(
                string spaceDesc,
                string dataFile,
                bool compMuDeffect,
                unsigned maxNumData,
                unsigned sampleQty,
                string sampleFile
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
  vector<double>  dist;

  // Prints the report
  ReportIntrinsicDimensionality("********", *space, data, dist, sampleQty);
  if (!sampleFile.empty()) {
    ofstream of(sampleFile);
    CHECK_MSG(of, "Cannot open for writing file: " + sampleFile);
    of.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  
    for (size_t i = 0; i < dist.size(); ++i) {
      if (i) of << ",";
      of << dist[i];
    } 
    of << std::endl;
  }
  if (compMuDeffect) {
    double    dleft, dright;
    ComputeMuDeffect<dist_t>(
                  *space,
                  data,
                  dleft, dright,
                  sampleQty
                 );
    LOG(LIB_INFO) << "### left mu-defect. : " << dleft << " right mu-defect. :" << dright;
  }
}

int main(int argc, char* argv[]) {
  string    spaceDesc, distType;
  string    dataFile;
  string    sampleFile;
  unsigned  maxNumData;
  unsigned  sampleQty;
  bool      compMuDeffect;

  CmdOptions cmd_options;

  cmd_options.Add(new CmdParam("spaceType,s", "space type, e.g., l1, l2, lp:p=0.5",
                               &spaceDesc, true));
  cmd_options.Add(new CmdParam("distType", "distance value type: int, float, double",
                               &distType, false, DIST_TYPE_FLOAT));
  cmd_options.Add(new CmdParam("dataFile,i", "input data file",
                               &dataFile, true));
  cmd_options.Add(new CmdParam("maxNumData", "if non-zero, only the first maxNumData elements are used",
                               &maxNumData, false, 0));
  cmd_options.Add(new CmdParam("sampleQty", "a number of samples (a sample is a pair of data points)",
                               &sampleQty, false, defaultSampleQty));
  cmd_options.Add(new CmdParam("muDeffect,m", "estimate the left and the right mu deffectiveness",
                               &compMuDeffect, false, false));
  cmd_options.Add(new CmdParam("sampleFile", "optional output sample file",
                               &sampleFile, false, ""));

  try {
    cmd_options.Parse(argc, argv);

    if (!DoesFileExist(dataFile)) {
      PREPARE_RUNTIME_ERR(err) << "data file " << dataFile << " doesn't exist";
      THROW_RUNTIME_ERR(err);
    }

    initLibrary(0, LIB_LOGSTDERR);

    if (DIST_TYPE_INT == distType) {
      TestSpace<int>(
                  spaceDesc,
                  dataFile,
                  compMuDeffect,
                  maxNumData,
                  sampleQty,
                  sampleFile
                );
    } else if (DIST_TYPE_FLOAT == distType) {
      TestSpace<float>(
                  spaceDesc,
                  dataFile,
                  compMuDeffect,
                  maxNumData,
                  sampleQty,
                  sampleFile
                 );
    } else if (DIST_TYPE_DOUBLE == distType) {
      TestSpace<double>(
                  spaceDesc,
                  dataFile,
                  compMuDeffect,
                  maxNumData,
                  sampleQty,
                  sampleFile
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
