/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2019
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _SPACE_SPARSE_DENSE_FUSION_H
#define _SPACE_SPARSE_DENSE_FUSION_H

#include <space.h>
#include <object.h>

#include <memory>

#include <space/space_sparse_vector_inter.h>

#define SPACE_SPARSE_DENSE_FUSION   "sparse_dense_fusion"


namespace similarity {

class SpaceSparseDenseFusion : public Space<float> {

public:
  explicit SpaceSparseDenseFusion(const string& weightFileName);

private:

  virtual ~SpaceSparseDenseFusion() {}

  /** Standard functions to read/write/create objects */
  // Create an object from string representation.
  virtual unique_ptr<Object> CreateObjFromStr(IdType id, LabelType label, const string &s,
                                              DataFileInputState *pInpState) const override;

  // Create a string representation of an object.
  virtual string CreateStrFromObj(const Object *pObj, const string &externId) const override {
    throw runtime_error("Not implemented!");
  }

  // Open a file for reading, fetch a header (if there is any) and memorize an input state
  virtual unique_ptr<DataFileInputState> OpenReadFileHeader(const string &inputFile) const override;

  // Open a file for writing, write a header (if there is any) and memorize an output state
  virtual unique_ptr<DataFileOutputState> OpenWriteFileHeader(const ObjectVector &dataset,
                                                              const string &outputFile) const override {
    throw runtime_error("Not implemented!");
  }

  void UpdateParamsFromFile(DataFileInputState& inpStateBase) override {
    DataFileInputStateSparseDenseFusion& inpState = dynamic_cast<DataFileInputStateSparseDenseFusion&>(inpStateBase);
    vCompDesc_ = inpState.vCompDesc_;
  }

  // Read a string representation of the next object in a file
  virtual bool ReadNextObjStr(DataFileInputState &, string &strObj, LabelType &label, string &externId) const override;

  // Write a string representation of the next object to a file
  virtual void WriteNextObj(const Object &obj, const string &externId, DataFileOutputState &) const override {
    throw runtime_error("Not implemented!");
  }
  /** End of standard functions to read/write/create objects */

  /*
   * Used only for testing/debugging: compares objects approximately. Floating point numbers
   * should be nearly equal. Integers and strings should coincide exactly.
   */
  virtual bool ApproxEqual(const Object &obj1, const Object &obj2) const override {
    throw runtime_error("Not supported!");
  }

  virtual string StrDesc() const override { return SPACE_SPARSE_DENSE_FUSION; }

  virtual void CreateDenseVectFromObj(const Object *obj, float *pVect, size_t nElem) const override {
    throw runtime_error("Cannot create vector for the space: " + StrDesc());
  }

  virtual size_t GetElemQty(const Object *object) const override { return 0; }

  virtual float ProxyDistance(const Object* obj1, const Object* obj2) const override;

protected:
  DISABLE_COPY_AND_ASSIGN(SpaceSparseDenseFusion);

  virtual float HiddenDistance(const Object *obj1, const Object *obj2) const override;

  float compDistance(const Object *obj1, const Object *obj2, bool isIndex) const;

  struct CompDesc {
    CompDesc(bool isSparse, size_t dim,
             float indexWeight, float queryWeight) :
      isSparse_(isSparse), dim_(dim) {
      indexWeight_ = indexWeight;
      queryWeight_ = queryWeight;
    }

    bool isSparse_;
    size_t dim_;
    float indexWeight_;
    float queryWeight_;
  };

  vector<CompDesc>  vCompDesc_;

  string            weightFileName_;
  vector<float>     vHeaderIndexWeights_;
  vector<float>     vHeaderQueryWeights_;

  struct DataFileInputStateSparseDenseFusion : public DataFileInputState {
    virtual void Close() { inpFile_.close(); }

    DataFileInputStateSparseDenseFusion(const string& inpFileName) :
      inpFile_(inpFileName.c_str(), ios::binary), readQty_(0) {

      if (!inpFile_) {
        PREPARE_RUNTIME_ERR(err) << "Cannot open file: " << inpFileName << " for reading";
        THROW_RUNTIME_ERR(err);
      }

      inpFile_.exceptions(ios::badbit);
    }

    virtual ~DataFileInputStateSparseDenseFusion(){};

    ifstream          inpFile_;
    size_t            qty_ = 0; // Total number of entries
    size_t            readQty_ = 0; // Number of read entries
    vector<CompDesc>  vCompDesc_;

    friend class SpaceSparseDenseFusion;
  };

  void readNextBinSparseVect(DataFileInputStateSparseDenseFusion &state, string& strObj) const {

    uint32_t qty;
    readBinaryPOD(state.inpFile_, qty);
    size_t elemSize = SparseVectElem<float>::dataSize();
    // Data has extra space for the number of sparse vector elements
    vector<char> data(elemSize * qty  + 4);
    char *p = &data[0];

    writeBinaryPOD(p, qty);

    state.inpFile_.read(p + 4, qty * elemSize);

    strObj.assign(p, data.size());
  }

  void readNextBinDenseVect(DataFileInputStateSparseDenseFusion &state, string& strObj, unsigned dim) const {
    uint32_t qty;
    readBinaryPOD(state.inpFile_, qty);
    if (qty != dim) {
      PREPARE_RUNTIME_ERR(err) << "Mismatch between dimension in the header (" << dim <<
                               ") and the actual dimensionality of the current entry (" << qty << ")";
      THROW_RUNTIME_ERR(err);

    }
    vector<char> data(dim * sizeof(float));
    char *p = &data[0];
    state.inpFile_.read(p, data.size());

    strObj.assign(p, data.size());
  }

  /*
   * Extract/parses a binary dense vector stored in the string
   * strObj starting from the position start.
   *
   * @param strObj a string that stores a vector + possibly something else
   * @param vDense a vector to store the result
   * @param start where to start extraction
   * @param dim dimensionality of the vector.
   */
  void parseDenseBinVect(const string& strObj, vector<float>& vDense, unsigned& start, unsigned dim) const {
    const char* p = strObj.data() + start;
    size_t expectSize = dim * 4;

    CHECK_MSG(strObj.size() >= expectSize + start,
              string("The received string object is too little! ") +
              " Start: " + ConvertToString(start) +
              " Str obj size: " + ConvertToString(strObj.size()) +
              " # dim: " + ConvertToString(dim) +
              " expected size: " + ConvertToString(expectSize));

    vDense.resize(dim);

    for (uint_fast32_t i = 0; i < dim; ++i) {
      readBinaryPOD(p, vDense[i]);
      p += 4;
    }

    start += expectSize;

  }

  /**
   * Extract/parses a binary sparse vector stored in the string
   * strObj starting from the position start.
   *
   * @param strObj a string that stores a vector + possibly something else
   * @param v an output vector id-value pairs
   * @param start where to start extraction
   * @param sortDimId true if we want to re-sort results by the ids
   */
  void parseSparseBinVect(const string &strObj,
                          vector<SparseVectElem<float>> &v,
                          unsigned &start,
                          bool sortDimId = true) const {
    uint32_t qty;

    CHECK(strObj.size() >= sizeof(qty));

    const char* p = strObj.data() + start;

    readBinaryPOD(p, qty);
    v.resize(qty);
    size_t elemSize = SparseVectElem<float>::dataSize();
    size_t expectSize = sizeof(qty) + elemSize * qty;
    CHECK_MSG(strObj.size() >= expectSize + start,
              string("The received string object is stoo little! ") +
              " Start: " + ConvertToString(start) +
              " Str obj size: " + ConvertToString(strObj.size()) +
              " # of vect elems: " + ConvertToString(qty) +
              " expected size: " + ConvertToString(expectSize));
    start += expectSize;
    p += sizeof(qty);
    for (uint_fast32_t i = 0; i < qty; ++i) {
      readBinaryPOD(p, v[i].id_);
      p += sizeof(v[i].id_);

      readBinaryPOD(p, v[i].val_);
      p += sizeof(v[i].val_);
    }
    if (sortDimId) {
      sort(v.begin(), v.end());
    }
    for (uint_fast32_t i = 1; i < qty; ++i) {
      CHECK_MSG(v[i].id_ > v[i-1].id_, "Ids in the input file are either unsorted or have duplicates!")
    }
  }

  inline size_t getPad4(size_t len) const {
    size_t rem = len & 3;
    if (rem) {
      return 4 - rem;
    }
    return 0;
  }


};



}


#endif

