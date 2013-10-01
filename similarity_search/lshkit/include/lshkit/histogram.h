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

#ifndef __LSHKIT_HISTOGRAM__
#define __LSHKIT_HISTOGRAM__

/** \file histogram.h
  * \brief LSH-based random histogram construction.
  *
  * Random histograms are used to match sets of feature vectors.
  * A random histogram is a simplified version of LSH hash table, except that
  * for each bin, only a count, instead of a list of features, is maintained.
  * If two sets are similar, then the counts in the corresponding bins would be
  * similar.  Thus, set similarity is mapped to vector similarity.
  *
  *
  * Following is an example to embed a set of sift features into a random histogram.
  * We use the ThresholdingLsh to approximate L1 distance between SIFT features.
  *
  * \code
  *      #include <lshkit.h>
  *      using namespace lshkit;
  *
  *      // One ThresholdingLsh only produces 1 bit.  We use the Repeat composition
  *      // to produce a hash value of 8 bits, which can be used to produce one
  *      // histogram of 2^8 = 256 dimensions.
  *      typedef Repeat<ThresholdingLsh> MyLSH;
  *
  *      typedef Histogram<MyLSH> MyHistogram;
  *
  *      TresholdingLsh::Parameter param;
  *
  *      param.repeat = 8; // repeat 8 times, produce 8 bits.
  *      param.dim = 128;
  *      param.min = 0.0;
  *      param.max = 255.0; // Parameters are set for SIFT features.
  *
  *      DefaultRng rng;
  *      unsigned N = 10, M = 10;
  *      MyHistogram hist;
  *      hist.reset(M, N, param, rng);
  *      // The histogram is constructed by concatenating N histograms of 2^8 dimensions,
  *      // So there will be 2560 dimensions in all.
  *
  *      float *output = new float[hist.getDim()]; // should be 2560.
  *      hist.zero(output); // init the output histogram
  *
  *      // IMAGE is a container of SIFT features.
  *      BOOST_FOREACH(const float *sift, IMAGE) {
  *         hist.add(output, sift);
  *      }
  *
  *      // now the output should contain the desired histogram.
  *      // The histogram can be fed into SVM for classification.
  *
  *      \endcode
  *
  *
  *
  * See the following reference for details.
  *
  * Wei Dong, Zhe Wang, Moses Charikar, Kai Li. Efficiently Matching Sets of
  * Features with Random Histograms. In Proceedings of the 16th ACM
  * International Conference on Multimedia. Vancouver, Canada. October 2008.
  */

namespace lshkit {

/// Random histogram construction.
template <typename LSH>
class Histogram
{
    BOOST_CONCEPT_ASSERT((LshConcept<LSH>));

    std::vector<LSH> lsh_;

    unsigned M_, N_;
    unsigned dim_;
    unsigned unit_;
public:
    typedef typename LSH::Parameter Parameter;
    typedef typename LSH::Domain Domain;

    /** Default constructor.
     */
    Histogram () {
    }

    /** Reset the sketcher.
     *  @param M Number repeated to take average.
     *  @param N Number of concatenated histograms.
     *  @param param Parameter to LSH.
     *  @param rng Random number generator (usually a variable of type lshkit::DefaultRng.
     */
    template <typename RNG>
    void reset(unsigned M, unsigned N, Parameter param, RNG &rng)
    {
        lsh_.resize(M * N);
        M_ = M;
        N_ = N;
        for (unsigned i = 0; i < lsh_.size(); ++i)
        {
            lsh_[i].reset(param, rng);
        }
        unit_ = lsh_[0].getRange();
        dim_ = N_ * unit_;
    }

    /** Constructor.
     *  @param M Number repeated to take average.
     *  @param N Number of concatenated histograms.
     *  @param param Parameter to LSH.
     *  @param rng Random number generator (usually a variable of type lshkit::DefaultRng.
     */
    template <typename RNG>
    Histogram(unsigned M, unsigned N, Parameter parameter, RNG &rng)
        : lsh_(M * N), M_(M), N_(N)
    {
        for (unsigned i = 0; i < lsh_.size(); ++i)
        {
            lsh_[i].reset(parameter, rng);
        }
        unit_ = lsh_[0].getRange();
        dim_ = N_ * unit_;
    }

    /// Get the total dimension of the histogram.
    unsigned dim () const
    {
        return dim_;
    }

    /// Initialize an output histogram.
    /**
     * @param out The array to hold the output histogram.
     */
    void zero (float *out) const
    {
        std::fill(out, out + dim_, 0);
    }
    
    /// Add the information of a vector to the output histogram.
    /**
     * @param out The array to hold the output histogram.
     * @param in The input vector.
     * @param weight Weight of the input vector.
     */
    void add (float *out, Domain in, float weight = 1.0) const
    {
        unsigned k = 0;
        unsigned base = 0;
        for (unsigned i = 0; i < N_; ++i)
        {
            for (unsigned j = 0; j < M_; ++j)
            {
                unsigned index = base + (lsh_[k])(in);
                out[index] +=  weight;
                ++k;
            }
            base += unit_;
        }
    }
};

}

#endif
