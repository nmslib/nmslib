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

#include "space/space_sparse_scalar_bin_fast.h"

#include <string>
#include <iostream>
#include <fstream>

using namespace std;
using namespace similarity;

const unsigned REPORT_QTY = 100000;

int main(int argc, char *argv[]) {
  string        inpTextFileName, inpBinFileName;

  CmdOptions    cmdOptions;

  cmdOptions.Add(new CmdParam("input_text", "input file (specify - for standard input)", &inpTextFileName, true));
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
    unique_ptr<ifstream> inpFile;
    istream* pInpText = &cin;

    if (inpTextFileName != "-") {
      inpFile.reset(new ifstream(inpTextFileName));
      pInpText = inpFile.get();
      cout << "Reading text data from: " << inpTextFileName << endl;
    } else {
      cout << "Reading data from standard input" << endl;
    }
    pInpText->exceptions(ios::badbit);

    cout << "Reading binary data from: " << inpBinFileName << endl;

    DataFileInputStateBinSparseVec inpBinFile(inpBinFileName);

    string lineText, lineBin;

    vector<SparseVectElem<float>> vText, vBin;

    size_t lineNum = 0;

    while (getline(*pInpText, lineText)) {
      lineNum++;
      if (lineText.empty())
        continue;
      if (!ReadSparseVecDataEfficiently(lineText, vText)) {
        PREPARE_RUNTIME_ERR(err) << "Failed to parse the line # " << lineNum << ": '" << lineText << "'" << std::endl;
        LOG(LIB_ERROR) << err.stream().str();
        THROW_RUNTIME_ERR(err);
      }
      uint32_t qtyText = vText.size();

      for (uint32_t i = 1; i < qtyText; ++i) {
        if (vText[i].id_ <= vText[i-1].id_) {
          PREPARE_RUNTIME_ERR(err) << "Entries not sorted or have duplicates in line " << lineNum <<
                                      " first bad index: " << i;
          THROW_RUNTIME_ERR(err);
        }
      }

      if (!SpaceSparseCosineSimilarityBinFast::readNextBinSparseVect(inpBinFile, lineBin)) {
        PREPARE_RUNTIME_ERR(err) << "Cannot retrieve binary entry # " << lineNum <<
                                    " although the corresponding text entry does exist";
        THROW_RUNTIME_ERR(err);
      }

      SpaceSparseCosineSimilarityBinFast::parseSparseBinVector(lineBin, vBin);
      if (vBin.size() != vText.size()) {
        PREPARE_RUNTIME_ERR(err) << "# of elements in the text entry: " << vText.size() <<
                                    " is diff. from the # of elements in the bin. entry: " << vBin.size();
        THROW_RUNTIME_ERR(err);
      }

      for (uint32_t i = 0; i < qtyText; ++i) {
        CHECK_MSG(vBin[i].id_ == vText[i].id_,
                  "Mismatch in IDs between text and binary in line # " + ConvertToString(lineNum));
        CHECK_MSG(vBin[i].val_ == vText[i].val_,
                  "Mismatch in IDs between text and binary in line # " + ConvertToString(lineNum));
      }

      if (lineNum % REPORT_QTY == 0) {
        cout << lineNum << " lines checked" << endl;
      }
    }
    cout << lineNum << " lines checked" << endl;

    CHECK_MSG(SpaceSparseCosineSimilarityBinFast::readNextBinSparseVect(inpBinFile, lineBin) == false,
              "Binary input file contains more entries than the input text file!");
    CHECK_MSG(lineNum == inpBinFile.qty_, "Mismatch between text file entries: " + ConvertToString(lineNum) +
                                          " and the number of entries in the binary header: " + ConvertToString(inpBinFile.qty_));

  } catch (const std::exception& e) {
    LOG(LIB_FATAL) << e.what();
  } catch (...) {
    LOG(LIB_FATAL) << "Unknown exception caught";
  }

  LOG(LIB_INFO) << "Check succeeded!";


  return 0;
}
