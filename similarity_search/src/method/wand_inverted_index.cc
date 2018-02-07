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
#include <unordered_map>
#include <queue>

#include "space.h"
#include "knnquery.h"
#include "method/wand_inverted_index.h"
#include "falconn_heap_mod.h"

#define SANITY_CHECKS

namespace similarity {

using namespace std;


template <typename dist_t>
void WandInvIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  // the query vector, its size is the number of query terms (non-zero dimensions of the query vector)
  vector<SparseVectElem<dist_t>>    query_vect;
  const Object* o = query->QueryObject();
  UnpackSparseElements(o->data(), o->datalength(), query_vect);

  size_t K = query->GetK();

  // sorted list (priority queue) of pairs (doc_id, its_position_in_the_posting_list)
  //   the doc_ids are negative to keep the queue ordered the way we need
  FalconnHeapMod1<IdType, int32_t>            postListQueue;
  // state information for each query-term posting list WAND
  vector<unique_ptr<PostListQueryStateWAND>>      queryStates(query_vect.size());

  // number of valid query terms (that are in the dictionary)
  size_t wordQty = 0;
  // query term index
  unsigned qsi = 0;
  // initialize queryStates and postListQueue variables
  for (auto eQuery : query_vect) {
    auto it = SimplInvIndex<dist_t>::index_.find(eQuery.id_);
    if (it != SimplInvIndex<dist_t>::index_.end()) { // There may be out-of-vocabulary words
#ifdef SANITY_CHECKS
      CHECK(it->second.get() != nullptr);
#endif
      const PostList& pl = *it->second;
      CHECK(pl.qty_ > 0);
      ++wordQty;
      // initialize the queryStates[query_term_index]  to the first position in the posting list WAND
      dist_t maxContrib = eQuery.val_ * max_contributions_.find(eQuery.id_)->second;
      queryStates[qsi].reset(new PostListQueryStateWAND(pl, eQuery.val_, maxContrib));
      // initialize the postListQueue to the first position - insert pair (-doc_id, query_term_index)
      postListQueue.push(-pl.entries_[0].doc_id_, qsi);
    }
    ++qsi;
  }

  // While some people expect the result set to always contain at least k entries,
  // it's not clear what we return here
  if (0 == wordQty) return;

  // temporary queue with the top-K results  (ordered by the accumulated values so that top value is the smallest=worst document)
  FalconnHeapMod1<dist_t, IdType>             tmpResQueue;

  // accumulation of PostListQueryState.qval_x_docval_ for each document (DAAT)
  dist_t   accum = 0;
  // accumulation of the PostListQueryStateWAND.max_term_contr_
  dist_t   max_contrib_accum = 0;

  vector<unsigned> lowest_doc_indexes(wordQty);
  int lowest_doc_count = 0;

  while (!postListQueue.empty()) {
    // index of the posting list with the current SMALLEST doc_id
    IdType minDocIdNeg = postListQueue.top_key();

    // this while accumulates the THRESHOLD values for the one document (DAAT), specifically for the one with   doc_id = -minDocIdNeg
    while (!postListQueue.empty() && postListQueue.top_key() == minDocIdNeg) {
      unsigned qsi = postListQueue.top_data();
      lowest_doc_indexes[lowest_doc_count++] = qsi;
      max_contrib_accum += queryStates[qsi]->max_term_contr_;

      PostListQueryStateWAND& queryState = *queryStates[qsi];
      const PostList& pl = *queryState.post_;

      // move to next position in the posting list
      queryState.post_pos_++;
      if (queryState.post_pos_ < pl.qty_) {
        // the next entry in the posting list
        postListQueue.replace_top_key(- pl.entries_[queryState.post_pos_].doc_id_);
        //queryState.qval_x_docval_ = eDoc.val_ * queryState.qval_;
      } else postListQueue.pop();
    }

    // if the potential maximal contribution of this doc_id is larger than current top-k limit
    bool fully_evaluate = (tmpResQueue.size() < K) || (tmpResQueue.top_key() > -max_contrib_accum);

    if (fully_evaluate) {
      // this while accumulates values for one document (DAAT), if its threshold
      for (int i = 0; i < lowest_doc_count; ++i) {
        PostListQueryStateWAND &queryState = *queryStates[lowest_doc_indexes[i]];
        // const PostList &pl = *queryState.post_;
        accum += queryState.qval_ * (*queryState.post_).entries_[queryState.post_pos_ - 1].val_;
      }
      dist_t negAccum = -accum;

      // This one seems to be a bit faster
      // DAVID: THIS CONSTRUCTION IS WRONG. DUE TO THE FIRST CONDITION, THERE MIGHT BE MORE THAN K DOCUMENTS IN THE
      // RESULT, RIGHT? BUT THE SECOND FORK ONLY REPLACES ONE OF THE "WORST" DOCUMENTS
      if (tmpResQueue.size() < K || tmpResQueue.top_key() == negAccum)
        tmpResQueue.push(negAccum, -minDocIdNeg);
      else if (tmpResQueue.top_key() > negAccum)
        tmpResQueue.replace_top(negAccum, -minDocIdNeg);
    }
    accum = 0;
    max_contrib_accum = 0;
    lowest_doc_count = 0;
  }

  while (!tmpResQueue.empty()) {
#ifdef SANITY_CHECKS
    CHECK(tmpResQueue.top_data() >= 0);
#endif

#if 0
    query->CheckAndAddToResult(-tmpResQueue.top_key(), SimplInvIndex<dist_t>::data_[tmpResQueue.top_data()]);
#else
    // This branch recomputes the distance, but it normally has a negligibly small effect on the run-time
    query->CheckAndAddToResult(SimplInvIndex<dist_t>::data_[tmpResQueue.top_data()]);
#endif
    tmpResQueue.pop();
  }
}

template <typename dist_t>
void WandInvIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);
  CreateIndex(pmgr);
}

template <typename dist_t>
void WandInvIndex<dist_t>::CreateIndex(AnyParamManager& ParamManager) {
  // create the index first
  SimplInvIndex<dist_t>::CreateIndex(ParamManager);

  for (const auto& dictEntry : SimplInvIndex<dist_t>::index_) {
    dist_t termMax = 0;
    unsigned qty = dictEntry.second->qty_;
    PostEntry * entries = dictEntry.second->entries_;
    for (int i = 0; i < qty; ++i) {
      if (termMax < entries[i].val_) {
        termMax = entries[i].val_;
      }
    }
    max_contributions_.insert(make_pair(dictEntry.first, termMax));
  }
}

template <typename dist_t>
WandInvIndex<dist_t>::~WandInvIndex() {
// nothing here yet
}

template <typename dist_t>
void
WandInvIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  SimplInvIndex<dist_t>::SetQueryTimeParams(QueryTimeParams);
}


template class WandInvIndex<float>;

}
