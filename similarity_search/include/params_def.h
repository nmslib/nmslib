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
#ifndef PARAMS_DEF_H
#define PARAMS_DEF_H

// Definition of most command-line parameters
#define HELP_PARAM_OPT  "help,h"
#define HELP_PARAM_MSG "produce help message"


#define SPACE_TYPE_PARAM_OPT    "spaceType,s"
#define SPACE_TYPE_PARAM_MSG    "space type, e.g., l1, l2, lp"

#define DIST_TYPE_PARAM_OPT     "distType"
#define DIST_TYPE_PARAM_MSG     "distance value type: int, float, double"

#define DATA_FILE_PARAM_OPT     "dataFile,i"
#define DATA_FILE_PARAM_MSG     "input data file"

#define MAX_NUM_DATA_PARAM_OPT  "maxNumData"
#define MAX_NUM_DATA_PARAM_MSG  "if non-zero, only the first maxNumData elements are used"
#define MAX_NUM_DATA_PARAM_DEFAULT 0

#define QUERY_FILE_PARAM_OPT    "queryFile,q"
#define QUERY_FILE_PARAM_MSG    "query file"
#define QUERY_FILE_PARAM_DEFAULT ""

#define LOAD_INDEX_PARAM_OPT    "loadIndex"
#define LOAD_INDEX_PARAM_MSG    "a location to load the index from "
#define LOAD_INDEX_PARAM_DEFAULT ""

#define SAVE_INDEX_PARAM_OPT    "saveIndex"
#define SAVE_INDEX_PARAM_MSG    "a location to save the index to "
#define SAVE_INDEX_PARAM_DEFAULT ""

#define CACHE_PREFIX_GS_PARAM_OPT     "cachePrefixGS"
#define CACHE_PREFIX_GS_PARAM_MSG     "a prefix of gold standard cache files"
#define CACHE_PREFIX_GS_PARAM_DEFAULT ""

#define MAX_CACHE_GS_QTY_PARAM_OPT "maxCacheGSQty"
#define MAX_CACHE_GS_QTY_PARAM_MSG "a maximum number of gold standard entries to compute/cache"
#define MAX_CACHE_GS_QTY_PARAM_DEFAULT     1000

#define LOG_FILE_PARAM_OPT       "logFile,l"
#define LOG_FILE_PARAM_MSG       "log file"
#define LOG_FILE_PARAM_DEFAULT   ""

#define MAX_NUM_QUERY_PARAM_OPT   "maxNumQuery"
#define MAX_NUM_QUERY_PARAM_MSG   "if non-zero, use maxNumQuery query elements (required in the case of bootstrapping)"
#define MAX_NUM_QUERY_PARAM_DEFAULT 0

#define TEST_SET_QTY_PARAM_OPT   "testSetQty,b"
#define TEST_SET_QTY_PARAM_MSG   "# of test sets obtained by bootstrapping; ignored if queryFile is specified"
#define TEST_SET_QTY_PARAM_DEFAULT 0

#define KNN_PARAM_OPT            "knn,k"
#define KNN_PARAM_MSG            "comma-separated values of K for the k-NN search"

#define RANGE_PARAM_OPT          "range,r"
#define RANGE_PARAM_MSG          "comma-separated radii for the range searches"

#define EPS_PARAM_OPT            "eps"
#define EPS_PARAM_MSG            "the parameter for the eps-approximate k-NN search."
#define EPS_PARAM_DEFAULT        0.0

#define QUERY_TIME_PARAMS_PARAM_OPT "queryTimeParams"
#define QUERY_TIME_PARAMS_PARAM_MSG "query-time method(s) parameters in the format:\nparam1=value1,param2=value2,...,paramK=valueK"

#define INDEX_TIME_PARAMS_PARAM_OPT "createIndex"
#define INDEX_TIME_PARAMS_PARAM_MSG "index-time method(s) parameters in the format:\nparam1=value1,param2=value2,...,paramK=valueK"

#define METHOD_PARAM_OPT         "method,m"
#define METHOD_PARAM_MSG         "method name"
#define METHOD_PARAM_DEFAULT     ""

#define THREAD_TEST_QTY_PARAM_OPT         "threadTestQty"
#define THREAD_TEST_QTY_PARAM_MSG         "# of threads during querying"
#define THREAD_TEST_QTY_PARAM_DEFAULT     1

#define OUT_FILE_PREFIX_PARAM_OPT      "outFilePrefix,o"
#define OUT_FILE_PREFIX_PARAM_MSG      "output file prefix"
#define OUT_FILE_PREFIX_PARAM_DEFAULT  ""

#define APPEND_TO_REF_FILE_PARAM_OPT      "appendToResFile"
#define APPEND_TO_REF_FILE_PARAM_MSG      "do not override information in results files, append new data"
#define APPEND_TO_REF_FILE_PARAM_DEFAULT  false

#define PRINT_PROGRESS_PARAM_OPT          "printProgress,p"
#define PRINT_PROGRESS_PARAM_MSG          "display (mostly indexing) progress (for some methods)"
#define PRINT_PROGRESS_PARAM_DEFAULT      true

    

#endif
