#!/usr/bin/python

import sys
import nmslib_vector

MAX_PRINT_QTY=50

def read_data(fn):
    with open(fn) as f:
      for line in f:
        yield [float(v) for v in line.strip().split()]

def test_vector_fresh():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.VECTOR,
                             nmslib_vector.DistType.FLOAT)

    for id, data in enumerate(read_data('sample_dataset.txt')):
        nmslib_vector.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_vector.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_vector.getDataPointQty(index))):
       print nmslib_vector.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'


    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_vector.createIndex(index, index_param)

    print 'The index is created'

    nmslib_vector.setQueryTimeParams(index,query_time_param)

    print 'Query time parameters are set'

    print "Results for the freshly created index:"

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib_vector.knnQuery(index, k, data)

    nmslib_vector.saveIndex(index, index_name)

    print "The index %s is saved" % index_name

    nmslib_vector.freeIndex(index)

def test_vector_loaded():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.VECTOR,
                             nmslib_vector.DistType.FLOAT)

    for id, data in enumerate(read_data('sample_dataset.txt')):
        nmslib_vector.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_vector.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_vector.getDataPointQty(index))):
       print nmslib_vector.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'


    query_time_param = ['initSearchAttempts=3']

    nmslib_vector.loadIndex(index, index_name)

    print "The index %s is loaded" % index_name
  
    nmslib_vector.setQueryTimeParams(index,query_time_param)

    print 'Query time parameters are set'

    print "Results for the loaded index"

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib_vector.knnQuery(index, k, data)

    nmslib_vector.freeIndex(index)


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
    index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.STRING,
                             nmslib_vector.DistType.INT)
    for id, data in enumerate(DATA_STRS):
        nmslib_vector.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_vector.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_vector.getDataPointQty(index))):
       print nmslib_vector.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'


    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_vector.createIndex(index, index_param)
    nmslib_vector.setQueryTimeParams(index, query_time_param)

    print 'Query time parameters are set'

    print "Results for the freshly created index:"

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_vector.knnQuery(index, k, data)

    nmslib_vector.saveIndex(index, index_name)

    print "The index %s is saved" % index_name

    nmslib_vector.freeIndex(index)

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
    index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.STRING,
                             nmslib_vector.DistType.INT)
    for id, data in enumerate(DATA_STRS):
        nmslib_vector.addDataPoint(index, id, data)

    print 'Let\'s print a few data entries'
    print 'We have added %d data points' % nmslib_vector.getDataPointQty(index) 

    for i in range(0,min(MAX_PRINT_QTY,nmslib_vector.getDataPointQty(index))):
       print nmslib_vector.getDataPoint(index,i)

    print 'Let\'s invoke the index-build process'


    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_vector.loadIndex(index, index_name)

    print "The index %s is loaded" % index_name

    nmslib_vector.setQueryTimeParams(index, query_time_param)

    print 'Query time parameters are set'

    print "Results for the loaded index:"

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_vector.knnQuery(index, k, data)

    nmslib_vector.freeIndex(index)

if __name__ == '__main__':

    print nmslib_vector.DataType.VECTOR
    print nmslib_vector.DataType.STRING
    print nmslib_vector.DistType.INT
    print nmslib_vector.DistType.FLOAT

    test_vector_fresh()
    test_vector_loaded()
    test_string_fresh()
    test_string_loaded()



