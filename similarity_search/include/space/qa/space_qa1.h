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

#ifndef SPACE_QA1_H_
#define SPACE_QA1_H_

#include <string>
#include <fstream>
#include <memory>
#include <limits>
#include <map>
#include <stdexcept>
#include <sstream>

#include <boost/lexical_cast.hpp>

#include "global.h"
#include "object.h"
#include "utils.h"
#include "space.h"
#include "logging.h"
#include "distcomp.h"

#include <space/space_vector.h>

#include <space/qa/simil.h>
#include <space/qa/docentry.h>
#include <space/qa/embed_reader_recoder.h>
#include <space/qa/simple_inv_index.h>
#include <space/qa/inmemfwd_indxread.h>
#include <space/qa/giza_tran_table_reader.h>
#include <space/qa/giza_vocab_reader.h>


#ifdef USE_GOOGLE_SPARSE_HASH_FOR_PAYLOADS
#include <google/dense_hash_map>
#else
#include <unordered_map>
#endif

#define SPACE_QA1      "qa1"

namespace similarity {


struct TranRecEntryInfo {
  TranRecEntryInfo(uint32_t startIndex = 0, uint32_t qty = 0) : startIndex_(startIndex), qty_(qty) {}
  uint32_t  startIndex_;
  uint32_t  qty_;
};

#ifdef USE_GOOGLE_SPARSE_HASH_FOR_PAYLOADS
typedef google::dense_hash_map<IdType, TranRecEntryInfo> TABLE_HASH_TYPE;
#else
typedef std::unordered_map<IdType, TranRecEntryInfo> TABLE_HASH_TYPE   ;
#endif

class HashTablePayload : public ObjectPayload {
public:
  HashTablePayload(size_t fieldQty) : vTranEntriesHash_(fieldQty) {}
  virtual ~HashTablePayload() override  {
    for (size_t i = 0; i < vTranEntriesHash_.size(); ++i)
    if (vTranEntriesHash_[i] != nullptr) {
      delete vTranEntriesHash_[i];
    }
  }
  virtual HashTablePayload* Clone() override {
    throw runtime_error("Clone isn't supported!");
  }
  std::vector<TABLE_HASH_TYPE*> vTranEntriesHash_;
};

struct PivotInvIndexHolder {
  PivotInvIndexHolder(
      vector<unique_ptr<SimpleInvIndex>>* cosineIndex, vector<unique_ptr<SimpleInvIndex>>*  bm25index, vector<unique_ptr<SimpleInvIndex>>  *model1index, size_t pivotQty) :
    cosineIndex_(cosineIndex), bm25index_(bm25index), model1index_(model1index), pivotQty_(pivotQty) {}
  unique_ptr<vector<unique_ptr<SimpleInvIndex>>>  cosineIndex_;
  unique_ptr<vector<unique_ptr<SimpleInvIndex>>>  bm25index_;
  unique_ptr<vector<unique_ptr<SimpleInvIndex>>>  model1index_;
  size_t                                          pivotQty_;
};

using std::string;
using std::vector;
using std::unique_ptr;
using std::ifstream;

using boost::lexical_cast;

const float OOV_PROB = 1e-9f;

struct SpaceParamQA1 {
  SpaceParamQA1(bool                    useHashBasedPayloads,
                const vector<float>&    featureWeights,
                const vector<float>&    featureWeightsPivots,
                const vector<string>&   indexFileNames,
                const vector<string>&   tranTablePrefix,
                const vector<size_t>&   gizaIterQty,
                const vector<float>&    minTranProb,
                const vector<float>&    lambdaModel1,
                const vector<float>&    probSelfTran,
                const vector<string>&   embedFileNames,
                const vector<uint64_t>& featureMasks,
                const vector<uint64_t>& featureMasksPivots):
                                                        mUseHashBasedPayloads(useHashBasedPayloads),
                                                        mFeatureWeights(featureWeights),
                                                        mFeatureWeightsPivots(featureWeightsPivots),
                                                        mIndxReader(indexFileNames),
                                                        mGizaIterQty(gizaIterQty),
                                                        mMinTranProb(minTranProb),
                                                        mLambdaModel1(lambdaModel1),
                                                        mProbSelfTran(probSelfTran),
                                                        mFeatureMasks(featureMasks),
                                                        mFeatureMasksPivots(featureMasksPivots){
    size_t fieldQty = indexFileNames.size();
    CHECK_MSG(indexFileNames.size() == tranTablePrefix.size(),
              "Bug: the number of index files " + boost::lexical_cast<string>(indexFileNames.size()) + " "
              " != tranTablePrefix.size() " +  boost::lexical_cast<string>(tranTablePrefix.size()));
    CHECK_MSG(indexFileNames.size() == gizaIterQty.size(),
              "Bug: the number of index files " + boost::lexical_cast<string>(indexFileNames.size()) + " "
              " != gizaIterQty.size() " +  boost::lexical_cast<string>(gizaIterQty.size()));
    CHECK_MSG(indexFileNames.size() == minTranProb.size(),
              "Bug: the number of index files " + boost::lexical_cast<string>(minTranProb.size()) + " "
              " != minTranProb.size() " +  boost::lexical_cast<string>(tranTablePrefix.size()));
    CHECK_MSG(indexFileNames.size() == lambdaModel1.size(),
              "Bug: the number of index files " + boost::lexical_cast<string>(lambdaModel1.size()) + " "
                  " != minTranProb.size() " +  boost::lexical_cast<string>(tranTablePrefix.size()));
    CHECK_MSG(indexFileNames.size() == probSelfTran.size(),
              "Bug: the number of index files " + boost::lexical_cast<string>(indexFileNames.size()) + " "
              " != probSelfTran.size() " +  boost::lexical_cast<string>(probSelfTran.size()));
    CHECK_MSG(indexFileNames.size() == featureMasks.size(),
              "Bug: the number of index files " + boost::lexical_cast<string>(indexFileNames.size()) + " "
              " != the number of feature masks " +  boost::lexical_cast<string>(featureMasks.size()));
    CHECK_MSG(embedFileNames.size() == featureMasks.size(),
              "Bug: the number of embedding files " + boost::lexical_cast<string>(embedFileNames.size()) + " "
                  " != the number of feature masks " +  boost::lexical_cast<string>(featureMasks.size()));
    // Read statistics and dictionaries before actual objects will be read
    mIndxReader.readVocsAndDocStat();
    mFilterAndRecoder.resize(fieldQty);
    mTranTables.resize(fieldQty);
    mVocSrc.resize(fieldQty);
    mVocDst.resize(fieldQty);
    mWordEmbeddings.resize(fieldQty);
    for (size_t fieldId = 0; fieldId < fieldQty; ++fieldId) {
      if (!tranTablePrefix[fieldId].empty()) {
        mFilterAndRecoder[fieldId].reset(new InMemForwardIndexFilterAndRecoder(mIndxReader, fieldId));
        mVocSrc[fieldId].reset(
            new GizaVocabularyReader(tranTablePrefix[fieldId] + "/source.vcb", mFilterAndRecoder[fieldId].get()));
        mVocDst[fieldId].reset(
            new GizaVocabularyReader(tranTablePrefix[fieldId] + "/target.vcb", mFilterAndRecoder[fieldId].get()));
        mTranTables[fieldId].reset(new GizaTranTableReaderAndRecoder(
            tranTablePrefix[fieldId] + "/output.t1." + lexical_cast<string>(mGizaIterQty[fieldId]),
            *mFilterAndRecoder[fieldId],
            *mVocSrc[fieldId],
            *mVocDst[fieldId],
            mProbSelfTran[fieldId],
            mMinTranProb[fieldId])
        );
      }
      if (!embedFileNames[fieldId].empty()) {
        mWordEmbeddings[fieldId].reset(new EmbeddingReaderAndRecoder(embedFileNames[fieldId], mIndxReader, fieldId));
      }
    }
  }
  size_t getFieldQty() const { return mFeatureMasks.size(); }


  bool                                   mUseHashBasedPayloads;
  const vector<float>                    mFeatureWeights;
  const vector<float>                    mFeatureWeightsPivots;
  InMemFwdIndexReader                    mIndxReader;
  vector<unique_ptr<const InMemForwardIndexFilterAndRecoder>> mFilterAndRecoder;
  vector<unique_ptr<const GizaTranTableReaderAndRecoder>>     mTranTables;
  vector<unique_ptr<const GizaVocabularyReader>>              mVocSrc;
  vector<unique_ptr<const GizaVocabularyReader>>              mVocDst;
  vector<unique_ptr<const EmbeddingReaderAndRecoder>>         mWordEmbeddings;
  const vector<size_t>                   mGizaIterQty;
  const vector<float>                    mMinTranProb;
  const vector<float>                    mLambdaModel1;
  const vector<float>                    mProbSelfTran;
  const vector<uint64_t>                 mFeatureMasks;
  const vector<uint64_t>                 mFeatureMasksPivots;
};

class DataFileInputStateQA1 : public DataFileInputState {
public:
  DataFileInputStateQA1(const string& headerFileName);

  virtual void Close() {
    DataFileInputState::Close();
  }
  virtual ~DataFileInputStateQA1() {} 

  // This member is deliberately left public
  unique_ptr<SpaceParamQA1> mSpaceParams;

private:
  bool                                   mIsQueryFile;
  unique_ptr<ifstream>                   mHeadStrm;
  size_t                                 mLineNum;

  void logFeatureMasks(const string& msg, const vector<uint64_t>& featureMasks) const;
  void logFeatureWeights(const string& msg,  const vector<float>& featureWeights) const;

  friend class SpaceQA1;
};

class SpaceQA1 : public Space<float> {
 public:
  explicit SpaceQA1() {
    LOG(LIB_INFO) << "Created " << StrDesc();
  }
  virtual ~SpaceQA1() {}

  virtual std::string StrDesc() const override {
    stringstream  str;
    str << "QA1";
    return str.str();
  }

  /** Standard functions to read/write/create objects */ 
  /*
   * Create an object from string representation.
   * If the input state pointer isn't null, we check
   * if the new vector is consistent with previous output.
   * If now previous output was seen, the state vector may be
   * updated. For example, when we start reading vectors,
   * we don't know the number of elements. When, we see the first
   * vector, we memorize dimensionality. If a subsequently read
   * vector has a different dimensionality, an exception will be thrown.
   */
  virtual unique_ptr<Object> CreateObjFromStr(IdType id, LabelType label, const string& s,
                                              DataFileInputState* pInpState) const override;
  // Create a string representation of an object.
  virtual string CreateStrFromObj(const Object* pObj, const string& externId /* ignored */) const override;
  // Open a file for reading, fetch a header (if there is any) and memorize an input state
  virtual unique_ptr<DataFileInputState> OpenReadFileHeader(const string& inputFile) const override;
  // Open a file for writing, write a header (if there is any) and memorize an output state
  virtual unique_ptr<DataFileOutputState> OpenWriteFileHeader(const ObjectVector& dataset,
                                                              const string& outputFile) const override;
  /*
   * Read a string representation of the next object in a file as well
   * as its label. Return false, on EOF.
   */
  virtual bool ReadNextObjStr(DataFileInputState &, string& strObj, LabelType& label, string& externId) const override;
  /* 
   * Write a string representation of the next object to a file. We totally delegate
   * this to a Space object, because it may package the string representation, by
   * e.g., in the form of an XML fragment.
   */
  virtual void WriteNextObj(const Object& obj, const string& externId, DataFileOutputState &) const override;

  virtual void UpdateParamsFromFile(DataFileInputState& inpStateBase) override {
    DataFileInputStateQA1* pInpState = dynamic_cast<DataFileInputStateQA1*>(&inpStateBase);
    CHECK_MSG(pInpState != NULL, "Bug: unexpected pointer type");

    CHECK_MSG(!pInpState->mIsQueryFile,
              "UpdateParamsFromFile shouldn't be used on query files!");
   /* 
    * Transferring ownership of read space parameters to the space object.
    */
    mSpaceParams.reset(pInpState->mSpaceParams.release());
  }
  /** End of standard functions to read/write/create objects */

  vector<unique_ptr<SimpleInvIndex>>* computeModel1PivotIndex(const ObjectVector& pivots) const;
  vector<unique_ptr<SimpleInvIndex>>* computeBM25PivotIndex(const ObjectVector& pivots) const;
  vector<unique_ptr<SimpleInvIndex>>* computeCosinePivotIndex(const ObjectVector& pivots) const;

  /*
   * get document/query/pivot statistics as well as "intersection" statistics.
   */
  void getObjStat(const Object* pObjData /* or pivot */, const Object* pObjQuery, IdTypeUnsign fieldId,
                  IdTypeUnsign& docWordQty, IdTypeUnsign& queryWordQty, IdTypeUnsign& intersectSize,
                  IdTypeUnsign& queryTranRecsQty, IdTypeUnsign& queryTranObjIntersectSize) const;

  /*
   * Used only for testing/debugging: compares objects approximately. Floating point numbers
   * should be nearly equal. Integers and strings should coincide exactly.
   */
  virtual bool ApproxEqual(const Object& obj1, const Object& obj2) const override {
    return true; // In an actual, non-dummy class, you should return the result
                 // of an actual comparison. Here 'true' is used only to make it compile.
  }

  /*
   * CreateDenseVectFromObj and GetElemQty() are only needed, if
   * one wants to use methods with random projections.
   */
  virtual void CreateDenseVectFromObj(const Object* obj, float* pVect,
                                   size_t nElem) const override {
    throw runtime_error("Cannot create vector for the space: " + StrDesc());
  }
  virtual size_t GetElemQty(const Object* object) const override {return 0;}

  /*
   * This function computes multiple distances to several queries, but it doesn't support
   * all the features: only BM25, overall match, and translation features: Model1 & Simple Tran
   */
  void computePivotDistances(const Object* pQuery, const PivotInvIndexHolder& pivotInfo, vector<float>& vDist) const;
  void setDontPrecomputeFlag(bool flag) const { mDontPrecomputeFlag = flag; }

  virtual float ProxyDistance(const Object* pObjData, const Object* pObjQuery) const override;
 protected:

 /*
  * This function should always be protected.
  * Only children and friends of the Space class
  * should be able to access it.
  */
  virtual float HiddenDistance(const Object* pObjData, const Object* pObjQuery) const override;
  float DistanceInternal(const Object* pObjData, const Object* pObjQuery,
                         const vector<float>& featureWeights,
                         const vector<uint64_t>& featureMasks) const;

private:
  /*
   * 1) retrieves the number of fields for this object 
   * 2) obtains a pointer to the data following the field counter
   */
  FIELD_QTY_TYPE getFieldQty(const Object* pObj, const char*& pBufPtr) const {
    CHECK(pObj->datalength() >= sizeof(FIELD_QTY_TYPE));
    pBufPtr = pObj->data();
    FIELD_QTY_TYPE res = *reinterpret_cast<const FIELD_QTY_TYPE*>(pBufPtr);
    pBufPtr += sizeof(FIELD_QTY_TYPE);
    return res;
  }

  inline void getNextDocEntryPtr(const char*& pBufPtr, DocEntryPtr& docEntry)  const {
    const DocEntryHeader* pHeader = reinterpret_cast<const DocEntryHeader*>(pBufPtr);
    pBufPtr += sizeof(*pHeader);
    docEntry.mWordIdsQty   = pHeader->mWordIdsQty;
    docEntry.mWordIdsTotalQty = pHeader->mWordIdsTotalQty;
    docEntry.mWordIdSeqQty = pHeader->mWordIdSeqQty;
    docEntry.mWordEmbedDim = pHeader->mWordEmbedDim;

    docEntry.mpWordIds = reinterpret_cast<const WORD_ID_TYPE*>(pBufPtr);
    pBufPtr += sizeof(WORD_ID_TYPE) * docEntry.mWordIdsQty;

    docEntry.mpBM25IDF     = reinterpret_cast<const IDF_TYPE*>(pBufPtr);
    pBufPtr += sizeof(IDF_TYPE) * docEntry.mWordIdsQty;
    docEntry.mpLuceneIDF   = reinterpret_cast<const IDF_TYPE*>(pBufPtr);
    pBufPtr += sizeof(IDF_TYPE) * docEntry.mWordIdsQty;

    docEntry.mpQtys        = reinterpret_cast<const QTY_TYPE*>(pBufPtr);
    pBufPtr += sizeof(QTY_TYPE) * docEntry.mWordIdsQty;

    docEntry.mpWordIdSeq   = reinterpret_cast<const WORD_ID_TYPE*>(pBufPtr);
    pBufPtr += sizeof(WORD_ID_TYPE) * docEntry.mWordIdSeqQty;

    if (docEntry.mWordEmbedDim) {
#ifdef USE_NON_IDF_AVG_EMBED
      docEntry.mpRegAvgWordEmbed = reinterpret_cast<const float*>(pBufPtr);
      pBufPtr += sizeof(float) * docEntry.mWordEmbedDim;
#endif
      docEntry.mpIDFWeightAvgWordEmbed = reinterpret_cast<const float*>(pBufPtr);
      pBufPtr += sizeof(float) * docEntry.mWordEmbedDim;
    }
#ifdef PRECOMPUTE_TRAN_TABLES
    docEntry.mTranEntryQty = pHeader->mTranRecQty;

    if (docEntry.mTranEntryQty >= 0) {
      docEntry.mpTranWordIds = reinterpret_cast<const WORD_ID_TYPE*>(pBufPtr);
      pBufPtr += sizeof(WORD_ID_TYPE) * pHeader->mTranRecQty;
      docEntry.mpTranEntries = reinterpret_cast<const OneTranEntryShort*>(pBufPtr);
      pBufPtr += sizeof(OneTranEntryShort) * pHeader->mTranRecQty;
    }
#endif
  }


  unique_ptr<SpaceParamQA1> mSpaceParams;
  mutable bool              mDontPrecomputeFlag = false;

  DISABLE_COPY_AND_ASSIGN(SpaceQA1);
};


}  // namespace similarity

#endif 
