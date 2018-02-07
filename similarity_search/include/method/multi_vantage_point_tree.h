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
#ifndef _MULTI_VANTAGE_POINT_TREE_H_
#define _MULTI_VANTAGE_POINT_TREE_H_

#include "index.h"
#include "params.h"

#define METH_MVPTREE                "mvptree"

namespace similarity {

/* 
 * (Binary version of ) Multi-Vantage Point Tree
 * T. Bozkaya and M. Ozsoyoglu,
 * Indexing large metric spaces for similarity search queries
 */

template <typename dist_t>
class Space;

template <typename dist_t>
class MultiVantagePointTree : public Index<dist_t> {
 public:
  MultiVantagePointTree(const Space<dist_t>& space,
                        const ObjectVector& data);
  void CreateIndex(const AnyParams& IndexParams) override;
  ~MultiVantagePointTree();

  const std::string StrDesc() const override;
  void Search(RangeQuery<dist_t>* query, IdType) const override;
  void Search(KNNQuery<dist_t>* query, IdType) const override;

  void SetQueryTimeParams(const AnyParams& QueryTimeParams) override {
    AnyParamManager pmgr(QueryTimeParams);
    pmgr.GetParamOptional("maxLeavesToVisit", MaxLeavesToVisit_, FAKE_MAX_LEAVES_TO_VISIT);
    LOG(LIB_INFO) << "Set MVP-tree query-time parameters:";
    LOG(LIB_INFO) << "maxLeavesToVisit" << MaxLeavesToVisit_;
    pmgr.CheckUnused();
  }

  virtual bool DuplicateData() const override { return ChunkBucket_; }
 private:
  struct Entry;

  typedef std::vector<dist_t> Dists;
  typedef std::vector<Entry> Entries;

  struct Entry {
    const Object* object;
    Dists path;
    dist_t d1;
    dist_t d2;
    explicit Entry(const Object* object) : object(object) {}
  };

  struct Dist1AscComparator {
    bool operator()(const Entry& e1, const Entry& e2) const {
      return e1.d1 < e2.d1;
    }
  };

  struct Dist2AscComparator {
    bool operator()(const Entry& e1, const Entry& e2) const {
      return e1.d2 < e2.d2;
    }
  };

  class Node {
   public:
    Node(const Object* pivot1, const Object* pivot2, const bool isleaf)
      : pivot1_(pivot1),
        pivot2_(pivot2),
        isleaf_(isleaf) {}
    virtual ~Node() {}
    inline bool isleaf() const { return isleaf_; }

   protected:
    const Object* pivot1_;
    const Object* pivot2_;
    const bool isleaf_;
    friend class MultiVantagePointTree;
  };

  class InternalNode : public Node {
   public:
    InternalNode(const Object* pivot1,
                 const Object* pivot2,
                 dist_t m1,
                 dist_t m21,
                 dist_t m22)
      : Node(pivot1, pivot2, false),
        m1_(m1),
        m21_(m21),
        m22_(m22),
        child1_(NULL),
        child2_(NULL),
        child3_(NULL),
        child4_(NULL) {}

    ~InternalNode() {
      delete child1_;
      delete child2_;
      delete child3_;
      delete child4_;
    }
   private:
    dist_t m1_;
    dist_t m21_, m22_;
    Node* child1_;
    Node* child2_;
    Node* child3_;
    Node* child4_;
    friend class MultiVantagePointTree;
  };

  class LeafNode : public Node {
   public:
    LeafNode(const Object* pivot1, const Object* pivot2, Entries& entries, bool ChunkBucket)
      : Node(pivot1, pivot2, true),
        entries_(entries), CacheOptimizedBucket_(NULL), bucket_(NULL) {
      if (ChunkBucket && entries.size()) {
        // TODO (@leo) Figure out whether and (why?) chunking does not improve performance
        ObjectVector TmpData(entries.size());

        for (unsigned i = 0; i < entries.size(); ++i) {
          TmpData[i] = entries[i].object;
        }

        CreateCacheOptimizedBucket(TmpData, CacheOptimizedBucket_, bucket_);

        for (unsigned i = 0; i < entries.size(); ++i) {
          entries[i].object = (*bucket_)[i];
        }
      }
    }
    ~LeafNode() {
      ClearBucket(CacheOptimizedBucket_, bucket_);
    }

   private:
    Entries       entries_;
    char*         CacheOptimizedBucket_; 
    ObjectVector* bucket_;
    friend class MultiVantagePointTree;
  };

  Node* BuildTree(const Space<dist_t>* space, Entries& entries);

  template <typename QueryType>
  void GenericSearch(Node* node, QueryType* query, Dists& path, size_t query_path_len, int& MaxLeavesToVisit) const;

  const Space<dist_t>& space_;
  unique_ptr<Node>    root_;               // root node

  size_t MaxPathLength_;   // the number of distances for the data
                             // points to be kept at the leaves (P)
  size_t BucketSize_;     // the maximum fanout for the leaf nodes (K)
  bool   ChunkBucket_;
  int    MaxLeavesToVisit_;


  // disable copy and assign
  DISABLE_COPY_AND_ASSIGN(MultiVantagePointTree);
};

}  // namespace similarity

#endif    // _MULTI_VANTAGE_POINT_TREE_H_

