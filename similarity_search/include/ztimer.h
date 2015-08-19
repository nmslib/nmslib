/**
* Non-metric Space Library
*
* Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
* With contributions from Lawrence Cayton (http://lcayton.com/) and others.
*
* For the complete list of contributors and further details see:
* https://github.com/searchivarius/NonMetricSpaceLib
*
* Copyright (c) 2014
*
* This code is released under the
* Apache License Version 2.0 http://www.apache.org/licenses/.
*
*/

#ifndef _CHRONO_TIMER_H_
#define _CHRONO_TIMER_H_

#include <chrono>
#include <cstdint>
#include <cmath>

using namespace std::chrono;
using std::round;

class WallClockTimer {
public:
  high_resolution_clock::time_point t1, t2;

  WallClockTimer() {
    reset();
  }

  void reset() {
    t1 = high_resolution_clock::now();
    t2 = t1;
  }

  uint64_t elapsed() {
    duration<double> elapsed_seconds = t2 - t1;
    return static_cast<uint64_t>(round(1e6 * elapsed_seconds.count()));
  }

  uint64_t split() {
    t2 = high_resolution_clock::now();
    return elapsed();
  }
};

#endif

