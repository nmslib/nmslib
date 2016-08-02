/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2015
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include <cmath>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "space/qa/inmemfwd_indxread.h"
#include "logging.h"

namespace similarity {

using namespace std;

void InMemFwdIndexReader::readVocsAndDocStat() {
  for (size_t fieldId = 0; fieldId < mvInpFiles.size(); ++fieldId) {
    string          line;
    ifstream&       strm = *mvInpFiles[fieldId];
    const string&   fileName = mvFileNames[fieldId];

    vector<string>  vFields, vParts;

    // Read meta-info
    CHECK_MSG(getline(strm, line), "Can't read the first string from '" + fileName + "'");

    {
      stringstream str(line);

      if (!(str >> mvDocQty[fieldId]) ||
          !(str >> mvTotalWordQty[fieldId]) || 
          !(str.eof())) {
        PREPARE_RUNTIME_ERR(err) << "Invalid meta information in the first line (should be two-integers), file '" << fileName << "'";
        THROW_RUNTIME_ERR(err);
      }
    }
    CHECK_MSG(getline(strm, line), "Can't read the second string from '" + fileName + "'");
    CHECK_MSG(line.empty(), "Invalid format, the second line isn't empty in '" + fileName + "'"); 

    mvLineNum[fieldId] = 3;
    // Read the dictionary
    for (;getline(strm, line) && !line.empty(); ++mvLineNum[fieldId]) {
      boost::split(vFields, line, boost::is_any_of("\t"));

      if (vFields.size() != 2) {
        PREPARE_RUNTIME_ERR(err) << "Invalid dictionary format (should be two tab-separated parts), line " << mvLineNum[fieldId] << 
                                    " file: '" << fileName << "'";
        THROW_RUNTIME_ERR(err);
      }

      const string& word = vFields[0];

      if (mvStr2WordId[fieldId].count(word) > 0) {
          PREPARE_RUNTIME_ERR(err) << "Duplicate word: '" << word << "', line " << mvLineNum[fieldId] << 
                                    " file: '" << fileName << "'";
          THROW_RUNTIME_ERR(err);
      }

      boost::split(vParts, vFields[1], boost::is_any_of(":"));
      if (vParts.size() != 2) {
        PREPARE_RUNTIME_ERR(err) << "Invalid dictionary entry format (should end with two colon separated integers), line " << mvLineNum[fieldId] << 
                                    " file: '" << fileName << "'";
        THROW_RUNTIME_ERR(err);
      }

      try {
        WORD_ID_TYPE    wordId = boost::lexical_cast<int>(vParts[0]);
        QTY_TYPE        wordFreq = boost::lexical_cast<int>(vParts[1]);

        if (mvWordId2WordRec[fieldId].count(wordId) > 0) {
          PREPARE_RUNTIME_ERR(err) << "Duplicate wordId: " << wordId << ", line " << mvLineNum[fieldId] << 
                                    " file: '" << fileName << "'";
          THROW_RUNTIME_ERR(err);
        }

        mvMaxWordId[fieldId] = max(mvMaxWordId[fieldId], static_cast<size_t>(max(wordId, 0)));
  
        mvWordId2WordRec[fieldId].insert(make_pair(wordId, WordRec(mvDocQty[fieldId], wordFreq)));
        mvStr2WordId[fieldId].insert(make_pair(word, wordId));
      } catch(const boost::bad_lexical_cast &) {
        PREPARE_RUNTIME_ERR(err) << "Invalid dictionary entry format (should end with two colon separated integers), line " << mvLineNum[fieldId] << 
                                    " file: '" << fileName << "'";
        THROW_RUNTIME_ERR(err);
      }
    }
    
    if (!strm) {
        PREPARE_RUNTIME_ERR(err) << "Premature end at line " << mvLineNum[fieldId] << 
                                    " file: '" << fileName << "' expecting an empty string after all dict. entries.";
        THROW_RUNTIME_ERR(err);
    }

    mvAvgDocLen[fieldId] = max(float(mvTotalWordQty[fieldId]) / mvDocQty[fieldId], 
                               1e-5f /* to prevent (though very unlikely division by zero */);
    mvInvAvgDocLen[fieldId] = 1.0/mvAvgDocLen[fieldId];
  }
}

bool InMemFwdIndexReader::readNextObjectStr(string& extId, string& strObj) {
  strObj.clear();

  vector<string>  vFields;

  extId = "";

  bool isEOF = false;

  for (size_t fieldId = 0; fieldId < mvInpFiles.size(); ++fieldId) {
    ifstream&       strm = *mvInpFiles[fieldId];

    if (strm.eof()) {
      if (!fieldId) isEOF = true;
      else {
        if (!isEOF) {
          PREPARE_RUNTIME_ERR(err) << "Premature EOF in file '" << mvFileNames[fieldId] << 
                                      ", e.g., file reader for '" << mvFileNames[fieldId - 1] << "' didn't reach EOF yet ";
          THROW_RUNTIME_ERR(err);
        }
      }
    }
  }

  if (isEOF) return false;

  for (size_t fieldId = 0; fieldId < mvInpFiles.size(); ++fieldId) {
    string          line;
    ifstream&       strm = *mvInpFiles[fieldId];
    const string&   fileName = mvFileNames[fieldId];
    size_t&         lineNum = mvLineNum[fieldId];


    ++lineNum;
    if (!getline(strm, line)) {
      PREPARE_RUNTIME_ERR(err) << "Premature end at line " << lineNum << 
                                    " file: '" << fileName << "' expecting a document ID.";
      THROW_RUNTIME_ERR(err);
    }
    if (!fieldId) {
      boost::trim(line);
      extId = line;
    } else {
      if (extId != line) {
        PREPARE_RUNTIME_ERR(err) << "Wrong document id at line " << lineNum << 
                                    " file: '" << fileName << "' expecting '" << extId << "' but got '" << line << "'";
        THROW_RUNTIME_ERR(err);
      }
    }
  }

  if (extId.empty()) {
    // This may be the end of all files, but let's double-check if this was the last string read
    for (size_t fieldId = 0; fieldId < mvInpFiles.size(); ++fieldId) {
      string          line;
      ifstream&       strm = *mvInpFiles[fieldId];
      const string&   fileName = mvFileNames[fieldId];
      size_t&         lineNum = mvLineNum[fieldId];


      ++lineNum;
      if (getline(strm, line)) {
        PREPARE_RUNTIME_ERR(err) << "Expecting EOF at line " << lineNum << 
                                    " after an empty line, but got some string " <<
                                    " file: '" << fileName << "'";
        THROW_RUNTIME_ERR(err);
      }
    }
    /* Ok, truly EOF */
    return false;
  }

  for (size_t fieldId = 0; fieldId < mvInpFiles.size(); ++fieldId) {
    string          line;
    ifstream&       strm = *mvInpFiles[fieldId];
    const string&   fileName = mvFileNames[fieldId];
    size_t&         lineNum = mvLineNum[fieldId];

    ++lineNum;
    if (!getline(strm, line)) {
      PREPARE_RUNTIME_ERR(err) << "Premature end at line " << lineNum << 
                                    " file: '" << fileName << "' expecting the first document entry for document ID='" << extId << "'";
      THROW_RUNTIME_ERR(err);
    }

    strObj += line;
    strObj += '\n';

    ++lineNum;
    if (!getline(strm, line)) {
      PREPARE_RUNTIME_ERR(err) << "Premature end at line " << lineNum << 
                                    " file: '" << fileName << "' expecting the second document entry for document ID='" << extId << "'";
      THROW_RUNTIME_ERR(err);
    }

    strObj += line;
    strObj += '\n';
  }

  return true;
}

};
