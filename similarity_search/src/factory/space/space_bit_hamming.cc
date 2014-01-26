/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
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

#include "searchoracle.h"
#include "space_bit_hamming.h"
#include "spacefactory.h"

namespace similarity {

/*
 * Creating functions.
 */

Space<int>* CreateBitHamming(const AnyParams& /* ignoring params */) {
  return new SpaceBitHamming();
}

/*
 * End of creating functions.
 */

/*
 * Let's register creating functions in a space factory.
 *
 * IMPORTANT NOTE: don't include this source-file into a library.
 * Sometimes C++ carries out a lazy initialization of global objects
 * that are stored in a library. Then, the registration code doesn't work.
 */

REGISTER_SPACE_CREATOR(int,  SPACE_BIT_HAMMING,  CreateBitHamming)

}

