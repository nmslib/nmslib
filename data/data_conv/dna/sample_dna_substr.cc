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
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cmath>
#include <fstream>
#include <string>
#include <sstream>
#include <random>
#include <vector>
#include <boost/program_options.hpp>


using namespace std;

const string COMMON_PREFIX = "label:-1 ";
const double PROB_BIAS_COEFF = 1.2;

namespace po = boost::program_options;

static void Usage(const char *prog,
                  const po::options_description& desc) {
    cout << prog << endl
         << desc << endl;
}

string toUpper(const string& s) {
  string res;
  for (char c: s) res += toupper(c);
  return res;
}

int main(int argc, char* argv[]) {
  po::options_description ProgOptDesc("Allowed options");

  size_t nString;
  size_t nMinLen;
  size_t nAvgLen;
  size_t nLenStd;
  size_t nQty;
  string inputFile;
  string outputFile;

  po::variables_map vm;

  ProgOptDesc.add_options()
    ("help,h",        "produce help message")
    ("inputFile,i",   po::value<string>(&inputFile)->required(),
                      "input uncompressed file (download using download_and_clean_DNA.sh)")
    ("outputFile,o",   po::value<string>(&outputFile)->required(),
                      "output data file")
    ("avgLen,a",      po::value<size_t>(&nAvgLen)->required(),
                      "average sequence length")
    ("qty,q",         po::value<size_t>(&nQty)->required(),
                      "# of sequences (may not generate this exact number)")
    ("minLen",        po::value<size_t>(&nMinLen)->default_value(1),
                      "minimum sequence length")
    ("lenSTD",        po::value<size_t>(&nLenStd)->required(),
                      "standard deviation of the sequence length")
  ;

  try {
    po::store(po::parse_command_line(argc, argv, ProgOptDesc), vm);
    po::notify(vm);

  } catch (const exception& e) {
    Usage(argv[0], ProgOptDesc);
    cerr << e.what();
    return 1;
  }

  if (vm.count("help")  ) {
    Usage(argv[0], ProgOptDesc);
    exit(0);
  }

  cout << "We are going to randomly sample " << nQty << " (sub)sequences." << endl;
  cout << "Average len: " << nAvgLen << " STD: " << nLenStd << " (minimum len: " << nMinLen << ")" << endl;

  static random_device                        rdev;
  static mt19937                              randEngine(rdev());
  static normal_distribution<double>          randGenNormal(nAvgLen, nLenStd);
  static uniform_real_distribution<double>    randGenUniform(0, 1);

  size_t symQty = 0;

  if (nMinLen < 1 || nMinLen > nAvgLen) {
    cerr << "Minimum string length can't be < 1 or > average string length";
    Usage(argv[0], ProgOptDesc);
    return 1;
  }

  {
    cout << "FIRST pass: just count the overall number of symbols" << endl;
    string    s;
    fstream   inp(inputFile.c_str(), ios::in); 
    if (!inp) {
      cerr << "Cannot open file '" << inputFile << "' for reading" << endl;
      return 1;
    }
    inp.exceptions(ios::badbit);
    while (getline(inp, s)) {
      symQty += s.size();
    }

    cout << "The total number of symbols: " << symQty << endl;
  }

  {
    cout << "SECOND pass: actually generating." << endl;
    cout << "**************************** !!! Note !!! **************************" << endl;
    cout << "Our simplistic generation algorithm is slightly biased towards ";
    cout << "numbers in the beginning and it may generate fewer sequences than you ask!." << endl;
    cout << "********************************************************************" << endl;

    double probSelect = PROB_BIAS_COEFF * nQty / symQty;

    cout << "Selection probability: " << probSelect << " expectation: " << (symQty * probSelect) << endl;

    string    line;
    fstream   inp(inputFile.c_str(), ios::in); 
    if (!inp) {
      cerr << "Cannot open file '" << inputFile << "' for reading" << endl;
      return 1;
    }

    fstream   outp(outputFile.c_str(), ios::out | ios::trunc); 
    if (!outp) {
      cerr << "Cannot open file '" << outputFile << "' for writing" << endl;
      return 1;
    }

    vector<string>   seqStack;
    vector<size_t>   seqStackExpLen;

    size_t genQty = 0;

    inp.exceptions(ios::badbit);
    while (getline(inp, line)) {
      // 1. Try to extend strings that were already on the stack or
      //    were added to the stack in the current iteration
      for (size_t i = 0; i < seqStack.size(); ) {
        string& cs = seqStack[i];
        if (cs.length() > seqStackExpLen[i]) {
          cerr << "Bug: the length of the string on the stack (" << cs.length() << ") "
              << " is already > seqStackExpLen[i] " << endl;
          return 1;
        }
        size_t diff = min(seqStackExpLen[i] - cs.length(), line.length());
        //cout << "DIFF=" << diff << endl;
        cs += line.substr(0, diff);
        if (cs.length() == seqStackExpLen[i]) {
          outp << COMMON_PREFIX << toUpper(cs) << endl;
          /*
           * Let's delete from the stack, this is rather expensive but this
           * is not going to happen very often: don't increase the
           * loop counter in this case though!
           */
          seqStack.erase(seqStack.begin() + i);
          seqStackExpLen.erase(seqStackExpLen.begin() + i);
          if (++genQty >= nQty) goto finish;
        } else ++i;
      }

      // 2. Add strings to the stack at randomly selected starting positions
      for (size_t i = 0; i < line.size(); ++i) {
        if (randGenUniform(randEngine) < probSelect) {
        // Ok, now select the length randomly
          size_t len = static_cast<size_t>(
                                      round(
                                        max<double>(nMinLen,
                                                    randGenNormal(randEngine))));

          //cout << "Pos: " << i << " len: " << len << endl;
          size_t diff = min(len, line.length() - i);
          seqStack.push_back(line.substr(i, diff));
          seqStackExpLen.push_back(len);
        }
      }
    }
finish:
    // Ignore whatever is on the stack, there is only a tiny chance we generated fewer strings
    // than we needed
    cout << "Finished, generated " << genQty << " (sub) sequences" << endl; 
  }

  return 0;
}
