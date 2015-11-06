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
#include <cstring>
#include <map>
#include <limits>
#include <iostream>
#include <stdexcept>

#include "utils.h"
#include "params.h"
#include "logging.h"
#include "space.h"

#include <cmath>

using namespace std;

namespace similarity {

using std::multimap;
using std::string;

void ParseArg(const string& descStr, vector<string>& vDesc) {
  vDesc.clear();

  if (!SplitStr(descStr, vDesc, ',')) {
    throw runtime_error("Cannot split space arguments in: '" + descStr + "'");
  }
}


};


