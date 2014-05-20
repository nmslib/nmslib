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

#ifndef _FACTORY_VPTREE_H_
#define _FACTORY_VPTREE_H_

#include "searchoracle.h"
#include <method/vptree.h>

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
    float     QuantileStepPivot             = 0.005f;
    float     QuantileStepPseudoQuery       = 0.001f;
    size_t    NumOfPseudoQueriesInQuantile  = 5;
    float     DistLearnThreshold            = 0.05f;

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
}

#endif
