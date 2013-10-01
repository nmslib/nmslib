/**
 * Non-metric Space Library
 *
 * Authors: Bilegsaikhan Naidan (https://github.com/bileg), Leonid Boytsov (http://boytsov.info).
 * With contributions from Lawrence Cayton (http://lcayton.com/).
 *
 * For the complete list of contributors and further details see:
 * https://github.com/searchivarius/NonMetricSpaceLib 
 * 
 * Copyright (c) 2010--2013
 *
 * This code is released under the
 * Apache License Version 2.0 http://www.apache.org/licenses/.
 *
 */

#include "searchoracle.h"
#include "vptree.h"
#include "methodfactory.h"

namespace similarity {

/* 
 * We have two different creating functions, 
 * b/c there can be two different oracle types.
 */
template <typename dist_t>
Index<dist_t>* CreateVPTreeTriang(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
    AnyParamManager pmgr(AllParams);

    double AlphaLeft = 1.0, AlphaRight = 1.0;

    pmgr.GetParamOptional("alphaLeft",  AlphaLeft);
    pmgr.GetParamOptional("alphaRight", AlphaRight);

    TriangIneqCreator<dist_t> OracleCreator(AlphaLeft, AlphaRight);

    AnyParams RemainParams = pmgr.ExtractParametersExcept({"alphaLeft", "alphaRight"});

    return new VPTree<dist_t, TriangIneq<dist_t>, TriangIneqCreator<dist_t> >(
                                                PrintProgress,
                                                OracleCreator,
                                                space,
                                                DataObjects,
                                                RemainParams
                                                );
}

template <typename dist_t>
Index<dist_t>* CreateVPTreeSample(bool PrintProgress,
                           const string& SpaceType,
                           const Space<dist_t>* space,
                           const ObjectVector& DataObjects,
                           const AnyParams& AllParams) {
    AnyParamManager pmgr(AllParams);

    bool      DoRandSample                  = true;
    unsigned  MaxK                          = 100;
    float     QuantileStepPivot             = 0.005;
    float     QuantileStepPseudoQuery       = 0.001;
    size_t    NumOfPseudoQueriesInQuantile  = 5;
    float     DistLearnThreshold            = 0.05;

    pmgr.GetParamOptional("doRandSample",            DoRandSample);
    pmgr.GetParamOptional("maxK",                    MaxK);
    pmgr.GetParamOptional("quantileStepPivot",       QuantileStepPivot);
    pmgr.GetParamOptional("quantileStepPseudoQuery", QuantileStepPseudoQuery);
    pmgr.GetParamOptional("numOfPseudoQueriesInQuantile", NumOfPseudoQueriesInQuantile);
    pmgr.GetParamOptional("distLearnThresh",         DistLearnThreshold);

    AnyParams RemainParams = pmgr.ExtractParametersExcept(
                        {"doRandSample", 
                         "maxK",
                         "quantileStepPivot",
                         "quantileStepPseudoQuery",
                         "quantileStepPseudoQuery",
                         "distLearnThresh"
                        });

    SamplingOracleCreator<dist_t> OracleCreator(space,
                                                 DataObjects,
                                                 DoRandSample,
                                                 MaxK,
                                                 QuantileStepPivot,
                                                 QuantileStepPseudoQuery,
                                                 NumOfPseudoQueriesInQuantile,
                                                 DistLearnThreshold);

    return new VPTree<dist_t, SamplingOracle<dist_t>, SamplingOracleCreator<dist_t> >(
                                                 PrintProgress,
                                                 OracleCreator,
                                                 space,
                                                 DataObjects,
                                                 RemainParams
                                                 );
}

/*
 * End of creating functions.
 */

/*
 * Let's register creating functions in a method factory.
 *
 * We need to do this for several distance types (int, float, double).
 *
 * IMPORTANT NOTE: don't include this source-file into a library.
 * Sometimes C++ carries out a lazy initialization of global objects
 * that are stored in a library. Then, the registration code doesn't work.
 */

REGISTER_METHOD_CREATOR(int,    METH_VPTREE, CreateVPTreeTriang)
REGISTER_METHOD_CREATOR(float,  METH_VPTREE, CreateVPTreeTriang)
REGISTER_METHOD_CREATOR(double, METH_VPTREE, CreateVPTreeTriang)

REGISTER_METHOD_CREATOR(int,    METH_VPTREE_SAMPLE, CreateVPTreeSample)
REGISTER_METHOD_CREATOR(float,  METH_VPTREE_SAMPLE, CreateVPTreeSample)
REGISTER_METHOD_CREATOR(double, METH_VPTREE_SAMPLE, CreateVPTreeSample)

}

