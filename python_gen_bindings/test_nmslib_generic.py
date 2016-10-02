#!/usr/bin/python

import sys
import nmslib_generic

MAX_PRINT_QTY=50

def read_data(file_name):
    f=open(file_name) 
    for line in f:
        yield line.strip()

def test_vector_fresh():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'

    index = nmslib_generic.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_generic.DistType.FLOAT)

    for id, data in enumerate(read_data('sample_dataset.txt')):
      nmslib_generic.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_generic.getDataPointQty(index) 


    for i in range(0,min(MAX_PRINT_QTY,nmslib_generic.getDataPointQty(index))):
       print nmslib_generic.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_generic.createIndex(index, index_param)

    print 'The index is created'

    nmslib_generic.setQueryTimeParams(index,query_time_param)

    print 'Query time parameters are set'

    print "Results for the freshly created index:"

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib_generic.knnQuery(index, k, data)

    nmslib_generic.saveIndex(index, index_name)

    print "The index %s is saved" % index_name

    nmslib_generic.freeIndex(index)

def test_vector_loaded():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'

    index = nmslib_generic.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_generic.DistType.FLOAT)

    for id, data in enumerate(read_data('sample_dataset.txt')):
      nmslib_generic.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_generic.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_generic.getDataPointQty(index))):
       print nmslib_generic.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_generic.loadIndex(index, index_name)

    print 'The index %s is loaded' % index_name

    nmslib_generic.setQueryTimeParams(index,query_time_param)

    print 'Query time parameters are set'

    print "Results for the loaded index:"

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib_generic.knnQuery(index, k, data)

    nmslib_generic.freeIndex(index)


def test_string_fresh():
    DATA_STRS = ["xyz", "beagcfa", "cea", "cb",
                  "d", "c", "bdaf", "ddcd",
                  "egbfa", "a", "fba", "bcccfe",
                  "ab", "bfgbfdc", "bcbbgf", "bfbb"
    ]
    QUERY_STRS = ["abc", "def", "ghik"]
    space_type = 'leven'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'

    index = nmslib_generic.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_generic.DistType.INT)

    for id, data in enumerate(DATA_STRS):
      nmslib_generic.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_generic.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_generic.getDataPointQty(index))):
       print nmslib_generic.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=1']

    nmslib_generic.createIndex(index, index_param)

    print 'The index is created'

    nmslib_generic.setQueryTimeParams(index,query_time_param)

    print 'Query time parameters are set'

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_generic.knnQuery(index, k, data)

    nmslib_generic.saveIndex(index, index_name)

    print "The index %s is saved" % index_name

    nmslib_generic.freeIndex(index)

def test_string_loaded():
    DATA_STRS = ["xyz", "beagcfa", "cea", "cb",
                  "d", "c", "bdaf", "ddcd",
                  "egbfa", "a", "fba", "bcccfe",
                  "ab", "bfgbfdc", "bcbbgf", "bfbb"
    ]
    QUERY_STRS = ["abc", "def", "ghik"]
    space_type = 'leven'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'

    index = nmslib_generic.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_generic.DistType.INT)

    for id, data in enumerate(DATA_STRS):
      nmslib_generic.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_generic.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_generic.getDataPointQty(index))):
       print nmslib_generic.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=1']

    nmslib_generic.loadIndex(index, index_name)

    print 'The index %s is loaded' % index_name

    nmslib_generic.setQueryTimeParams(index,query_time_param)

    print 'Query time parameters are set'

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_generic.knnQuery(index, k, data)

    nmslib_generic.freeIndex(index)

if __name__ == '__main__':

    print nmslib_generic.DistType.INT
    print nmslib_generic.DistType.FLOAT

    test_vector_fresh()
    test_vector_loaded()
    test_string_fresh()
    test_string_loaded()



