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

#include <string>
#include <iostream>
#include <fstream>

#include "space/space_sparse_bin_common.h"
#include "space/space_sparse_scalar_bin_fast.h"

using namespace std;
using namespace similarity;

int main(int argc, char *argv[]) {
  string        inputFileName, outputFileName;
  unsigned      maxRecQty = 0;

  CmdOptions    cmdOptions;

  cmdOptions.Add(new CmdParam("input", "input file (specify - for standard input)", &inputFileName, true));
  cmdOptions.Add(new CmdParam("output", "output file", &outputFileName, true));
  cmdOptions.Add(new CmdParam("maxRecQty", "maximum # of records to convert (or zero to convert all)", &maxRecQty, false, 0));

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
    unique_ptr<ifstream> inpFile;
    istream* pInpText = &cin;

    cout << "Reading binary data from: " << inputFileName << endl;

    DataFileInputStateBinSparseVec inpBinFile(inputFileName);

    string lineText, lineBin;

    vector<SparseVectElem<float>> vText, vBin;

    size_t recQty = 0;

    ofstream outFile(outputFileName);
    outFile.exceptions(ios::badbit | ios::failbit);

    string colon = ":";
    string space = " ";

    for (; recQty < inpBinFile.qty_; ) {
      if (!readNextBinSparseVect(inpBinFile, lineBin)) {
        PREPARE_RUNTIME_ERR(err) << "Cannot retrieve binary entry # " << recQty <<
                                 " although the corresponding text entry does exist";
        THROW_RUNTIME_ERR(err);
      }

      parseSparseBinVector(lineBin, vBin);

      string line;

      for (size_t i = 0; i < vBin.size(); ++i) {
        if (i) line.append(space);
        const auto& e = vBin[i];
        line.append(ConvertToString(e.id_));
        line.append(colon);
        line.append(ConvertToString(e.val_));
      }

      outFile << line << endl;

      if (++recQty >= maxRecQty && maxRecQty)
        break;

    }

    LOG(LIB_INFO) << "Converted " << recQty << " entries";

    outFile.close();

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception caught";
  }


  return 0;
}
