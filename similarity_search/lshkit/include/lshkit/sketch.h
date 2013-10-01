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


#ifndef __LSHKIT_SKETCH__
#define __LSHKIT_SKETCH__

/**
 * \file sketch.h
 * \brief  Implementation of LSH based sketches.
 *
 * Sketches are compact representations (as bit-vectors) of large objects.  They
 * are simply constructed by concatenating a bunch of 1-bit LSH hash values.
 * The distance between the original objects can be approximated by the hamming
 * distance between sketches.  The generated bit-vectors are stored in
 * arrays of type CHUNK (unsigned char by default, so each chunk has 8 bits).
 *
 *  Following is an example of distance
 *  estimation using sketches:
 *
 *  \code
 *      
 *      #include <lshkit.h>
 *      using namespace lshkit;
 *
 *      typedef Sketch<ThresholdingLsh> MySketch; // approximate L1 distance
 *      TresholdingLsh::Parameter param;
 *
 *      param.dim = 128;
 *      param.min = 0.0;
 *      param.max = 255.0; // Parameters are set for, maybe, SIFT features.
 *
 *      DefaultRng rng;
 *
 *      const static SKETCH_BITS = 256;
 *      const static SKETCH_BYTES = 256 / 8;
 *
 *      MySketch sketch(SKETCH_BYTES, param, rng); // from 128 floats to 256 bits.
 *      // Note that in practice you are probably not going to simply construct a sketcher like this.
 *      // The sketcher used to sketch the query point should be the exact one used to
 *      // sketch the data points.  So by the time a database has been constructed,
 *      // the sketcher is most probably already constructed and saved in an archive.
 *      // You should use the serialize method to load the sketcher from the archive (first default construct
 *      // a sketcher, than apply sketch.serialize(some_input_stream, 0).)
 *
 *
 *      
 *      // Allocate space for sketches.  Using "new" is probably not a good idea.
 *      char *query_sketch = new char[SKETCH_BYTES];
 *      float *asym_info = new float[SKETCH_BITS]; // to hold asymmetric information.
 *
 *      float *query = ...;
 *
 *      sketch.apply(query, query_sketch, asym_info);
 *
 *      WeightedHammingHelper asym_helper(SKETCH_BYTES);
 *      asym_helper.update(query_sketch, asym_info);
 *
 *      metric::hamming<char> hamming(SKETCH_BYTES);
 *
 *      // Scan the database.
 *      // DATABASE is a container of previously constructed sketches.
 *      BOOST_FOREACH (const char* data_sketch, DATABASE) {
 *
 *          // evaluate the symmetric sketch distance
 *          float sym_dist = hamming(query_sketch, data_sketch);
 *
 *          // evaluate the asymmetric sketch distance
 *          float asym_dist = asym_helper.distTo(data_sketch);
 *
 *          // asym_dist should be more reliable than sym_dist for ranking.
 *      }
 *
 *      \endcode
 *
 * See src/sketch-run.cpp for a full example.
 *
 *
 * For more information on sketches and asymmetric distance estimators, see
 *
 *     Wei Dong, Moses Charikar, Kai Li. Asymmetric Distance Estimation with
 *     Sketches for Similarity Search in High-Dimensional Spaces. In
 *     Proceedings of the 31st Annual International ACM SIGIR Conference on Research
 *     & Development on Information Retrieval. Singapore. July 2008.
 *
*/

#include <lshkit/matrix.h>


namespace lshkit {

/// LSH-based sketcher.
template <typename LSH, typename CHUNK = unsigned char>
class Sketch
{
    BOOST_CONCEPT_ASSERT((DeltaLshConcept<LSH>));

    unsigned dim_;
    std::vector<LSH> lsh_;

public:
    /// LSH parameter
    typedef typename LSH::Parameter Parameter;
    /// Domain of LSH & Sketcheter
    typedef typename LSH::Domain Domain; 
    /// Number of bits in each CHUNK.
    static const unsigned CHUNK_BIT = sizeof(CHUNK) * 8; // #bits in CHUNK

    /** Constructor, without initialization.
     */
    Sketch() {
    }

    /** Reset the sketcher.
     *  @param chunks Number of chunks in the sketch.
     *  @param param Parameter to LSH.
     *  @param engine Random number generator (usually a variable of type lshkit::DefaultRng.
     */
    template <typename Engine>
    void reset (unsigned chunks, Parameter param, Engine &engine) {
        dim_ = chunks;
        lsh_.resize(dim_ * CHUNK_BIT);
        for (unsigned i = 0; i < lsh_.size(); ++i) lsh_[i].reset(param, engine);
    }

    /** Constructor with initialation (same as Sketch() immediately followed by reset()).
     *  @param chunks Number of chunks in the sketch.
     *  @param param Parameter to LSH.
     *  @param engine Random number generator (usually a variable of type lshkit::DefaultRng.
     */
    template <typename Engine>
    Sketch(unsigned chunks, Parameter param, Engine &engine) : 
        dim_(chunks), lsh_(dim_ * CHUNK_BIT)
    {
        for (unsigned i = 0; i < lsh_.size(); ++i) lsh_[i].reset(param, engine);
    }

    /// Sketch construction
    void apply (Domain in, CHUNK *out) const
    {
        unsigned l = 0;
        for (unsigned i = 0; i < dim_; ++i) {
            out[i] = 0;
            for (unsigned j = 0; j < CHUNK_BIT; ++j) {
                unsigned k = lsh_[l++](in);
                out[i] = out[i] | (k << j);
            }
        }
    }
    
    /// Asymmetric sketch construction
    /**  @param asym The values used for asymmetric distance estimation.
     *
     *  For sketch of N bits, the array asym should have enough space to
     *  hold N floats.
     */
    void apply (Domain in, CHUNK *out, float *asym) const
    {
        unsigned l = 0;
        for (unsigned i = 0; i < dim_; ++i) {
            out[i] = 0;
            for (unsigned j = 0; j < CHUNK_BIT; ++j) {
                unsigned k = lsh_[l](in, &asym[l]);
                out[i] = out[i] | (k << j);
                l++;
            }
        }
    }

    /// Serialize the sketcher.
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & dim_;
        ar & lsh_;
    }

    /// Load from stream.
    void load (std::istream &is) {
        serialize(is, 0);
    }

    /// Save to stream.
    void save (std::ostream &os) {
        serialize(os, 0);
    }

    /// Return the number of bits in the sketch.
    unsigned getBits () const {
        return dim_ * CHUNK_BIT;
    }

    /// Return the number of chunks in the sketch.
    unsigned getChunks () const {
        return dim_;
    }
};

/// Weighted hamming distance calculator.
/**
 *  A helper class to accelerate weighted hamming distance calculation by using
 *  a lookup table.  Weighted hamming distance is used in
 *  asymmetric distance evaluation between a sketch and a query point.
 *  A instance of this class holds the information of a single query point, and
 *  can be used to estimate the distance from the query point to different sketches
 *  in the database.  The construction of such a helper instance is costly, so
 *  once constructed, you need to use it as many times as possible to lower the
 *  amortized cost.
 *  
 */
template <typename CHUNK = unsigned char>
class WeightedHammingHelper
{
public:
    static const unsigned CHUNK_BIT = sizeof(CHUNK) * 8;
    /// Constructor.
    /**
      * @param chunks number of chunks in sketch.
      */
    WeightedHammingHelper(unsigned chunks)
        : nchunk_(chunks), lookup_(1 << CHUNK_BIT, chunks)
    {
    }

    /// Update the information of the query point.
    /**
      * @param in sketch of the query point.
      * @param asym weights, the output of Sketch::apply.
      */
    void update (const CHUNK *in, const float *asym)
    {
        unsigned l = 0;
        for (unsigned i = 0; i < nchunk_; ++i) {
            CHUNK q = in[i];
            CHUNK p = 0;
            for (;;) {
                CHUNK c = q ^ p;
                float v = 0;
                for (unsigned j = 0; j < CHUNK_BIT; ++j) {
                    if (c & (1 << j)) {
                        v += asym[l + j];
                    }
                }
                lookup_[i][p] = v;
                ++p;
                if (p == 0) break;
            }
            l += CHUNK_BIT;
        }
    }

    /// Calculate the distance between the saved query point to an incoming point.
    float distTo (const CHUNK *in)
    {
        float dist = 0;
        for (unsigned i = 0; i < nchunk_; ++i) {
            dist += lookup_[i][in[i]];
        }
        return dist;
    }

private:
    unsigned nchunk_;
    Matrix<float> lookup_;
};


}

#endif

