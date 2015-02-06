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

#include <cmath>
#include <fstream>
#include <string>
#include <sstream>

#include "object.h"
#include "logging.h"
#include "distcomp.h"
#include "experimentconf.h"
#include "space/space_string.h"

namespace similarity {

using namespace std;

template <typename dist_t>
void StringSpace<dist_t>::ReadStr(string line, LabelType& label, string& str) const
{
  label = Object::extractLabel(line);

  if (EMPTY_LABEL == label) {
    LOG(LIB_FATAL) << "Missing label in line: '" << line << "'";
  }

  str = line;
}


template <typename dist_t>
void StringSpace<dist_t>::WriteDataset(const ObjectVector& dataset,
                                       const char* outputfile) const {
  std::ofstream outFile(outputfile, ostream::out | ostream::trunc);

  if (!outFile) {
    LOG(LIB_FATAL) << "Cannot open: '" << outputfile << "' for writing!";
  }

  outFile.exceptions(std::ios::badbit | std::ios::failbit);

  for (const Object* obj: dataset) {
    CHECK(obj->datalength() > 0);
    const char* x = reinterpret_cast<const char*>(obj->data());
    const size_t length = obj->datalength() / sizeof(char);

    if (obj->label()>=0) outFile << LABEL_PREFIX << obj->label() << " ";

    outFile << string(x, length) << endl;
  }

  outFile.close();
}


template <typename dist_t>
void StringSpace<dist_t>::ReadDataset(
    ObjectVector& dataset,
    const ExperimentConfig<dist_t>* config,
    const char* FileName,
    const int MaxNumObjects) const {

  dataset.clear();
  dataset.reserve(MaxNumObjects);

  string temp;

  std::ifstream InFile(FileName);

  if (!InFile) {
      LOG(LIB_FATAL) << "Cannot open file: " << FileName;
  }

  InFile.exceptions(std::ios::badbit);

  try {

    std::string StrLine;

    int linenum = 0;
    int id = linenum;
    LabelType label = -1;

    while (getline(InFile, StrLine) && (!MaxNumObjects || linenum < MaxNumObjects)) {
      ReadStr(StrLine, label, temp);

      id = linenum++;
      dataset.push_back(CreateObjFromStr(id, label, temp));
    }
  } catch (const std::exception &e) {
    LOG(LIB_ERROR) << "Exception: " << e.what();
    LOG(LIB_FATAL) << "Failed to read/parse the file: '" << FileName << "'";
  }
}


template class StringSpace<int>;
// TODO: do we really need floating-point distances for string spaces? 
template class StringSpace<float>;
template class StringSpace<double>;

}  // namespace similarity
