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

#include <fstream>
#include <iomanip>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "logging.h"
#include "space/qa/giza_vocab_reader.h"

using namespace std;

using boost::trim;
using boost::split;
using boost::lexical_cast;

namespace similarity {

GizaVocRec::GizaVocRec(string line) {
  trim(line);

  vector<string> parts;
  split(parts, line, boost::is_any_of("\t "));

  CHECK_MSG(parts.size() == 3,
        "Wrong format of line '" + line + "', got  " + lexical_cast<string>(parts.size()) + " fields instead of three.");

  mWord = parts[1];
  
  try {
    mId  = lexical_cast<WORD_ID_TYPE>(parts[0]);
    mQty = lexical_cast<QTY_TYPE>(parts[2]);
  } catch(const boost::bad_lexical_cast &) {
    PREPARE_RUNTIME_ERR(err) << "Wrong format of line '" << line << "', either ID or the quantity field doesn't contain a proper integer.";
    THROW_RUNTIME_ERR(err);
  }
}


GizaVocabularyReader::GizaVocabularyReader(string fileName, const VocabularyFilter* pFilter) : mpProb(NULL), mpId(NULL)  {
  size_t qty = 0;
  double  totOccQty = 0;
    
  string line;

  {
    // Pass 1: compute the # of records and the total # of occurrences
    ifstream fr(fileName);
    CHECK_MSG(fr, "Cannot open file '" + fileName + "' for reading");
    fr.exceptions(ios::badbit);

    while (getline(fr, line)) {
      trim(line);
      if (line.empty()) continue;

      GizaVocRec rec(line);
      ++qty; totOccQty += rec.mQty;
    }

    mpProb    = new float[qty];
    mpId      = new WORD_ID_TYPE[qty];

    mWords.resize(qty);
      
    fr.close();
  }

  {
    // Pass 2: re-read the file and compute probabilities/
    ifstream fr(fileName);
    fr.exceptions(ios::badbit);

    size_t pos = 0;
    while (getline(fr, line)) {
      trim(line);
      if (line.empty()) continue;

      GizaVocRec rec(line);

      if (mWord2InternIdMap.count(rec.mWord)) {
        throw runtime_error("Repeating word: '" + rec.mWord+ "' in file: '" + fileName + "'");
      }
      if (mId2InternIdMap.count(rec.mId)) {
        throw runtime_error("Repeating ID: '" + lexical_cast<string>(rec.mId) + "' in file: '" + fileName + "'");
      }

      if (NULL == pFilter || pFilter->checkWord(rec.mWord)) {
        mWord2InternIdMap.insert(make_pair(rec.mWord, pos));
        mId2InternIdMap.insert(make_pair(rec.mId, pos));

        
        mpProb[pos]     = static_cast<float>(rec.mQty / totOccQty);
        mpId[pos]       = rec.mId;
        mWords[pos]     = rec.mWord;
        pos++;
      }
    }

    fr.close();
  }

  LOG(LIB_INFO) << "Read the vocabulary from '" << fileName << "'";

}


};
