#ifndef RBO_H
#define RBO_H
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


#include <unordered_set>
#include <vector>

#include "idtype.h"

namespace similarity {

using std::unordered_set;
using std::vector;

/**
 *  State of an RBO calculation.
 *
 *  @param depth is the current depth, counting from 1.
 *  @param overlap the current overlap.
 *  @param geom the current (geometrically decreasing) weight.
 *  @param rbo is the cumulative RBO score to date.
 *  @param p the p value being used.
 *  @param seen the strings seen to date.
 */
typedef struct rbo_state {
    /* General fields */
    unsigned depth;
    unsigned overlap;
    double wgt;
    double rbo;
    double p;
    unordered_set<IdType> seen;

    /* Fields for handling uneven lists. */
    int short_depth;
    int short_overlap;
} rbo_state_t;

/**
 *  Initialize the RBO state.
 */
void rbo_state_init(rbo_state_t * st, double p);

/**
 *  Update the RBO state with two new elements.
 *
 *  Both strings must be non-empty and non-null.  See rbo_mark_end_short
 *  for dealing with the end of one of the lists.
 */
void rbo_state_update(rbo_state_t * st, const char *e1, const char *e2);

/**
 *  Inform a state to that we have reached the end of the shorter list.
 *
 *  Either list can be the short list; RBO doesn't actually care.
 */
void rbo_mark_end_short(rbo_state_t * st);

/**
 *  Update the RBO state with a single string.
 *
 *  This occurs after a single list has finished.  rbo_mark_end_short
 *  must be called first.
 */
void rbo_state_update_uneven(rbo_state_t * st, const char * s);

/**
 *  Calculate final, extrapolated RBO.
 */
double rbo_calc_extrapolated(rbo_state_t * st);

/**
 *  Clear an rbo state.
 */
void rbo_state_clear(rbo_state_t * st);

double ComputeRBO(const vector<IdType>& rank1, const vector<IdType>& rank2, float p);

};

#endif /* RBO_H */
