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

#ifndef __LSHKIT_SPECTRAL_HASH__
#define __LSHKIT_SPECTRAL_HASH__

// This define is needed for Micrisoft VS
#define _USE_MATH_DEFINES
#include <math.h>
#include <cmath>
#include <vector>
#include <algorithm>

/**
 * \file spectral-hash.h
 * \brief LSH based on spectral hashing.
 */

namespace lshkit {

/// Spectral hashing.
/**
 * This class only supports loading externally learned hash functions.
 * To learn a hash function from a sample dataset, use the matlab
 * script at matlab/lshkitTrainSH, which is a wrapper of Y. Weiss'
 * spectral hashing code.  The learned function can then be loaded
 * by the serialize method.
 *
 * LSHKIT currently doesn't provide native support of learning
 * spectral hash functions.
 *
 * This class can both produce LSH hash values or sketches.  When
 * used for hashing and more than 32 bits are produced by the
 * hash function, only the first 32 bits are used.
 *
 * 
 * For more information on spectral hashing, see the following reference:
 * Y. Weiss, A. Torralba, R. Fergus. Spectral Hashing.
 * Advances in Neural Information Processing Systems, 2008.
 *
*/
class SpectralHash
{
    std::vector<std::vector<float> > pc;
    std::vector<float> mn;
    std::vector<std::vector<float> > omegas;
public:
    typedef unsigned char CHUNK;
    static const unsigned CHUNK_BIT = sizeof(CHUNK) * 8; // #bits in CHUNK
    struct Parameter
    {
    // NO PARAMETER IS DEFINED.
    };

    typedef const float *Domain;

    SpectralHash ()
    {
    }

    ///This is a placeholder and should not be invoked.
    template <typename RNG>
    void reset(const Parameter &param, RNG &rng)
    {
        BOOST_VERIFY(0);
    }

    ///This is a placeholder and should not be invoked.
    template <typename RNG>
    SpectralHash(const Parameter &param, RNG &rng)
    {
        reset(param, rng);
    }

    unsigned getRange () const
    {
        if (pc.size() <= sizeof(unsigned) * 8) {
            return 1 << pc.size();
        }
        return 0;
    }

    /// Return the number of bits in the sketch.
    unsigned getBits () const {
        return pc.size();
    }

    /// Return the number of chunks in the sketch.
    unsigned getChunks () const {
        return (pc.size() + CHUNK_BIT - 1) / CHUNK_BIT;
    }

    unsigned operator () (Domain obj) const
    {
        std::vector<float> X(pc.size());
        for (unsigned i = 0; i < pc.size(); ++i) {
            X[i] = 0;
            for (unsigned j = 0; j < pc[i].size(); ++j) {
                X[i] += pc[i][j] * obj[j];
            }
            X[i] -= mn[i];
        }

        unsigned ret = 0, mm = std::min(sizeof(unsigned) * 8, omegas.size());
        for (unsigned i = 0; i < mm; ++i) {
            float y = 1;
            for (unsigned j = 0; j < omegas[i].size(); ++j) {
                y *= sin(X[j] * omegas[i][j] + M_PI/2);
            }
            if (y > 0) ret += (1 << i);
        }

        return ret;
    }

    /// Sketch construction
    void apply (Domain in, CHUNK *out) const
    {
        std::vector<float> X(pc.size());
        for (unsigned i = 0; i < pc.size(); ++i) {
            X[i] = 0;
            for (unsigned j = 0; j < pc[i].size(); ++j) {
                X[i] += pc[i][j] * in[j];
            }
            X[i] -= mn[i];
        }

        unsigned l = 0, dim = getChunks();
        for (unsigned i = 0; i < dim; ++i)
        {
            out[i] = 0;
            for (unsigned j = 0; j < CHUNK_BIT; ++j) {
                if (l >= omegas.size()) break;
                float y = 1;
                for (unsigned k = 0; k < omegas[l].size(); ++k) {
                    y *= sin(X[k] * omegas[l][k] + M_PI/2);
                }
                if (y > 0) out[i] += (1 << j);
                ++l;
            }
        }
    }

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & pc;
        ar & mn;
        ar & omegas;
    }

};

}

#endif

