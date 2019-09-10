#ifndef FASTSCANCOUNT_AVX2_H
#define FASTSCANCOUNT_AVX2_H

// this code expects an x64 processor with AVX2

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace fastscancount {
namespace {
// credit: implementation and design by Travis Downes
static inline size_t find_next_gt(uint8_t *array, size_t size,
                                  uint8_t threshold) {
  size_t vsize = size / 32;
  __m256i *varray = (__m256i *)array;
  const __m256i comprand = _mm256_set1_epi8(threshold);
  int bits = 0;

  for (size_t i = 0; i < vsize; i++) {
    __m256i v = _mm256_loadu_si256(varray + i);
    __m256i cmp = _mm256_cmpgt_epi8(v, comprand);
    if ((bits = _mm256_movemask_epi8(cmp))) {
      return i * 32 + __builtin_ctz(bits);
    }
  }

  // tail handling
  for (size_t i = vsize * 32; i < size; i++) {
    auto v = array[i];
    if (v > threshold)
      return i;
  }

  return SIZE_MAX;
}

void populate_hits_avx(std::vector<uint8_t> &counters, size_t range,
                       size_t threshold, size_t start,
                       std::vector<uint32_t> &out) {
  uint8_t *array = counters.data();
  //size_t ro = range;
  while (true) {
    size_t next = find_next_gt(array, range, (uint8_t)threshold);
    if (next == SIZE_MAX)
      break;
    out.push_back(start + next);
    range -= (next + 1);
    array += (next + 1);
  }
}

void update_counters(const uint32_t *&it_, uint8_t *counters,
                     uint32_t range_end) {
  const uint32_t *it = it_;
  for (uint32_t e; (e = *it) < range_end; ++it) {
    counters[e]++;
  }
  it_ = it;
}

void update_counters_final(const uint32_t *&it_, const uint32_t *end,
                           uint8_t *counters) {
  //uint64_t e;
  const uint32_t *it = it_;
  for (; it != end; it++) {
    counters[*it]++;
  }
  it_ = end;
}
} // namespace

void fastscancount_avx2(std::vector<uint8_t> &counters,
                        const std::vector<const std::vector<uint32_t>*> &data,
                        std::vector<uint32_t> &out, uint8_t threshold) {
  const size_t cache_size = 40000;
  out.clear();
  const size_t dsize = data.size();

  struct data_info {
    const uint32_t *cur; // current pointer into data
    const uint32_t *end; // pointer to end
    uint32_t last;       // value of last element
    data_info(const uint32_t *cur, const uint32_t *end, uint32_t last)
        : cur{cur}, end{end}, last{last} {}
  };

  std::vector<data_info> iter_data;
  iter_data.reserve(dsize);
  for (auto &d : data) {
    iter_data.emplace_back(d->data(), d->data() + d->size(), d->back());
  }

  uint32_t csize = counters.size();
  auto cdata = counters.data();
  for (uint32_t start = 0; start < csize; start += cache_size) {
    memset(cdata, 0, cache_size * sizeof(counters[0]));
    for (auto &id : iter_data) {
      // determine if the loop will end because we get to the end of
      // data, or because we get to the end of the range
      if (__builtin_expect(id.last >= start + cache_size, 1)) {
        // the iteration is guaranteed to end because an element becomes >=
        // range_end, so we don't need to check for end of data
        update_counters(id.cur, cdata - start, start + cache_size);
      } else {
        // the iteration is guaranteed to end because we get to the end of the
        // data
        update_counters_final(id.cur, id.end, cdata - start);
      }
    }

    populate_hits_avx(counters, cache_size, threshold, start, out);
  }
}

} // namespace fastscancount
#endif
