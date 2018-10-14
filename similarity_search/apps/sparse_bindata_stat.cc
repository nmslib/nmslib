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
#include "params.h"
#include "params_cmdline.h"
#include "cmd_options.h"
#include "read_data.h"
#include "utils.h"

#include "space/space_sparse_bin_common.h"

#include "space/space_sparse_scalar_bin_fast.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace similarity;

const unsigned REPORT_QTY = 100000;

int main(int argc, char *argv[]) {
  string        repFile, inpBinFileName;

  CmdOptions    cmdOptions;

  cmdOptions.Add(new CmdParam("repFile,r", "output report file",
                               &repFile, true));
  cmdOptions.Add(new CmdParam("input_binary", "binary file", &inpBinFileName, true));

  try {
    cmdOptions.Parse(argc, argv);
  } catch (const CmdParserException& e) {
    cmdOptions.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (const std::exception& e) {
    cmdOptions.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    cmdOptions.ToString();
    std::cout.flush();
    LOG(LIB_FATAL) << "Failed to parse cmd arguments";
  }

  LOG(LIB_INFO) << "Program arguments are processed";

  try {
    cout << "Reading binary data from: " << inpBinFileName << endl;

    DataFileInputStateBinSparseVec inpBinFile(inpBinFileName);

    string lineBin;

    vector<SparseVectElem<float>> vBin;

    size_t lineNum = 0;
    size_t maxDim = 0;
    size_t nonZeroQty = 0;
    for (;lineNum < inpBinFile.qty_; lineNum++) {
      if (!readNextBinSparseVect(inpBinFile, lineBin)) {
        PREPARE_RUNTIME_ERR(err) << "Cannot retrieve binary entry # " << lineNum <<
                                 " although the corresponding text entry does exist";
        THROW_RUNTIME_ERR(err);
      }

      parseSparseBinVector(lineBin, vBin);

      for (auto e : vBin) {
        maxDim = max(maxDim, (size_t)e.id_);
        nonZeroQty++;
      }

      if (lineNum % REPORT_QTY == 0) {
        cout << lineNum << " lines checked" << endl;
      }
    }
    cout << lineNum << " lines checked" << endl;
    float avgPerVector = float(nonZeroQty) / max(size_t(1), inpBinFile.qty_);
    LOG(LIB_INFO) << "Total # of vectors: " << inpBinFile.qty_;
    LOG(LIB_INFO) << "Total # of non-zeros:" << nonZeroQty;
    LOG(LIB_INFO) << "Maximum # of dimensions:" << maxDim;
    LOG(LIB_INFO) << "Avg. # of non-zeros per vector: " << avgPerVector;

    ofstream out(repFile.c_str());
    out << "qty\tnonZeroQty\tmaxDim\tnonZeroPerVect" << endl;
    out << inpBinFile.qty_ << '\t' << nonZeroQty << '\t' << maxDim << '\t' << avgPerVector << endl;

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception caught";
  }

  LOG(LIB_INFO) << "Check succeeded!";


  return 0;
}