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
#ifndef _WAND_INV_INDEX_H_
#define _WAND_INV_INDEX_H_

#include "simple_inverted_index.h"

#define METH_WAND_INV_INDEX             "wand_invindx"

namespace similarity {

using std::string;

template <typename dist_t>
class WandInvIndex : public SimplInvIndex<dist_t> {
 public:
  /*
   * The constructor here space and data-objects' references,
   * which are guaranteed to be be valid during testing.
   * So, we can memorize them safely.
   */
  WandInvIndex(bool printProgress,
               Space<dist_t>& space,
               const ObjectVector& data) : SimplInvIndex<dist_t>(printProgress, space, data) {
  }

  void CreateIndex(const AnyParams& IndexParams) override; 

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override;

  ~WandInvIndex() override;

  void Search(KNNQuery<dist_t>* query, IdType) const override;


  const string StrDesc() const override {
    return METH_WAND_INV_INDEX;
  }

 protected:

  // a protected creator that has already a created parameter manager
  void CreateIndex(AnyParamManager& ParamManager);

  typedef typename SimplInvIndex<dist_t>::PostList PostList;
  typedef typename SimplInvIndex<dist_t>::PostEntry PostEntry;

  /**
   * A structure that keeps information about current state of search within one posting list (for WAND).
   */
  struct PostListQueryStateWAND {
    // pointer to the posting list (fixed from the beginning)
    const PostList*  post_;
    // value of the respective term in the query (fixed from the beginning)
    const dist_t           qval_;
    // product of the value in query and MAX contribution for given term
    const dist_t max_term_contr_;

    // actual position in the list
    size_t           post_pos_;

    PostListQueryStateWAND(const PostList& pl, const dist_t qval, const dist_t max_term_contr)
        : post_(&pl), qval_(qval), max_term_contr_(max_term_contr), post_pos_(0) {}
  };

  // the maximal theoretical contributions of each term (max over values in the posting lists)
  std::unordered_map<unsigned, dist_t>  max_contributions_;


  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(WandInvIndex);
};

}   // namespace similarity

#endif     // _WAND_INV_INDEX_H_
