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

/**
 * \file topk.h
 * \brief Top-K data structures.
 */

#ifndef __LSHKIT_TOPK__
#define __LSHKIT_TOPK__

#include <vector>
#include <limits>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <lshkit/metric.h>

/**
 * \file topk.h
 * \brief Top-K data structure for K-NN search.
 *
 * Usage:
 * \code
 *
 * Topk<unsigned> knn(100); // search for 100-NNs.
 * knn.reset();
 *
 * BOOST_FOREACH(data_point, DATABASE) {
 *      Topk<unsigned>::Element e;
 *      e.key = key of data_point
 *      e.dist = distance(query, data_point);
 *      knn << e;
 * }
 *
 * for (unsigned i = 0; i < knn.size(); ++i) {
 *      cout << knn[i].key << ':' << knn[i].dist << endl;
 * }
 *
 * \endcode
 */

namespace lshkit {

/// Top-K entry.
/**
  * The entry stored in the top-k data structure.  The class Topk is implemented
  * as a heap of TopkEntry.
  */
template <typename KEY>
struct TopkEntry
{
    KEY key;
    float dist;
    bool match (const TopkEntry &e) const { return key == e.key; }
    bool match (KEY e) const { return key == e; }

    TopkEntry (KEY key_, float dist_) : key(key_), dist(dist_) {}

    // XXX original
    //TopkEntry () : dist(std::numeric_limits<float>::max()) { }

    // XXX changed!
    TopkEntry () : key(std::numeric_limits<KEY>::max()), dist(std::numeric_limits<float>::max()) { }

    void reset () { dist = std::numeric_limits<float>::max(); }

    friend bool operator < (const TopkEntry &e1, const TopkEntry &e2)
    {
        return e1.dist < e2.dist;
    }
};

/// Top-K heap.
/**
  * Following is an example of using the Topk class:
  *
  * Topk<Key> topk;
  * topk.reset(k);
  *
  * for each candidate key {
  *     topk << key;
  * }
  *
  * At this point topk should contain the best k keys.
  */
template <class KEY>
class Topk: public std::vector<TopkEntry<KEY> >
{
    unsigned K;
    float R;
    float th;
public:
    typedef TopkEntry<KEY> Element;
    typedef typename std::vector<TopkEntry<KEY> > Base;

    Topk () {}

    ~Topk () {}

    /// Reset the heap.
    void reset (unsigned k, float r = std::numeric_limits<float>::max()) {
        // if (k == 0) throw std::invalid_argument("K MUST BE POSITIVE");
        R = th = r;
        K = k;
        this->resize(k);
        for (typename Base::iterator it = this->begin(); it != this->end(); ++it)
          it->reset();
    }

    void reset (unsigned k, KEY key, float r = std::numeric_limits<float>::max()) {
        // if (k == 0) throw std::invalid_argument("K MUST BE POSITIVE");
        R = th = r;
        K = k;
        this->resize(k);
        for (typename Base::iterator it = this->begin(); it != this->end(); ++it) {
          it->reset();
          it->key = key;
        }
    }

    void reset (float r) {
        K = 0;
        R = th = r;
        this->clear();
    }

    float threshold () const {
        return th;
    }

    /// Insert a new element, update the heap.
    Topk &operator << (Element t)
    {
        if (!(t.dist < th)) return *this;
        if (K == 0) { // R-NN
            this->push_back(t);
            return *this;
        }
        // K-NN
        unsigned i = this->size() - 1;
        unsigned j;
        for (;;)
        {
            if (i == 0) break;
            j = i - 1;

            #if DEBUG_LEVEL > 90
              std::cout << "size " << this->size() << " j " << j;
              std::cout << " t " << t.key;
              //std::cout << " this->at(j) " << this->at(j);
              std::cout << " this->at(j).dist " << this->at(j).dist;
              std::cout << " this->at(j).key " << this->at(j).key;    //XXX
              std::cout << std::endl;
            #endif

            if (this->at(j).match(t)) return *this;
            if (this->at(j) < t) break;
            i = j;
        }
        /* i is the place to insert to */

        j = this->size() - 1;
        for (;;)
        {
            if (j == i) break;
            this->at(j) = this->at(j-1);
            --j;
        }
        this->at(i) = t;
        th = this->back().dist;
        return *this;
    }

    /// Calculate recall.
    /** Recall = size(this /\ topk) / size(this). */
    float recall (const Topk<KEY> &topk /* to be evaluated */) const
    {
        unsigned matched = 0;
        //if (this->size() == 0) return 1.0;
        for (typename Base::const_iterator ii = this->begin(); ii != this->end(); ++ii)
        {
            for (typename Base::const_iterator jj = topk.begin(); jj != topk.end(); ++jj)
            {
                if (ii->match(*jj))
                {
                    matched++;
                    break;
                }
            }
        }
        return float(matched + 1)/float(this->size() + 1);
    }

    unsigned getK () const {
        return K;
    }
};

/// Top-K scanner.
/**
  * Scans keys for top-K query.  This is the object passed into the LSH query interface.
  */
template <typename ACCESSOR, typename METRIC>
class TopkScanner {
public:
    /// Key type.
    typedef typename ACCESSOR::Key Key;
    /// Value type.
    typedef typename ACCESSOR::Value Value;

    /// Constructor.
    /**
      * @param accessor The scanner use accessor to retrieva values from keys.
      * @param metric The distance metric.
      * @param K value used to reset internal Topk class.
      * @param R value used to reset internal Topk class.
      *
      * (ACCESSOR)accessor is used as a function.  Given a key, accessor(key)
      * returns an object (or reference) of the type LSH::Domain. That object is
      * used to calculate a hash value.  The key is saved in the hash table.
      * When the associated object is needed, e.g. when scanning a bin,
      * accessor(key) is called to access the object.
      *
      * Metric metric is also used as a function.  It accepts two parameters of
      * the type LSH::Domain and returns the distance between the two.
      */
    TopkScanner(const ACCESSOR &accessor, const METRIC &metric, unsigned K, float R = std::numeric_limits<float>::max())
        : accessor_(accessor), metric_(metric), K_(K), R_(R) {
    }

    /// Reset the query.
    /**
      * This function should be invoked before each query.
      */
    void reset (Value query) {
        query_ = query;
        accessor_.reset();
        topk_.reset(K_, R_);
        cnt_ = 0;
    }

    /// Number of points scanned for the current query.
    unsigned cnt () const {
        return cnt_;
    }

    /// TopK results.
    const Topk<Key> &topk () const {
        return topk_;
    }

    /// TopK results.
    Topk<Key> &topk () {
        return topk_;
    }

    /// Update the current query by scanning key.
    /**
      * This is normally invoked by the LSH index structure.
      */
    void operator () (Key key) {
        #if DEBUG_LEVEL > 50
          std::cerr << "topkscanner operator() " << key << std::endl;
        #endif

        if (accessor_.mark(key)) {
            ++cnt_;
            topk_ << typename Topk<Key>::Element(key, metric_(query_, accessor_(key)));
        }
    }
private:
    ACCESSOR accessor_;
    METRIC metric_;
    unsigned K_;
    float R_;
    Topk<Key> topk_;
    Value query_;
    unsigned cnt_;
};

/**
  * Specialized for l2sqr.
  */
template <typename ACCESSOR>
class TopkScanner <ACCESSOR, metric::l2sqr<float> >{
public:
    typedef typename ACCESSOR::Key Key;
    typedef const float *Value;

    TopkScanner(const ACCESSOR &accessor, const metric::l2sqr<float> &metric, unsigned K, float R = std::numeric_limits<float>::max())
        : accessor_(accessor), dim_(metric.dim()), K_(K), R_(R) {
    }

    void reset (const float *query) {
        query_ = query;
        accessor_.reset();
        topk_.reset(K_, R_);
        cnt_ = 0;
    }

    unsigned cnt () const {
        return cnt_;
    }

    const Topk<Key> &topk () const {
        return topk_;
    }

    Topk<Key> &topk () {
        return topk_;
    }

    void operator () (unsigned key) {
        if (accessor_.mark(key)) {
            ++cnt_;
            unsigned d = dim_ & ~unsigned(7);
            const float *aa = query_, *end_a = aa + d;
            const float *bb = accessor_(key), *end_b = bb + d;
#ifdef __GNUC__
            __builtin_prefetch(aa, 0, 3);
            __builtin_prefetch(bb, 0, 0);
#endif
            double th = topk_.threshold();
            float r = 0.0;
            float r0, r1, r2, r3, r4, r5, r6, r7;

            const float *a = end_a, *b = end_b;

            r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = 0.0;

            switch (dim_ & 7) {
                case 7: r6 = sqr(a[6] - b[6]);
                case 6: r5 = sqr(a[5] - b[5]);
                case 5: r4 = sqr(a[4] - b[4]);
                case 4: r3 = sqr(a[3] - b[3]);
                case 3: r2 = sqr(a[2] - b[2]);
                case 2: r1 = sqr(a[1] - b[1]);
                case 1: r0 = sqr(a[0] - b[0]);
            }

            a = aa; b = bb;

            for (; a < end_a; a += 8, b += 8) {
#ifdef __GNUC__
                __builtin_prefetch(a + 32, 0, 3);
                __builtin_prefetch(b + 32, 0, 0);
#endif
                r += r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7;
                if (r > th) return;
                r0 = sqr(a[0] - b[0]);
                r1 = sqr(a[1] - b[1]);
                r2 = sqr(a[2] - b[2]);
                r3 = sqr(a[3] - b[3]);
                r4 = sqr(a[4] - b[4]);
                r5 = sqr(a[5] - b[5]);
                r6 = sqr(a[6] - b[6]);
                r7 = sqr(a[7] - b[7]);
            }

            r += r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7;
            topk_ << typename Topk<Key>::Element(key, r);
        }
    }

private:
    ACCESSOR accessor_;
    unsigned dim_;
    unsigned K_;
    float R_;
    Topk<Key> topk_;
    const float* query_;
    unsigned cnt_;
};


}

#endif

