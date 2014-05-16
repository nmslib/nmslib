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

#ifndef _FACTORY_LSH_MULTIPROBE_H_
#define _FACTORY_LSH_MULTIPROBE_H_

#include "method/lsh_multiprobe.h"

namespace similarity {

/*
 * Creating functions.
 */

template <typename dist_t>
Index<dist_t>* CreateLSHMultiprobe(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
    unsigned  LSH_M = 20;
    unsigned  LSH_L = 50;
    unsigned  LSH_H = 1017881;
    float     LSH_W = 20;
    unsigned  LSH_T = 10;
    unsigned  LSH_TuneK = 1;
    float     DesiredRecall = 0.5;

    AnyParamManager pmgr(AllParams);

    pmgr.GetParamOptional("M",  LSH_M);
    pmgr.GetParamOptional("L",  LSH_L);
    pmgr.GetParamOptional("H",  LSH_H);
    pmgr.GetParamOptional("W",  LSH_W);
    pmgr.GetParamOptional("T",  LSH_T);
    pmgr.GetParamOptional("tuneK",  LSH_TuneK);
    pmgr.GetParamOptional("desiredRecall",  DesiredRecall);

    if (SpaceType != "l2") LOG(FATAL) << "Multiprobe LSH works only with L2";

    // For FitData():
    // number of points to use
    unsigned N1 = DataObjects.size();
    // number of pairs to sample
    unsigned P = 10000;
    // number of queries to sample
    unsigned Q = 1000;
    // search for K neighbors neighbors
    unsigned K = LSH_TuneK;

    LOG(INFO) << "lshTuneK: " << K;
    // divide the sample to F folds
    unsigned F = 10;
    // For MPLSHTune():
    // dataset size
    unsigned N2 = DataObjects.size();
    // desired recall

    return new MultiProbeLSH<dist_t>(
                  space,
                  DataObjects,
                  N1, P, Q, K, F, N2,
                  DesiredRecall,
                  LSH_L,
                  LSH_T,
                  LSH_H,
                  LSH_M,
                  LSH_W
                  );
}

/*
 * End of creating functions.
 */

}

#endif
