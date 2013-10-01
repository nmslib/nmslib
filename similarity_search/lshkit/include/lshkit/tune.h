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
#ifndef WDONG_LSHKIT_TUNE
#define WDONG_LSHKIT_TUNE

/**
 * \file tune.h
 * \brief Constrained multi-variate monotone function optimizatioin.
 */

#include <cassert>
#include <vector>
#include <map>

namespace lshkit { namespace tune {

struct Interval {  // [begin, end)
    unsigned begin;
    unsigned end;       // max should always work, minimize the working value
};

typedef std::vector<Interval> Range;
typedef std::vector<unsigned> Input;

template <typename F>
class TuneHelper
{
    const Range &range;
    const F &constraint;
public:
    TuneHelper (const Range &ran, const F &con)
        : range(ran), constraint(con)
    {
    }

    bool search (Input *v, unsigned depth)
    {
        /*
        std::cout << depth << ':';
        for (unsigned i = 0; i < depth; ++i) {
            std::cout << ' ' << v->at(i);
        }
        std::cout << std::endl;
        */

        if (depth >= range.size())
        {
            return constraint(*v);
        }

        assert(range[depth].begin < range[depth].end); // range should not be empty

        Input left(*v);
        Input right(*v);
        Input middle(*v);

        left[depth] = range[depth].begin;
        right[depth] = range[depth].end - 1;

        // make sure left doesn't work
        if (search(&left, depth+1)) {
            // if left works, then that's the answer
            *v = left;
            return true;
        }

        // make sure right works
        if (!search(&right, depth+1)) { // best doesn't work
            return false;
        }

        for (;;) {
            unsigned m = (left[depth] + right[depth]) / 2;
            if (m == left[depth] || m == right[depth]) {
                *v = right;
                return true;
            }
            
            middle[depth] = m;
            if (search(&middle, depth+1)) {
                right = middle;
            }
            else {
                left = middle;
            }
        }
    }
};

template <typename F>
bool Tune (const Range &ran, const F &con, Input *v)
{
    TuneHelper<F> helper(ran, con);
    v->resize(ran.size());
    return helper.search(v, 0);
}

} }

#endif
