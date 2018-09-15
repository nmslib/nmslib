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
#include "space/space_sparse_scalar.h"

#define SPACE_SPARSE_COSINE_SIMILARITY_BIN          "cosinesimil_sparse_bin"

namespace similarity {

class SpaceSparseCosineSimilarityBin : public SpaceSparseCosineSimilarity<float> {
public:
  SpaceSparseCosineSimilarityBin() {}

  virtual std::string StrDesc() const {
    return SPACE_SPARSE_COSINE_SIMILARITY_BIN;
  }

  unique_ptr<DataFileInputState> OpenReadFileHeader(const string& inpFileName) const override;
  virtual bool ReadNextObjStr(DataFileInputState &, string& strObj, LabelType& label, string& externId) const override;
  virtual unique_ptr<Object> CreateObjFromStr(IdType id, LabelType label, const string& s,
                                              DataFileInputState* pInpState) const;

protected:
  DISABLE_COPY_AND_ASSIGN(SpaceSparseCosineSimilarityBin);
};

}