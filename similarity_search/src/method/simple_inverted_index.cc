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

#include <unordered_map>
#include <queue>

#include "space.h"
#include "knnquery.h"
#include "method/simple_inverted_index.h"
#include "falconn_heap_mod.h"

#define SANITY_CHECKS

namespace similarity {

using namespace std;


template <typename dist_t>
void SimplInvIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  vector<SparseVectElem<dist_t>>    query_vect;
  const Object* o = query->QueryObject();
  UnpackSparseElements(o->data(), o->datalength(), query_vect);

  size_t K = query->GetK();

  FalconnHeapMod1<dist_t, IdType>             tmpResQueue;
  FalconnHeapMod1<IdType, int32_t>            postListQueue;
  vector<unique_ptr<PostListQueryState>>      queryStates(query_vect.size());

  size_t wordQty = 0;
  for (auto e : query_vect) {
    auto it = index_.find(e.id_);
    if (it != index_.end()) { // There may be out-of-vocabulary words
      const PostList& pl = *it->second;
      CHECK(pl.qty_ > 0);
      ++wordQty;
    }
  }

  // While some people expect the result set to always contain at least k entries,
  // it's not clear what we return here
  if (0 == wordQty) return;

  unsigned qsi = 0;
  for (auto eQuery : query_vect) {
    uint32_t wordId = eQuery.id_;
    auto it = index_.find(wordId);
    if (it != index_.end()) { // There may be out-of-vocabulary words
#ifdef SANITY_CHECKS
      CHECK(it->second.get() != nullptr);
#endif
      const PostList& pl = *it->second;
      CHECK(pl.qty_ > 0);

      queryStates[qsi].reset(new PostListQueryState(pl, eQuery.val_, eQuery.val_ * pl.entries_[0].val_));
      postListQueue.push(-pl.entries_[0].doc_id_, qsi);
      ++wordQty;
    }
    ++qsi;
  }

  dist_t   accum = 0; //

  while (!postListQueue.empty()) {
    IdType minDocIdNeg = postListQueue.top_key();

    while (!postListQueue.empty() && postListQueue.top_key() == minDocIdNeg) {
      unsigned qsi = postListQueue.top_data();
      PostListQueryState& queryState = *queryStates[qsi];
      const PostList& pl = *queryState.post_;
      accum += queryState.qval_x_docval_;
      //accum += queryState.qval_ * pl.entries_[queryState.post_pos_].val_;
      queryState.post_pos_++;
      /*
       * If we didn't reach the end of the posting list, we retrieve the next document id.
       * Then, we push this update element down the priority queue.
       *
       * On reaching the end of the posting list, we evict the entry from the priority queue.
       */
      if (queryState.post_pos_ < pl.qty_) {
        const auto& eDoc = pl.entries_[queryState.post_pos_];
        /*
         * Leo thinks it may be beneficial to access the posting list entry only once.
         * This access is used for two things
         * 1) obtain the next doc id
         * 2) compute the contribution of the current document to the overall dot product (val_ * qval_)
         */
        postListQueue.replace_top_key(-eDoc.doc_id_);
        queryState.qval_x_docval_ = eDoc.val_ * queryState.qval_;
      } else postListQueue.pop();
    }

    // tmpResQueue is a MAX-QUEUE (which is what we need, b/c we maximize the dot product
    dist_t negAccum = -accum;
#if 1
    // This one seems to be a bit faster
    if (tmpResQueue.size() < K || tmpResQueue.top_key() == negAccum)
      tmpResQueue.push(negAccum, -minDocIdNeg);
    else if (tmpResQueue.top_key() > negAccum)
      tmpResQueue.replace_top(-accum, -minDocIdNeg);
#else
    query->CheckAndAddToResult(negAccum, data_[-minDocIdNeg]);
#endif

    accum = 0;
  }

  while (!tmpResQueue.empty()) {
#ifdef SANITY_CHECKS
    CHECK(tmpResQueue.top_data() >= 0);
#endif

#if 0
    query->CheckAndAddToResult(-tmpResQueue.top_key(), data_[tmpResQueue.top_data()]);
#else
    // This branch recomputes the distance, but it normally has a negligibly small effect on the run-time
    query->CheckAndAddToResult(data_[tmpResQueue.top_data()]);
#endif
    tmpResQueue.pop();
  }
}

template <typename dist_t>
void SimplInvIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager  pmgr(IndexParams);
  pmgr.CheckUnused();
  // Always call ResetQueryTimeParams() to set query-time parameters to their default values
  this->ResetQueryTimeParams();

  // Let's first calculate memory requirements
  unordered_map<unsigned, size_t>   dict_qty;
  vector<SparseVectElem<dist_t>>    tmp_vect;
  LOG(LIB_INFO) << "Collecting dictionary stat";

  for (const Object* o : data_) {
    tmp_vect.clear();
    UnpackSparseElements(o->data(), o->datalength(), tmp_vect);
    for (const auto& e : tmp_vect) dict_qty[e.id_] ++;
  }

  // Create posting-list place holders
  unordered_map<unsigned, size_t> post_pos;
  LOG(LIB_INFO) << "Actually creating the index";
  for (const auto dictEntry : dict_qty) {
    unsigned wordId = dictEntry.first;
    size_t   qty = dictEntry.second;
    post_pos.insert(make_pair(wordId, 0));
    index_.insert(make_pair(wordId, unique_ptr<PostList>(new PostList(qty))));
  }

  // Fill posting lists
  for (size_t did = 0; did < data_.size(); ++did) {
    tmp_vect.clear();
    UnpackSparseElements(data_[did]->data(), data_[did]->datalength(), tmp_vect);
    for (const auto& e : tmp_vect) {
      const auto wordId = e.id_;
      auto itPost     = index_.find(wordId);
      auto itPostPos  = post_pos.find(wordId);
#ifdef SANITY_CHECKS
      CHECK(itPost != index_.end());
      CHECK(itPostPos != post_pos.end());
      CHECK(itPost->second.get() != nullptr);
#endif
      PostList& pl = *itPost->second;
      size_t curr_pos = itPostPos->second++;;
      CHECK(curr_pos < pl.qty_);
      pl.entries_[curr_pos] = PostEntry(did, e.val_);
    }
  }
#ifdef SANITY_CHECKS
  // Sanity check
  LOG(LIB_INFO) << "Sanity check";
  for (const auto dictEntry : dict_qty) {
    unsigned wordId = dictEntry.first;
    size_t qty = dictEntry.second;
    CHECK(qty == post_pos[wordId]);
  }
#endif
}

template <typename dist_t>
SimplInvIndex<dist_t>::~SimplInvIndex() {
// nothing here yet
}

template <typename dist_t>
void 
SimplInvIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  // Check if a user specified extra parameters, which can be also misspelled variants of existing ones
  AnyParamManager pmgr(QueryTimeParams);
  int dummy;
  // Note that GetParamOptional() should always have a default value
  pmgr.GetParamOptional("dummyParam", dummy, -1);
  LOG(LIB_INFO) << "Set dummy = " << dummy;
  pmgr.CheckUnused();
}


template class SimplInvIndex<float>;

}
