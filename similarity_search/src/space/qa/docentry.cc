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

#include <space/qa/docentry.h>
#include <space/qa/inmemfwd_indxread.h>

namespace similarity {

DocEntryParser::DocEntryParser(const InMemFwdIndexReader &indxReader, size_t fieldId, const string &docStr) {
  stringstream strm(docStr);
  string line;
  vector<string> vParts1;

  CHECK_MSG(getline(strm, line), "Cannot read the first document line");

  if (!line.empty()) {
    boost::split(vParts1, line, boost::is_any_of("\t "));

    for (size_t i = 0; i < vParts1.size(); ++i) {
      vector<string> vParts2;

      boost::split(vParts2, vParts1[i], boost::is_any_of(":"));
      CHECK_MSG(vParts2.size() == 2,
                "Invalid document entry format in the first line (should end with two colon separated integers)");

      try {
        WORD_ID_TYPE wordId = boost::lexical_cast<int>(vParts2[0]);
        QTY_TYPE wordQty = boost::lexical_cast<int>(vParts2[1]);

        IDF_TYPE BM25IDF = 0, LuceneIDF = 0;

        if (wordId >= 0) {
          const WordRec *pWordRec = indxReader.getWordRec(fieldId, wordId);

          CHECK_MSG(NULL != pWordRec,
                    "Bug: Cannot obtain word info for wordId=" + boost::lexical_cast<string>(wordId) +
                    " fieldId=" + boost::lexical_cast<string>(fieldId));
          BM25IDF = pWordRec->mBM25IDF;
          LuceneIDF = pWordRec->mLuceneIDF;
        }

        mvWordIds.push_back(wordId);
        mvBM25IDF.push_back(BM25IDF);
        mvLuceneIDF.push_back(LuceneIDF);
        mvQtys.push_back(wordQty);
      } catch (const boost::bad_lexical_cast &) {
        throw runtime_error(
            "Invalid document entry format in the first line (cannot convert either word id and/or frequency to integer)");
      }
    }
  }

  CHECK_MSG(getline(strm, line), "Cannot read the second document line");

  boost::split(vParts1, line, boost::is_any_of("\t "));

  if (!line.empty()) {
    for (size_t i = 0; i < vParts1.size(); ++i) {
      try {
        int wordId = boost::lexical_cast<int>(vParts1[i]);

        mvWordIdSeq.push_back(wordId);
      } catch (const boost::bad_lexical_cast &) {
        throw runtime_error("Invalid document entry format in the second (cannot convert word id to integer)");
      }
    }
  }
}


};