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
 * \file metric.h
 * \brief Common distance measures.
 */

#ifndef __FERRET_METRIC__
#define __FERRET_METRIC__

// This file implements some common distance functions.

#include <functional>
#include <iostream>

namespace lshkit { namespace metric {

/// L1 distance
template <typename T /* real type */>
class l1 : public std::binary_function<T, T, float>
{
    unsigned dim_;
public:
    l1 (unsigned dim) : dim_(dim) {}
    float operator () (const T *first1, const T *first2) const
    {
        #if DEBUG_LEVEL > 50
          std::cout << "dim " << dim_ << std::endl;
          for (unsigned i = 0; i < dim_; ++i) std::cout << first1[i] << " "; std::cout << std::endl;
          for (unsigned i = 0; i < dim_; ++i) std::cout << first2[i] << " "; std::cout << std::endl;
        #endif

        float r = 0.0;
        for (unsigned i = 0; i < dim_; ++i)
        {
            r += std::fabs(first1[i] - first2[i]);
        }
        return r;
    }
};

/// L2 distance
template <typename T /* real type */>
class l2 : public std::binary_function<const T*, const T*, float>
{
    unsigned dim_;
public:
    l2 (unsigned dim) : dim_(dim) {}
    float operator () (const T *first1, const T *first2) const
    {
        float r = 0.0;
        for (unsigned i = 0; i < dim_; ++i)
        {
            r += sqr(first1[i] - first2[i]);
        }
        return std::sqrt(r);
    }
};

/// Squred L2 distance
/**
  * The square root operation is costly.  For K-NN search, l2sqr
  * gives the same ranking as l2.
  */
template <typename T /* real type */>
class l2sqr : public std::binary_function<const T*, const T*, float>
{
    unsigned dim_;
public:
    l2sqr (unsigned dim) : dim_(dim) {}
    unsigned dim() const {
        return dim_;
    }
    float operator () (const T *first1, const T *first2) const
    {
        float r = 0.0;
        for (unsigned i = 0; i < dim_; ++i)
        {
            r += sqr(first1[i] - first2[i]);
        }
        return r;
    }
};

/// Max-norm distance.
template <typename T /* real type */>
class max : public std::binary_function<const T*, const T*, float>
{
    unsigned dim_;
public:
    max (unsigned dim) : dim_(dim) {}
    float operator () (const T *first1, const T *first2) const
    {
        double r = std::numeric_limits<T>::min();
        for (unsigned i = 0; i < dim_; ++i)
        {
            r += max(r, std::fabs(first1[i] - first2[i]));
        }
        return (float)std::sqrt(r);
    }
};

/// (Basic) hamming distance
/** Take the hamming distance between two values of type T as bit-vectors.
 *  Normally you should use hamming instead of basic_hamming.
  */
struct basic_hamming
{
    static unsigned char_bit_cnt[];

    template <typename B>
    unsigned __hamming (B a, B b)
    {
        B c = a ^ b;
        unsigned char *p = reinterpret_cast<unsigned char *>(&c);
        unsigned r = 0;
        for (unsigned i = 0; i < sizeof(B); i++)
        {
            r += char_bit_cnt[*p++];
        }
        return r;
    }

    unsigned __hamming (unsigned char c1, unsigned char c2) const
    {
        return char_bit_cnt[c1 ^ c2];
    }

};

/// Hamming distance.
/**
  * Take the hamming distance between two bit-vectors, represented as arrays
  * of some basic type.  The parameter dim is the size of the bit-vectors in
  * terms of the number of basic types.  For example, if we use uchar_t as basic
  * type, then the dim of a 256-bit vector is 256/8 = 32.
 */
template <typename T>
struct hamming : public std::binary_function<const T*, const T*, float>, public basic_hamming
{
    unsigned dim_;
public:

    hamming(unsigned dim): dim_(dim) {}
    float operator () (const T *first1, const T *first2) const
    {
        unsigned r = 0;
        for (unsigned i = 0; i < dim_; ++i)
        {
            r += __hamming(first1[i], first2[i]);
        }
        return r;
    }
};


}}

#endif


