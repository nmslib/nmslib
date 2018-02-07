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
#include <stdexcept>

#include "space.h"
#include "knnquery.h"
#include "method/blkmax_inverted_index.h"
#include "falconn_heap_mod.h"

#define SANITY_CHECKS

namespace similarity {

using namespace std;


template <typename dist_t>
void BlockMaxInvIndex<dist_t>::Search(KNNQuery<dist_t>* query, IdType) const {
  try {
//  LOG(LIB_INFO) << "Starting new kNN query";
  // the query vector, its size is the number of query terms (non-zero dimensions of the query vector)
  vector<SparseVectElem<dist_t>>    query_vect;
  const Object* o = query->QueryObject();
  UnpackSparseElements(o->data(), o->datalength(), query_vect);

  size_t K = query->GetK();

  // sorted list (priority queue) of pairs (doc_id, index_into_queryStates)
  //   the doc_ids are negative
  FalconnHeapMod1<IdType, int32_t>            postListQueue;
  // state information for each query-term posting list WAND
  vector<unique_ptr<PostListQueryStateBlock>>      queryStates(query_vect.size());

  // number of valid query terms (that are in the dictionary)
  size_t wordQty = 0;
  // query term index
  int32_t qsi = 0;
  // initialize queryStates and postListQueue variable
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
      dist_t maxContrib = eQuery.val_ * WandInvIndex<dist_t>::max_contributions_.find(eQuery.id_)->second;

      // the list of blocks for given posting list
      vector<BlockInfo *> & blocks_ = *(blocks_map_.find(eQuery.id_)->second);
      queryStates[qsi].reset(new PostListQueryStateBlock(pl, eQuery.val_, maxContrib, block_size_, blocks_, eQuery.id_));

      // initialize the postListQueue to the first position - insert pair (-doc_id, query_term_index)
      postListQueue.push(-queryStates[qsi]->doc_id_, qsi);
    }
    ++qsi;
  }

  // While some people expect the result set to always contain at least k entries,
  // it's not clear what we return here
  if (0 == wordQty) return;

  // temporary queue with the top-K results  (ordered by the accumulated values so that top value is the smallest=worst document)
  FalconnHeapMod1<dist_t, IdType>             tmpResQueue;
  // query threshold (positive)
  dist_t queryThreshold = 0;

  // accumulation of PostListQueryState.qval_x_docval_ for each document (DAAT)
  dist_t   accum = 0;
  // accumulation of the PostListQueryStateWAND.max_term_contr_
  dist_t   max_contrib_accum = 0;
  dist_t   max_block_contrib_accum = 0;

  // list of indexes into index_into_queryStates of the lowest doc_ids up to the pivot (inclusive)
  std::vector<unsigned> lowest_doc_indexes(wordQty);
  int pivotIdx = 0;
  IdType pivot_doc_id_neg = 1;

//  LOG(LIB_INFO) << "\tinitialization OK, # of query terms " << wordQty;
  while (!postListQueue.empty()) {
    // find the WAND-like pivot (in case of having IDs like (3, 5, 7, 7, 8) and in case sum of max
    string top_id_str = "(";
    while (!postListQueue.empty() &&
           (max_contrib_accum <= queryThreshold || pivot_doc_id_neg == postListQueue.top_key())) {
      postListQueue.extract_top(pivot_doc_id_neg, qsi);
      lowest_doc_indexes[pivotIdx++] = qsi;
      max_contrib_accum += queryStates[qsi]->max_term_contr_;
      top_id_str += to_string(- pivot_doc_id_neg) + ", ";
    }
    IdType pivot_doc_id = -pivot_doc_id_neg;
//    LOG(LIB_INFO) << "\ttop doc_ids: " << top_id_str << "  " << (postListQueue.empty() ? "" : to_string(- postListQueue.top_key()))
//        << "), " << "\tpivot index " << pivotIdx << ", pivot doc id " << pivot_doc_id;

    // shift blocks, if necessary, and calculate max_block_contribution
    bool someListEnded = false;
    for (int i = 0; i < pivotIdx; ++i) {
      try {
        PostListQueryStateBlock &queryState = *queryStates[lowest_doc_indexes[i]];
        max_block_contrib_accum += queryState.NextShallow(pivot_doc_id);
      } catch (const std::length_error &e) {
        // if the shallow next reaches the end of the posting list, do not contribute to the accumulation
//        LOG(LIB_INFO) << "\tsome list ended";
        someListEnded = true;
      }
    }
//    LOG(LIB_INFO) << "\t\tmax contribution: " << max_contrib_accum << ", max block contrib: " << max_block_contrib_accum << " (threshold " << queryThreshold << ")";

    // if the sum of the block maximums does not exceed the threshold, we just shift all the pointers significantly
    if (max_block_contrib_accum <= queryThreshold) {
      // find new doc_id to shift all inspected lists (up to the pivot)
      IdType new_doc_id = postListQueue.empty() ? MAX_DATASET_QTY : - postListQueue.top_key();
      for (int i = 0; i < pivotIdx; ++i) {
        try {
          if (someListEnded) { // do the block shift once again, because the ended list will throw an exception
            queryStates[lowest_doc_indexes[i]]->NextShallow(pivot_doc_id);
          }
          IdType blockBoundaryPlusOne = queryStates[lowest_doc_indexes[i]]->getBlockLastId() + 1;
          if (new_doc_id > blockBoundaryPlusOne) {
            new_doc_id = blockBoundaryPlusOne;
          }
        } catch (const std::length_error &e) {
          // skip the post list that ended
//          LOG(LIB_INFO) << "\t\tNextShallow throws exception";
        }
      }
//      LOG(LIB_INFO) << "\t\tjust shifting pointers to: " << new_doc_id;
      // shift pointers to the next id and re-insert them into the queue
      for (int i = 0; i < pivotIdx; ++i) {
//        LOG(LIB_INFO) << "\t\t\tlowest_doc_indexes[i]: " << lowest_doc_indexes[i];
//        LOG(LIB_INFO) << "\t\t\tqueryStates[lowest_doc_indexes[i]] - is null: " << (queryStates[lowest_doc_indexes[i]] == NULL);
        PostListQueryStateBlock &queryState = *queryStates[lowest_doc_indexes[i]];
        try {
          queryState.Next(new_doc_id, true);
          postListQueue.push(-queryState.doc_id_, lowest_doc_indexes[i]);
        } catch (const std::length_error &e) {
          // if the shallow next reaches the end of the posting list, do not reinsert this list into the queue
//          LOG(LIB_INFO) << "\t\tNext throws exception";
        }
      }
    } else { // we check the actual values
      // make sure that all pointers point to doc_ids >= pivot_doc_id
      for (int i = 0; i < pivotIdx; ++i) {
        PostListQueryStateBlock &queryState = *queryStates[lowest_doc_indexes[i]];
        try {
          // if this posting list contains the pivot_doc_id
          if (queryState.Next(pivot_doc_id, false)) {
            accum += queryState.GetCurrentQueryVal();
            postListQueue.push(-queryState.Next(), lowest_doc_indexes[i]);
          } else { // this means that the pointer in this posting list is larger than the pivot doc_id
            postListQueue.push(-queryState.doc_id_, lowest_doc_indexes[i]);
          }
        } catch (const std::length_error &e) {
          // if the shallow next reaches the end of the posting list, do not reinsert this list into the queue
//          LOG(LIB_INFO) << "\t\tNext throws exception";
        }
      }

      dist_t negAccum = -accum;
      // This one seems to be a bit faster
      // DAVID: THIS CONSTRUCTION IS WRONG. DUE TO THE FIRST CONDITION, THERE MIGHT BE MORE THAN K DOCUMENTS IN THE
      // RESULT, RIGHT? BUT THE SECOND FORK ONLY REPLACES ONE OF THE "WORST" DOCUMENTS
      if (tmpResQueue.size() < K || tmpResQueue.top_key() == negAccum)
        tmpResQueue.push(negAccum, pivot_doc_id);
      else if (tmpResQueue.top_key() > negAccum) {
        tmpResQueue.replace_top(negAccum, pivot_doc_id);
        queryThreshold = -tmpResQueue.top_key();
      }
    }

    accum = 0;
    max_contrib_accum = 0;
    max_block_contrib_accum = 0;
    pivotIdx = 0;
    pivot_doc_id_neg = 1;
//    LOG(LIB_INFO) << "\t\tEnd of loop, size of post list queue: " << postListQueue.size();
  }

//  LOG(LIB_INFO) << "\tOut of the main loop";
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
  } catch (const std::exception &e) {
    LOG(LIB_INFO) << "\t\t\tthe processing threw exception: " << e.what();
  }
}

template <typename dist_t>
void BlockMaxInvIndex<dist_t>::CreateIndex(const AnyParams& IndexParams) {
  AnyParamManager pmgr(IndexParams);
  // read the parameter with block size
  pmgr.GetParamOptional<int,int>(PARAM_BLOCK_SIZE, block_size_, PARAM_BLOCK_SIZE_DEFAULT);

  // create the index and initialize the maximum values over all entries
  WandInvIndex<dist_t>::CreateIndex(pmgr);

  LOG(LIB_INFO) << "creating blocks";
  for (const auto& dictEntry : SimplInvIndex<dist_t>::index_) {
    vector<BlockInfo * > * blocks = new vector<BlockInfo * >();
//    string entryString = "list " + to_string(dictEntry.first) + " [";
//    string blockString = "[";
    unsigned qty = dictEntry.second->qty_;
    PostEntry * entries = dictEntry.second->entries_;
    dist_t termMax = 0;
    for (int i = 0; i < qty; ++i) {
      if (termMax < entries[i].val_) {
        termMax = entries[i].val_;
      }
//      entryString += "(" + to_string(entries[i].doc_id_) + " " + to_string(entries[i].val_) + ") ";
      // if we are ending a block
      if ((i + 1) % block_size_ == 0) {
        blocks->push_back(new BlockInfo(entries[i].doc_id_, termMax));
        termMax = 0;
//        entryString += "], [";
//        blockString += to_string((*blocks)[blocks->size() - 1]->last_id) + " " + to_string((*blocks)[blocks->size() - 1]->max_val) + "], [";
      }
    }
    // the last block
    if (qty % block_size_ != 0) {
      blocks->push_back(new BlockInfo(entries[qty - 1].doc_id_, termMax));
//      blockString += to_string((*blocks)[blocks->size() - 1]->last_id) + " " + to_string((*blocks)[blocks->size() - 1]->max_val) + "]";
    }
//    LOG(LIB_INFO) << entryString;
//    LOG(LIB_INFO) << blockString;

    blocks_map_.insert(make_pair(dictEntry.first, unique_ptr<vector<BlockInfo * >>(blocks)));
  }
}

template <typename dist_t>
BlockMaxInvIndex<dist_t>::~BlockMaxInvIndex() {
// nothing here yet
}

template <typename dist_t>
void
BlockMaxInvIndex<dist_t>::SetQueryTimeParams(const AnyParams& QueryTimeParams) {
  WandInvIndex<dist_t>::SetQueryTimeParams(QueryTimeParams);
}


template class BlockMaxInvIndex<float>;

}
