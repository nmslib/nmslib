/**
 * Non-metric Space Library
 *
 * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib
 *
 * Copyright (c) 2013-2018
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */
#ifndef _MULTI_VANTAGE_POINT_TREE_UTILS_H_
#define _MULTI_VANTAGE_POINT_TREE_UTILS_H_

namespace similarity {

template <class T>
T Remove(std::vector<T>& array, int index) {
  T result = array[index];
  array.erase(array.begin() + index);
  return result;
}

template <class T>
T SplitByMedian(std::vector<T>& data,
                std::vector<T>& split1,
                std::vector<T>& split2) {
  CHECK(!data.empty());
  size_t index_of_median = data.size() >> 1;
  T median = data[index_of_median];
  size_t i = 0;
  for ( ; i <= index_of_median; ++i) {
    split1.push_back(data[i]);
  }
  for ( ; i < data.size(); ++i) {
    split2.push_back(data[i]);
  }
  std::vector<T>().swap(data);
  return median;
}

}  // namespace similarity

#endif    // _MULTI_VANTAGE_POINT_TREE_UTILS_H_
