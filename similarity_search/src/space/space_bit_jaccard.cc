///**
// * Non-metric Space Library
// *
// * Main developers: Bilegsaikhan Naidan, Leonid Boytsov, Yury Malkov, Ben Frederickson, David Novak
// *
// * For the complete list of contributors and further details see:
// * https://github.com/searchivarius/NonMetricSpaceLib
// *
// * Copyright (c) 2013-2018
// *
// * This code is released under the
// * Apache License Version 2.0 http://www.apache.org/licenses/.
// *
// */
//#include <cmath>
//#include <fstream>
//#include <string>
//#include <sstream>
//#include <bitset>
//
//#include "space/space_bit_jaccard.h"
//#include "permutation_utils.h"
//#include "logging.h"
//#include "distcomp.h"
//#include "read_data.h"
//#include "experimentconf.h"
//
////namespace similarity {
////
//////using namespace std;
////
//////template <typename dist_t, typename dist_uint_t>
//////dist_t SpaceBitJaccard<dist_t,dist_uint_t>::HiddenDistance(const Object* obj1, const Object* obj2) const {
//////  CHECK(obj1->datalength() > 0);
//////  CHECK(obj1->datalength() == obj2->datalength());
//////  const dist_uint_t* x = reinterpret_cast<const dist_uint_t*>(obj1->data());
//////  const dist_uint_t* y = reinterpret_cast<const dist_uint_t*>(obj2->data());
//////  const size_t length = obj1->datalength() / sizeof(dist_uint_t)
//////                        - 1; // the last integer is an original number of elements
//////
//////  return BitJaccard(x, y, length);
//////}
////
////}
