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

#ifndef __LSHKIT_EVAL__
#define __LSHKIT_EVAL__

/**
 * \file eval.h
 * \brief A set of classes for evaluation.
 *
 */

#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>
#include <fstream>
#include <boost/random.hpp>
#include <lshkit/topk.h>


namespace lshkit {

/// Generate non-duplicate random numbers in a given range as query IDs.
/**
 * @param qry the vector to hold the generated IDs.
 * @param max The maximum ID(excluded).
 * @param rng Random number generator, use a variable of type DefaultRng.
 *
 */
template <typename RNG>
void SampleQueries (std::vector<unsigned> *qry, unsigned max, RNG
        &rng)
{
    boost::variate_generator<RNG &, UniformUnsigned> gen(rng, UniformUnsigned(0,
                max-1));
    for (unsigned i = 0; i < qry->size(); ++i)
    {
        for (;;)
        {
            qry->at(i) = gen();
            unsigned j;
            for (j = 0; j < i; j++) if (qry->at(i) == qry->at(j)) break;
            if (j >= i) break;
        }
    }
}


/// Access a benchmark file.
/**
 * We assume the feature vectors in the benchmark database are
 * numbered from 0 to N.  We sample Q queries as test examples
 * and run K-NN search against the database with linear scan.  The results are
 * saved in a benchmark file for evaluation purpose.  
 * A benchmark file is made up of Q lines, each line represents a
 * test query and is of the following format:
 *
 * <query ID> <K> <1st NN's ID> <distance> <2nd NN's ID> <distance> ... <Kth NN's ID> <distance>
 *
 * For all queries in the benchmark file, the K value should be the same.
 *
 * Because the query points are also sampled from the database, they should be excluded from
 * scanning when running this particular query.
 */
template <typename KEY = unsigned>
class Benchmark
{
    unsigned Q_;

    std::vector<unsigned> queries_;
    std::vector<Topk<KEY> > topks_;
public:
    Benchmark(): Q_(0) {}

    void resize(unsigned Q, unsigned K = 0)
    {
        Q_ = Q;
        queries_.resize(Q);
        topks_.resize(Q);
        if (K > 0) {
            BOOST_FOREACH(Topk<KEY> &knn, topks_) {
                if (knn.size() < K) throw std::runtime_error("BENCHMARK NOT LARGE ENOUGH");
                knn.resize(K);
            }
        }
    }

    // Random initialization
    void init(unsigned Q, unsigned maxID, unsigned seed = 0) {
        Q_ = Q;
        queries_.resize(Q);
        topks_.resize(Q);

        DefaultRng rng;
        if (seed != 0) rng.seed(seed);
        SampleQueries(&queries_, maxID, rng);
    }

    ~Benchmark() {}

    void load (std::istream &is)
    {
        std::string line;
        queries_.clear();
        topks_.clear();
        for (;;) {
            unsigned q, k;
            is >> q;
            if (!is) break;
            queries_.push_back(q);
            is >> k;
            topks_.push_back(Topk<KEY>());
            topks_.back().resize(k);
            for (unsigned j = 0; j < k; j++) {
                is >> topks_.back()[j].key;
                is >> topks_.back()[j].dist;
            }
        }
        Q_ = queries_.size();
    }

    void save (std::ostream &os) const
    {
        for (unsigned i = 0; i < Q_; i++) {
            os << queries_[i] << '\t' << topks_[i].size() << '\t';
            for (unsigned j = 0; j < topks_[i].size(); j++) {
                os << '\t' <<  topks_[i][j].key;
                os << '\t' <<  topks_[i][j].dist;
            }
            os << std::endl;
        }
    }

    void load (const std::string &path)
    {
        std::ifstream is(path.c_str());
        load(is);
        is.close();
    }

    void save (const std::string &path) const
    {
        std::ofstream os(path.c_str());
        save(os);
        os.close();
    }

    unsigned getQ () const { return Q_; }

    /// Get the ID of the nth query.
    unsigned getQuery (unsigned n) const {  return queries_[n]; }

    /// Get the nearest neighbors of the nth query.
    const Topk<KEY> &getAnswer (unsigned n) const { return topks_[n]; }

    /// Get the KNNs for modification.
    Topk<KEY> &getAnswer (unsigned n) { return topks_[n]; }
};


/// Basic statistics.
/**
 *  The interface is self-evident.
 *  Usage:
 *
 *  \code
 *  Stat stat;
 *
 *  stat << 1.0 << 2.0 << 3.0;
 *
 *  Stat stat2;
 *  stat2 << 3.0 << 5.0 << 6.0;
 *
 *  stat.merge(stat2);
 *
 *  stat.getCount();
 *  stat.getSum();
 *  stat.getMax();
 *  stat.getMin();
 *  stat.getStd();
 *  \endcode
 *
 */
class Stat
{
    int count;
    float sum;
    float sum2;
    float min;
    float max;
public:
    Stat () : count(0), sum(0), sum2(0), min(std::numeric_limits<float>::max()), max(-std::numeric_limits<float>::max()) {
    }

    ~Stat () {
    }

    void reset () {
        count = 0;
        sum = sum2 = 0;
        min = std::numeric_limits<float>::max();
        max = -std::numeric_limits<float>::max();
    }

    void append (float r)
    {
        count++;
        sum += r;
        sum2 += r*r;
        if (r > max) max = r;
        if (r < min) min = r;
    }

    Stat & operator<< (float r) { append(r); return *this; }

    int getCount() const { return count; }
    float getSum() const { return sum; }
    float getAvg() const { return sum/count; }
    float getMax() const { return max; }
    float getMin() const { return min; }
    float getStd() const
    {
        if (count > 1) return std::sqrt((sum2 - (sum/count) * sum)/(count - 1)); 
        else return 0; 
    }

    void merge (const Stat &stat)
    {
        count += stat.count;
        sum += stat.sum;
        sum2 += stat.sum2;
        if (stat.min < min) min = stat.min;
        if (stat.max > max) max = stat.max;
    }
};

/*
class Timer
{
    struct  timeval start; 
public:
    Timer () {}
    /// Start timing.
    void tick ()
    {
        gettimeofday(&start, 0); 
    }
    /// Stop timing, return the time passed (in second).
    float tuck (const char *msg) const
    {
        struct timeval end;
        float   diff; 
        gettimeofday(&end, 0); 

        diff = (end.tv_sec - start.tv_sec) 
                + (end.tv_usec - start.tv_usec) * 0.000001; 
        if (msg != 0) {
            std::cout << msg << ':' <<  diff << std::endl;
        }
        return diff;
    }
};
*/

}

#endif

