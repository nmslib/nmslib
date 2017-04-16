/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/) and others.
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2016
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _SIMPLE_INV_INDEX_H_QA1_
#define _SIMPLE_INV_INDEX_H_QA1_

#include <string>
#include <sstream>
#include <unordered_map>
#include <memory>

#include "index.h"
#include "space/qa/space_qa1.h"

#define METH_SIMPLE_INV_INDEX_QA1             "simple_invindx_qa1"

namespace similarity {

using std::string;

template <typename dist_t>
class SimplInvIndexQA1 : public Index<dist_t> {
 public:
  /*
   * The constructor here space and data-objects' references,
   * which are guaranteed to be be valid during testing.
   * So, we can memorize them safely.
   */
  SimplInvIndexQA1(Space<dist_t>& space, 
              const ObjectVector& data) : data_(data),
                                          pSpace_(dynamic_cast<SpaceQA1*>(&space)) {
    if (pSpace_ == nullptr) {
      PREPARE_RUNTIME_ERR(err) <<
          "The method " << StrDesc() << " works only with the space " << SPACE_QA1;
      THROW_RUNTIME_ERR(err);
    }
  }

  void CreateIndex(const AnyParams& IndexParams) override; 

  /*
   * One can implement functions for index serialization and reading.
   * However, this is not required.
   */
  // SaveIndex is not necessarily implemented
  virtual void SaveIndex(const string& location) override {
    throw runtime_error("SaveIndex is not implemented for method: " + StrDesc());
  }
  // LoadIndex is not necessarily implemented
  virtual void LoadIndex(const string& location) override {
    throw runtime_error("LoadIndex is not implemented for method: " + StrDesc());
  }

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;

  ~SimplInvIndexQA1() override;

  const std::string StrDesc() const override { 
    return METH_SIMPLE_INV_INDEX_QA1;
  }

  void Search(RangeQuery<dist_t>* query, IdType) const override { 
    throw runtime_error("Range search is not supported!");
  }
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  virtual bool DuplicateData() const override { return false; }

 protected:

  // a protected creator that has already a created parameter manager
  void CreateIndex(AnyParamManager& ParamManager);

  struct PostEntry {
    IdType   doc_id_; // IdType is signed
    dist_t   val_;
    PostEntry(int32_t doc_id = 0, dist_t val = 0) : doc_id_(doc_id), val_(val) {}
  };
  struct PostList {
    // Variables are const, so that they can be initialized only in constructor
    // However, the memory that entries_ point to isn't const and can be modified
    const size_t     qty_;
    PostEntry* const entries_;
    PostList(size_t qty) :
        qty_(qty),
        entries_(new PostEntry[qty]) {
    }
    ~PostList() {
      delete [] entries_;
    }
  };

  /**
   * A structure that keeps information about current state of search within one posting list.
   */
  struct PostListQueryState {
    // pointer to the posting list (fixed from the beginning)
    const PostList*  post_;
    // actual position in the list
    size_t           post_pos_;
    // value of the respective term in the query (fixed from the beginning)
    const dist_t     qval_;
    // product of the values in query in the document (for given term)
    dist_t           qval_x_docval_;

    PostListQueryState(const PostList& pl, dist_t qval, dist_t qval_x_docval)
        : post_(&pl), post_pos_(0), qval_(qval), qval_x_docval_(qval_x_docval) {}
  };

  const ObjectVector&                                      data_;
  SpaceQA1*                                                pSpace_;
  std::unordered_map<unsigned, std::unique_ptr<PostList>>  index_;
  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(SimplInvIndexQA1);
};

}   // namespace similarity

#endif     
