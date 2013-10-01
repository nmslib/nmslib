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
  * \file mplsh.h
  * \brief Multi-Probe LSH indexing.
  *
  * Multi-Probe LSH (MPLSH) uses the same data structure as LshIndex, except that it
  * probes more than one buckets in each hash table to generate more accurate
  * results.  Equivalently, less hash tables are needed to achieve the same
  * accuracy.  The limitation is that the current implementation only works for
  * L2 distance.
  * 
  * Follow the following 4 steps to use the MPLSH API.
  * 
  * \section mplsh-1 1. Implement a scanner class which scan the candidate keys.
  * 
  * The MPLSH data structure doesn't manage the feature vectors, but only keeps
  * the keys to retrieve them.  You need to provide a scanner class to MPLSH, and
  * for each query, MPLSH will pass the candidate keys to the scanner.
  * The scanner usually keeps an K-NN data structure internally and updates
  * it when receives candidate keys.
  *
  * MPLSH uses the scanner as a unary function taking a key as argument.
  *
  * The default scanner implementation is TopkScanner.
  * 
  * \code
  * class Scanner
  * {
  *      ...
  *      void operator () (unsigned key);
  * };
  * \endcode
  *
  * \section mplsh-2 2. Construct the MPLSH data structure.
  *
  * Assume we use key type KEY.
  *
  * \code
  *
  * typedef MultiProbeLshIndex<KEY> Index;
  *
  * Index index;
  * \endcode
  *
  * The index can not be used yet.
  *
  * \section mplsh-3 3. Populate the index / Load the index from a previously saved file.
  *
  * When the index is initially built, use the following to populate the index:
  * \code
  * Index::Parameter param;
  *
  * //Setup the parameters.  Note that L is not provided here.
  * param.W = W;
  * param.H = H; // See H in the program parameters.  You can just use the default value.
  * param.M = M;
  * param.dim = DIMENSION_OF_THE_DATA
  * DefaultRng rng; // random number generator.
  * 
  * index.init(param, rng, L);
  * 
  * for (each possible key, value pair) {
  *     index.insert(key, value);
  * }
  * 
  * // You can now save the index for future use.
  * ofstream os(index_file.c_str(), std::ios::binary);
  * index.save(os);
  * \endcode
  *  
  * Or you can load from a previously saved file
  *  
  * \code
  * ifstream is(index_file.c_str(), std::ios::binary);
  * index.load(is);
  * \endcode
  * 
  * \section mplsh-4 4. Query the MPLSH. 
  * 
  * \code
  *   
  * float *query;
  * ...
  * index.query(query, T, scanner);   cnt is the number of points actually scanned.
  * 
  * \endcode
  *
  * See the source file lshkit/tools/mplsh-run.cpp for a full example of using MPLSH.
  *
  * For adaptive probing, I hard coded the sensitive range of KNN distance to
  * [0.0001W, 100W] and logarithmically quantized the range to 200 levels.
  * If you find that your KNN distances fall outside this range, or want more refined
  * quantization, you'll have to modify the code in lshkit::MultiProbeLshIndex::init().
  *
  * \section ref Reference
  *
  * Wei Dong, Zhe Wang, William Josephson, Moses Charikar, Kai Li. Modeling LSH
  * for Performance Tuning.. To appear in In Proceedings of ACM 17th Conference
  * on Information and Knowledge Management (CIKM). Napa Valley, CA, USA.
  * October 2008.
  *
  * Qin Lv, William Josephson, Zhe Wang, Moses Charikar, Kai Li. Multi-Probe LSH:
  * Efficient Indexing for High-Dimensional Similarity Search. Proceedings of the
  * 33rd International Conference on Very Large Data Bases (VLDB). Vienna,
  * Austria. September 2007.
  *
  */

#ifndef __LSHKIT_PROBE__
#define __LSHKIT_PROBE__

#include <lshkit/common.h>
#include <lshkit/lsh.h>
#include <lshkit/composite.h>
#include <lshkit/metric.h>
#include <lshkit/lsh-index.h>
#include <lshkit/mplsh-model.h>
#include <lshkit/topk.h>

namespace lshkit
{

static inline unsigned long long leftshift (unsigned N) {
    return (unsigned long long)1 << N;
}

/// Probe vector.
struct Probe
{
    unsigned long long mask;
    unsigned long long shift;
    float score;
    unsigned reserve;
    bool operator < (const Probe &p) const { return score < p.score; }
    Probe operator + (const Probe &m) const
    {
        Probe ret;
        ret.mask = mask | m.mask;
        ret.shift = shift | m.shift;
        ret.score = score + m.score;
        return ret;
    }
    bool conflict (const Probe &m)
    {
        return (mask & m.mask) != 0;
    }
    static const unsigned MAX_M = 64;
    static const unsigned MAX_T = 200;
}; 

/// Probe sequence.
typedef std::vector<Probe> ProbeSequence;

/// Generate a template probe sequence.
void GenProbeSequenceTemplate (ProbeSequence &seq, unsigned M, unsigned T);

/// Probe sequence template.
class ProbeSequenceTemplates: public std::vector<ProbeSequence>
{
public:
    ProbeSequenceTemplates(unsigned max_M, unsigned max_T)
        : std::vector<ProbeSequence>(max_M + 1)
    {
        for (unsigned i = 1; i <= max_M; ++i)
        {
            GenProbeSequenceTemplate(at(i), i, max_T);
        }
    }
};

extern ProbeSequenceTemplates __probeSequenceTemplates;

/// Multi-Probe LSH class.
class MultiProbeLsh: public RepeatHash<GaussianLsh> 
{
    unsigned H_;
public:
    typedef RepeatHash<GaussianLsh> Super;
    typedef Super::Domain Domain;

    /**
     * Parameter to MPLSH. 
     *
     * The following parameters are inherited from the ancestors
     * \code
     *   unsigned repeat; // the same as M in the paper
     *   unsigned dim;
     *   float W;
     * \endcode
     */
    struct Parameter : public Super::Parameter {

        unsigned range;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & range;
            ar & repeat;
            ar & dim;
            ar & W;
        }
    };

    MultiProbeLsh () {}

    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        H_ = param.range;
        Super::reset(param, rng);
    }

    template <typename RNG>
    MultiProbeLsh(const Parameter &param, RNG &rng)
    {
        H_ = param.range;
        Super::reset(param, rng);
    }

    unsigned getRange () const
    {
        return H_;
    }

    unsigned operator () (Domain obj) const
    {
        return Super::operator ()(obj) % H_;
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        Super::serialize(ar, version);
        ar & H_;
    }

    void genProbeSequence (Domain obj, std::vector<unsigned> &seq, unsigned T) const;
};


/// Multi-Probe LSH index.
template <typename KEY>
class MultiProbeLshIndex: public LshIndex<MultiProbeLsh, KEY>
{
public:
    typedef LshIndex<MultiProbeLsh, KEY> Super;
    /**
     * Super::Parameter is the same as MultiProbeLsh::Parameter
     */
    typedef typename Super::Parameter Parameter;

private:

    Parameter param_;
    MultiProbeLshRecallTable recall_;

public: 
    typedef typename Super::Domain Domain;
    typedef KEY Key;

    /// Constructor.
    MultiProbeLshIndex() {
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
        param_ = param;
        // we are going to normalize the distance by window size, so here we pass W = 1.0.
        // We tune adaptive probing for KNN distance range [0.0001W, 20W].
        recall_.reset(MultiProbeLshModel(Super::lshs_.size(), 1.0, param_.repeat, Probe::MAX_T), 200, 0.0001, 20.0);
    }

    /// Load the index from stream.
    void load (std::istream &ar) 
    {
        Super::load(ar);
        param_.serialize(ar, 0);
        recall_.load(ar);
        BOOST_VERIFY(ar);
    }

    /// Save to the index to stream.
    void save (std::ostream &ar)
    {
        Super::save(ar);
        param_.serialize(ar, 0);
        recall_.save(ar);
        BOOST_VERIFY(ar);
    }

    /// Query for K-NNs.
    /**
      * @param obj the query object.
      * @param scanner 
      */
    template <typename SCANNER>
    void query (Domain obj, unsigned T, SCANNER &scanner)
    {
        std::vector<unsigned> seq;
        for (unsigned i = 0; i < Super::lshs_.size(); ++i) {
            Super::lshs_[i].genProbeSequence(obj, seq, T);
            for (unsigned j = 0; j < seq.size(); ++j) {
                typename Super::Bin &bin = Super::tables_[i][seq[j]];
                BOOST_FOREACH(Key key, bin) {
                    scanner(key);
                }
            }
        }
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
        unsigned K = scanner.topk().getK();
        if (K == 0) throw std::logic_error("CANNOT ACCEPT R-NN QUERY");
        if (scanner.topk().size() < K) throw std::logic_error("ERROR");
        unsigned L = Super::lshs_.size();
        std::vector<std::vector<unsigned> > seqs(L);
        for (unsigned i = 0; i < L; ++i) {
            Super::lshs_[i].genProbeSequence(obj, seqs[i], Probe::MAX_T);
        }

        for (unsigned j = 0; j < Probe::MAX_T; ++j) {
            if (j >= seqs[0].size()) break;
            for (unsigned i = 0; i < L; ++i) {
                BOOST_FOREACH(Key key, Super::tables_[i][seqs[i][j]]) {
                    scanner(key);
                }
            }
            float r = 0.0;
            for (unsigned i = 0; i < K; ++i) {
                r += recall_.lookup(std::sqrt(scanner.topk()[i].dist) / param_.W, j + 1);
            }
            r /= K;
            if (r >= recall) break;
        }
    }
};

}


#endif

