/* 
    Copyright (C) 2008 Wei Dong <wdong@princeton.edu>. All Rights Reserved.
  
    This file is part of LSHKIT.
  
    LSHKIT is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    LSHKIT is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with LSHKIT.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef __LSHKIT_FOREST__
#define __LSHKIT_FOREST__

/**
  * \file forest.h
  * \brief A preliminary implementation of LSH Forest index.
  *
  * This is a preliminary implementation of main memory LSH Forest index.
  * The implementation is largely based on the WWW'05 LSH Forest paper.
  * The descend and synchascend algorithms are implemented in a 
  * different but equivalent way, so the candidate set do not have to
  * be explicitly generated.  I also made a simplification to the synchascending
  * algorithm, so that the original line
  \code
    while (x > 0 and (|P| < cl or |distinct(P)| < m)) {
  \endcode
  * is simplified to
  \code
    while (x > 0 and |P| < M) {
  \endcode
  * the deduplication is left to the scanning process.
  *
  * The implementation is not efficient.  The initial goal of this implementation
  * is to study the selectivity of the algorithm.  I'll further optimize the
  * implementation if selectivity is proved competitive.
  * 
  * Reference 
  * 
  * Mayank Bawa , Tyson Condie , Prasanna Ganesan, LSH forest: self-tuning
  * indexes for similarity search, Proceedings of the 14th international
  * conference on World Wide Web, May 10-14, 2005, Chiba, Japan.
  * 
  */

#include <algorithm>
#include <lshkit/common.h>
#include <lshkit/topk.h>

namespace lshkit {

/// LSH Forest index
/**
  * @param LSH LSH class.
  * @param KEY key type.
  */
template <typename LSH, typename KEY>
class ForestIndex
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));
public:
    typedef typename LSH::Parameter Parameter;
    typedef typename LSH::Domain Domain;
    typedef KEY Key;

private:

    struct Tree
    {
        std::vector<LSH> lsh; // the hash functions

        struct Node
        {
            size_t size; // total # points in subtree
            std::vector<Node *> children;
            std::vector<Key> data;

            Node () : size(0) {
            }

            ~Node () {
                BOOST_FOREACH(Node *n, children) {
                    if (n != 0) delete n;
                }
            }

            bool empty () const {
                return size == 0;
            }

            template <typename ACCESSOR>
            void insert (Tree *tree, unsigned depth, Key key, ACCESSOR &acc) {
                ++size;
                if (children.empty()) {
                    data.push_back(key);
                    if (depth < tree->lsh.size() && data.size() > 1) {
                    // split
                        LSH &lsh = tree->lsh[depth];
                        if (lsh.getRange() == 0) throw std::logic_error("LSH WITH UNLIMITED HASH VALUE CANNOT BE USED IN LSH FOREST.");
                        children.resize(lsh.getRange());
                        BOOST_FOREACH(Key key, data) {
                            unsigned h = lsh(acc(key));
                            if (children[h] == 0) {
                                children[h] = new Node();
                            }
                            children[h]->insert(tree, depth+1, key, acc);
                        }
                        data.clear();
                    }
                }
                else {
                    unsigned h = tree->lsh[depth](acc(key));
                    if (children[h] == 0) {
                        children[h] = new Node();
                    }
                    children[h]->insert(tree, depth+1, key, acc);
                }
            }

            template <typename SCANNER>
            void scan (Domain val, SCANNER &scanner) const {
                if (!children.empty()) {
                    BOOST_FOREACH(const Node *n, children) {
                        if (n != 0) {
                            n->scan(val, scanner);
                        }
                    }
                }
                if (!data.empty()) {
                    BOOST_FOREACH(Key key, data) {
                        scanner(key);
                    }
                }
            }
        } *root;

        public:

        Tree (): root(0) {
        }

        template <typename ENGINE>
        void reset (const Parameter &param, ENGINE &engine, unsigned depth)
        {
            lsh.resize(depth);
            BOOST_FOREACH(LSH &h, lsh) {
                h.reset(param, engine);
            }
            root = new Node();
        }

        ~Tree ()
        {
            if (root != 0) delete root;
        }

        template <typename ACCESSOR>
        void insert (Key key, ACCESSOR &acc)
        {
            root->insert(this, 0, key, acc);
        }

        void lookup (Domain val, std::vector<const Node *> *nodes) const {
            const Node *cur = root;
            unsigned depth = 0;
            nodes->clear();
            for (;;) {
                nodes->push_back(cur);
                if (cur->children.empty()) break;
                unsigned h = lsh[depth](val);
                cur = cur->children[h];
                if (cur == 0) break;
                ++depth;
            }
        }
    };

    friend struct Tree;

    std::vector<Tree> trees;
    

public:
    ForestIndex() 
    {
    }

    /// Initialize the LSH Forest index.
    /**
      * @param param LSH parameters.
      * @param engine random number generator.
      * @param L number of trees in the forest.
      * @param depth maximal depth of the forest.
      */
    template <typename Engine>
    void init(const Parameter &param, Engine &engine, unsigned L, unsigned depth) 
    {
        trees.resize(L);
        BOOST_FOREACH(Tree &t, trees) {
            t.reset(param, engine, depth);
        }
    }

    /// Insert a point to the forest.
    /**
      * @param key The key to be inserted.
      * @param acc The accessor to retrieve the data corresponding to keys.
      *
      * We might need to rebalance the trees, so an accessor is
      * needed to retrieve data points.
      */
    template <typename ACCESSOR>
    void insert (Key key, ACCESSOR &acc)
    {
        BOOST_FOREACH(Tree &t, trees) {
            t.insert(key, acc);
        }
    }

    /// Query for K-NNs.
    /**
      * @param val the query object.
      * @param M lower bound of the total number of points to scan.
      * @param scanner the functional object to passed keys to.
      */
    template <typename SCANNER>
    void query (Domain val, unsigned M, SCANNER &scanner) const
    {
        std::vector<std::vector<const typename Tree::Node *> > list(trees.size());
        for (unsigned i = 0; i < trees.size(); ++i) {
            trees[i].lookup(val, &list[i]);
        }
        // Find the minimal depth covering at least M points
        unsigned d = 0;
        for (;;) {
            unsigned s = 0;
            for (unsigned i = 0; i < list.size(); ++i) {
                if (d < list[i].size()) {
                    s += list[i][d]->size;
                }
            }
            if (s < M) break;
            ++d;
        }

        if (d > 0) --d;

        // recursively scan the nodes
        for (unsigned i = 0; i < list.size(); ++i) {
            if (d < list[i].size()) {
                list[i][d]->scan(val, scanner);
            }
        }
    }
};


}

#endif

