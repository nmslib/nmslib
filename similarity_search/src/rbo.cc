/**
 *  The RBO indefinite rank similarity metric.
 *
 *  This code implements the RBO metric, as described in:
 *
 *  @article{wmz10:acmtois,
 *      author = "Webber, William and Moffat, Alistair and Zobel, Justin",
 *      title = "A similarity measure for indefinite rankings",
 *      journal = "ACM Transactions on Information Systems",
 *      year = {2010},
 *  }
 *
 *  In particular, the code implements extrapolated RBO, Equation 32
 *  in the above paper, supporting uneven lists, but not supporting 
 *  ties (for lack of a simple input format).
 */
/*
Copyright (c) 2010 William Webber <wew@csse.unimelb.edu.au>

Leo changed the code by doing the following:
1) Using a stanard C++ hash 
2) Relying only on integer entry IDs.
3) Implementing a wrapper ComputeRBO that operates on vectors of IDs
4) Note that Leo also implemented some tests to ensure that these changes
   did not screw up original William's code

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <cmath>
#include <algorithm>

#include "logging.h"
#include "rbo.h"

/**
 *  The fundamental step in the working of RBO is the calculation
 *  of overlap $X_d$, or size of intersection, between the two rankings 
 *  at each depth.  The key insight is that: 
 *
 *     $X_{d+1} = X_{d} + I(S_{d+1} \in T_{1:{d}}) 
 *                      + I(T_{d+1} \in S_{1:{d}})
 *                      + I(T_{d+1} == T_{s+1})
 *
 *  where $S$ and $T$ are the two lists, and $I$ is the indicator function,
 *  return $1$ if the enclosed statement is true, $0$ otherwise.
 *  That is, the overlap at the next depth is the overlap at the current
 *  depth, plus one each if the next element in one list is found by
 *  the next depth in the other list.  To implement this efficiently,
 *  we keep a lookup set of the elements encountered in each list to date.
 *  Note that we do not require separate lookup sets for each list: we 
 *  only record elements if they've only been encountered once.
 */

namespace similarity {


/**
 *  Initialize the RBO state.
 */
void rbo_state_init(rbo_state_t * st, double p) {
    st->depth = 0;
    st->overlap = 0;
    st->wgt = (1 - p) / p;
    st->rbo = 0.0;
    st->p = p;
    st->seen.clear();
    st->short_depth = -1;
    st->short_overlap = -1;
}

/**
 *  Update the RBO state with two new strings.
 */
void rbo_state_update(rbo_state_t * st, IdType e1, IdType e2) {
    /* Make sure that rbo_mark_end_short has not been called. */
    CHECK(st->short_depth == -1);
    if (e1 == e2) {
        st->overlap += 1;
    } else {
        if (st->seen.erase(e1) == 1) {
            st->overlap += 1;
        } else {
            st->seen.insert(e1);
        }
        if (st->seen.erase(e2) == 1) {
            st->overlap += 1;
        } else {
            st->seen.insert(e2);
        }
    }
    st->depth += 1;
    st->wgt *= st->p;
    st->rbo += ((double) st->overlap / st->depth) * st->wgt;
}

/**
 *  Mark a state to show that we have reached the end of the shorter list.
 */
void rbo_mark_end_short(rbo_state_t * st) {
    st->short_depth = st->depth;
    st->short_overlap = st->overlap;
}

/**
 *  Update the RBO state with a single string.
 *
 *  This indicates that we have uneven lists, and the shorter list has
 *  finished.
 */
void rbo_state_update_uneven(rbo_state_t * st, IdType s) {
    /* rbo_mark_end_short must have been called. */
    CHECK(st->short_depth != -1);
    if (st->seen.erase(s) == 1) {
        st->overlap += 1;
    }
    st->depth += 1;
    st->wgt *= st->p;
    /* Contribution from overlap */
    st->rbo += ((double) st->overlap / st->depth) * st->wgt;
    /* Extrapolation of overlap at end of short list */
    st->rbo += ((st->short_overlap * ((double) st->depth - st->short_depth)) /
        (st->depth * st->short_depth)) * st->wgt;
}

/**
 *  Calculate final, extrapolated RBO.
 */
double rbo_calc_extrapolated(rbo_state_t * st) {
    double p_l = pow(st->p, st->depth);
    CHECK(fabs((st->wgt * st->p) / (1 - st->p) - p_l) < 0.00001);
    if  (st->short_depth == -1) {
        rbo_mark_end_short(st);
    }
    return st->rbo + ((st->overlap - st->short_overlap) / (double) st->depth
      + (st->short_overlap / (double) st->short_depth)) * p_l;
}

/**
 *  Clear an rbo state.
 */
void rbo_state_clear(rbo_state_t * st) {
    st->seen.clear();
}

double ComputeRBO(const vector<IdType>& ids1, const vector<IdType>& ids2, float p) {
  rbo_state_t rbo_st;

  rbo_state_init(&rbo_st, p);

  size_t rank = 0;
  size_t qtyMin = std::min(ids1.size(), ids2.size());
  size_t qtyMax = std::max(ids1.size(), ids2.size());

  if (!qtyMin) return 0; /* William's code would return NAN
                          * but it seems more natural to assume that
                          * if one list is empty then overlap is empty
                          * at any rank
                         */

  for (;rank < qtyMin;++rank) {
    rbo_state_update(&rbo_st, ids1[rank], ids2[rank]);
  }

  if (rank < qtyMax) { 
    rbo_mark_end_short(&rbo_st);

    // At this point only one of the following loops will be executed!
    for (;rank < ids1.size();++rank) {
      rbo_state_update_uneven(&rbo_st, ids1[rank]);
    }
    for (;rank < ids2.size();++rank) {
      rbo_state_update_uneven(&rbo_st, ids2[rank]);
    }
  }

  double rbo = rbo_calc_extrapolated(&rbo_st);

  rbo_state_clear(&rbo_st);
  
  return rbo;
}

};
