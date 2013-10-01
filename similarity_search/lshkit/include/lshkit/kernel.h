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
 * \file kernel.h
 * \brief Common kernel definitions.
 */

#ifndef __LSHKIT_KERNEL__
#define __LSHKIT_KERNEL__

// This file implements some common kernel functions.

#include <functional>

namespace lshkit { namespace kernel {

/// inner product
template <typename T /* real type */>
class dot : public std::binary_function<const T*, const T*, float>
{
    unsigned dim_;
public:
    dot (unsigned dim) : dim_(dim) {}
    float operator () (const T *first1, const T *first2) const
    {
        float r = 0.0;
        for (unsigned i = 0; i < dim_; ++i)
        {
            r += first1[i] * first2[i];
        }
        return r;
    }
};


}}

#endif


