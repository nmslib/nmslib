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
  * \file apost.h
  * \brief A Posteriori Multi-Probe LSH indexing.
  *
  * Reference:
  *
  * Alexis Joly, Olivier Buisson.   A posteriori multi-probe locality sensitive
  * hashing. In Proceedings of the 16th ACM International Conference on
  * Multimedia. Vancouver, Canada. October 2008.
  *
  * 
  */

#ifndef __LSHKIT_APOST__
#define __LSHKIT_APOST__

#include <lshkit/common.h>
#include <lshkit/lsh.h>
#include <lshkit/composite.h>
#include <lshkit/metric.h>
#include <lshkit/lsh-index.h>
#include <lshkit/topk.h>

namespace lshkit
{


struct APostLsh
{
    unsigned dim;
    unsigned M;
    float W;
    unsigned H;
    std::vector<std::vector<float> > a;
    std::vector<float> b;
    std::vector<unsigned> c;

    std::vector<float> umin;
    std::vector<float> umax;

    typedef const float *Domain;

    /**
     * Parameter to MPLSH. 
     */
    struct Parameter {

        unsigned dim;
        unsigned repeat;
        unsigned range;
        float W;
    };

    APostLsh () {}

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        dim = param.dim;
        M = param.repeat;
        W = param.W;
        H = param.range;

        boost::variate_generator<RNG &, Gaussian> gaussian(rng, Gaussian());
        boost::variate_generator<RNG &, Uniform> uniform(rng, Uniform(0,W));

        a.resize(M);
        b.resize(M);
        c.resize(M);
        umin.resize(M);
        umax.resize(M);

        for (unsigned i = 0; i < M; i++) {
            a[i].resize(dim);
            umin[i] = std::numeric_limits<float>::max();
            umax[i] = std::numeric_limits<float>::min();
            for (unsigned j = 0; j < dim; j++) {
                a[i][j] = gaussian();
            }
            b[i] = uniform();
            c[i] = rng();
        }
    }

    template <typename RNG>
    APostLsh(const Parameter &param, RNG &rng)
    {
        reset(param, rng);
    }

    unsigned getRange () const
    {
        return H;
    }

    float apply1 (Domain obj, unsigned m) const {
        float r = b[m];
        for (unsigned j = 0; j < dim; ++j)
        {
            r += a[m][j] * obj[j];
        }
        r /= W;
        return r;
    }

    void apply1 (Domain obj, std::vector<float> *h) const {
        for (unsigned i = 0; i < M; ++i) {
            h->at(i) = apply1(obj, i);
        }
    }

    unsigned operator () (Domain obj)
    {
        unsigned r2 = 0;
        for (unsigned i = 0; i < M; ++i)
        {
            float r1 = apply1(obj, i);
            if (r1 < umin[i]) umin[i] = r1;
            else if (r1 > umax[i]) umax[i] = r1;
            r2 += c[i] * unsigned(int(std::floor(r1)));
        }
        return r2 % H;
    }

    unsigned apply_const (Domain obj) const
    {
        unsigned r2 = 0;
        for (unsigned i = 0; i < M; ++i)
        {
            float r1 = apply1(obj, i);
            r2 += c[i] * unsigned(int(std::floor(r1)));
        }
        return r2 % H;
    }


    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & dim;
        ar & M;
        ar & W;
        ar & H;
        ar & a;
        ar & b;
        ar & c;
        ar & umin;
        ar & umax;
    }
};

struct APostExample {
    const float *query;
    std::vector<const float *> results;
};

struct PrH {    // Pr[h] of certain compnent
    int h;
    float pr;
    template<class Archive>

    void serialize(Archive & ar, const unsigned int version)
    {
        ar & h;
        ar & pr;
    }

    friend bool operator < (const PrH &p1, const PrH &p2) {
        // we need descending order
        return p1.pr > p2.pr;
    }
};

class APostModel
{
    unsigned Nz;
    float ex;
    // lookup[M][h_query][h]
    std::vector<std::vector<std::vector<PrH> > > lookup;
    std::vector<std::vector<float> > means;
    std::vector<std::vector<float> > stds;
    std::vector<float> umin;
    std::vector<float> umax;
public:
    APostModel () {
    };

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & Nz;
        ar & ex;
        ar & lookup;
        ar & means;
        ar & stds;
        ar & umin;
        ar & umax;
    }

    void train (const APostLsh &lsh, const std::vector<APostExample> &examples,
            unsigned N, float k_sigma = 1.0/5, float expand = 0.1);

    void genProbeSequence (const APostLsh &lsh, const float *query,
            float recall, unsigned T, std::vector<unsigned> *probe) const;
};


/// Multi-Probe LSH index.
template <typename KEY>
class APostLshIndex: public LshIndex<APostLsh, KEY>
{
public:
    typedef LshIndex<APostLsh, KEY> Super;
    /**
     * Super::Parameter is the same as MultiProbeLsh::Parameter
     */
    typedef typename Super::Parameter Parameter;

    typedef typename Super::Domain Domain;
    typedef KEY Key;

private:

    std::vector<APostModel> model;

    /// Query for K-NNs.
    /**
      * @param obj the query object.
      * @param scanner 
      */
    template <typename SCANNER>
    void query_helper (Domain obj, float recall, unsigned T, SCANNER &scanner) const
    {
        std::vector<unsigned> seq;
        BOOST_VERIFY(recall <= 1.0);
        recall = 1.0 - exp(1.0/Super::lshs_.size() * log(1.0 - recall));
        for (unsigned i = 0; i < Super::lshs_.size(); ++i) {
            model[i].genProbeSequence(Super::lshs_[i], obj, recall, T, &seq);
            BOOST_FOREACH(unsigned j, seq) {
                typename Super::Bin const &bin = Super::tables_[i][j];
                BOOST_FOREACH(Key key, bin) {
                    scanner(key);
                }
            }
        }
    }

public:

    /// Constructor.
    APostLshIndex() {
    } 

    /// Initialize MPLSH.
    /**
      * @param param parameters.
      * @param engine random number generator (if you are not sure about what to
      * use, then pass DefaultRng.
      * @param accessor object accessor (same as in LshIndex).
      * @param L number of hash tables maintained.
      */
    template <typename Engine>
    void init (const Parameter &param, Engine &engine, unsigned L) {
        Super::init(param, engine, L);
        model.resize(L);
        // we are going to normalize the distance by window size, so here we pass W = 1.0.
        // We tune adaptive probing for KNN distance range [0.0001W, 20W].
    }

    void train (const std::vector<APostExample> &examples, unsigned Nz, float k_sigma, float expand) {
        for (unsigned i = 0; i < model.size(); ++i) {
            model[i].train(Super::lshs_[i], examples, Nz, k_sigma, expand);
        }
    }

    /// Load the index from stream.
    void load (std::istream &ar) 
    {
        Super::load(ar);
        ar & model;
        BOOST_VERIFY(ar);
    }

    /// Save to the index to stream.
    void save (std::ostream &ar)
    {
        Super::save(ar);
        ar & model;
        BOOST_VERIFY(ar);
    }

    /// Query for K-NNs.
    /**
      * @param obj the query object.
      * @param scanner 
      */
    template <typename SCANNER>
    void query (Domain obj, unsigned T, SCANNER &scanner) const
    {
        query_helper(obj, 1.0, T, scanner);
    }

    /// Query for K-NNs, try to achieve the given recall by adaptive probing.
    /**
      * There's a special requirement for the scanner type used in adaptive query.
      * It should support the following method to return the current K-NNs:
      *
      * const Topk<KEY> &topk () const;
      */
    template <typename SCANNER>
    void query_recall (Domain obj, float recall, SCANNER &scanner) const
    {
        query_helper(obj, recall, std::numeric_limits<unsigned>::max(), scanner);
    }
};

}


#endif

