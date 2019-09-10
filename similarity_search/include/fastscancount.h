#ifndef FASTSCANCOUNT_H
#define FASTSCANCOUNT_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// credit: implementation and design by Nathan Kurz and Daniel Lemire

namespace fastscancount {

namespace {

// used by natefastscancount
uint32_t *natefastscancount_maincheck(uint8_t *counters, size_t &it,
                                      const uint32_t *d, size_t start,
                                      size_t range, uint8_t threshold,
                                      uint32_t *out) {
  range += start;
  counters -= start;
  size_t i = it;
  for (uint32_t val = d[i]; val < range; val = d[++i]) {
    uint8_t c = counters[val];
    if (c == threshold) *out++ = val;
    counters[val] = c + 1;
  }
  it = i;
  return out;
}

// used by natefastscancount
uint32_t *natefastscancount_finalcheck(uint8_t *counters, size_t &it,
                                       const uint32_t *d, size_t start,
                                       size_t itend, uint8_t threshold,
                                       uint32_t *out) {
  uint8_t *const deccounters = counters - start;
  size_t i = it;
  for (; i < itend; i++) {
    uint32_t val = d[i];
    uint8_t *location = deccounters + val;
    uint8_t c = *location;
    if (c == threshold) {
      *out++ = val;
    }
    *location = c + 1;
  }
  it = i;
  return out;
}
} // namespace

void fastscancount(const std::vector<const std::vector<uint32_t>*> &data,
                   std::vector<uint32_t> &out, uint8_t threshold) {
  size_t cache_size = 32768;
  size_t range = cache_size;
  std::vector<uint8_t> counters(cache_size);
  size_t ds = data.size();
  out.resize(out.size() + 4 * range); // let us add lots of capacity
  uint32_t *output = out.data();
  uint32_t *initout = out.data();
  std::vector<size_t> iters(ds);
  size_t countsofar = 0;
  uint32_t largest = 0;
  for (size_t c = 0; c < ds; c++) {
    if (largest < (*data[c])[data[c]->size() - 1])
      largest = (*data[c])[data[c]->size() - 1];
  }
  // we are assuming that all vectors in data are non-empty
  for (size_t start = 0; start < largest; start += range) {
    // make sure that the capacity is sufficient
    countsofar = output - initout;
    if (out.size() - countsofar < range) {
      out.resize(out.size() + 4 * range);
      initout = out.data();
      output = out.data() + countsofar;
    }
    memset(counters.data(), 0, range);
    for (size_t c = 0; c < ds; c++) {
      size_t it = iters[c]; // recover where we were
      const std::vector<uint32_t> &d = *data[c];
      const size_t itend = d.size();
      if (it == itend) // check that there is data to be processed
        continue;      // exhausted
      // check if we need to be careful:
      bool near_the_end = (d[itend - 1] < start + range);
      if (near_the_end) {
        output = natefastscancount_finalcheck(counters.data(), it, d.data(),
                                              start, itend, threshold, output);
      } else {
        output = natefastscancount_maincheck(counters.data(), it, d.data(),
                                             start, range, threshold, output);
      }
      iters[c] = it; // store it for next round
    }
  }
  countsofar = output - initout;
  out.resize(countsofar);
}
} // namespace fastscancount

#endif
