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

using namespace std;
using namespace similarity;

int main(int argc, char *argv[]) {
  string        inputFileName, outputFileName;

  CmdOptions    cmdOptions;

  cmdOptions.Add(new CmdParam("input", "input file", &inputFileName, true));
  cmdOptions.Add(new CmdParam("output", "output file", &outputFileName, true));

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
    istream* pInp = &cin;

    if (inputFileName != "-") {
      inpFile.reset(new ifstream(inputFileName));
      pInp = inpFile.get();
      cout << "Reading data from: " << inputFileName << endl;
    } else {
      cout << "Reading data from standard input" << endl;
    }
    pInp->exceptions(ios::badbit);

    ofstream outFile(outputFileName);
    outFile.exceptions(ios::badbit | ios::failbit);

    string line;

    vector<SparseVectElem<float>> v;

    size_t lineNum = 0;

    size_t recQty = 0;

    writeBinaryPOD(outFile, recQty);

    while (getline(*pInp, line)) {
      lineNum++;
      if (line.empty())
        continue;
      if (!ReadSparseVecDataEfficiently(line, v)) {
        PREPARE_RUNTIME_ERR(err) << "Failed to parse the line # " << lineNum << ": '" << line << "'" << std::endl;
        LOG(LIB_ERROR) << err.stream().str();
        THROW_RUNTIME_ERR(err);
      }
      uint32_t qty = v.size();
      writeBinaryPOD(outFile, qty);

      for (uint32_t i = 0; i < qty; ++i) {
        writeBinaryPOD(outFile, v[i].id_);
        writeBinaryPOD(outFile, v[i].val_);
      }
      recQty++;
    }
    outFile.seekp(0, ios_base::beg);
    writeBinaryPOD(outFile, recQty);

    LOG(LIB_INFO) << "Converted " << recQty << " entries";

    outFile.close();

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception caught";
  }


  return 0;
}