#!/usr/bin/python

import sys
import nmslib_generic

def read_data(file_name):
    f=open(file_name) 
    for line in f:
        yield line.strip()

def test_vector():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'

    index = nmslib_generic.initIndex(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_generic.DistType.FLOAT)

    for id, data in enumerate(read_data('sample_dataset.txt')):
      nmslib_generic.addDataPoint(index, id, data)
    print 'Let\'s invoke the index-build process'

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_generic.buildIndex(index, index_param)
    nmslib_generic.setQueryTimeParams(index,query_time_param)

    print 'The index is created'

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib_generic.knnQuery(index, k, data)

    nmslib_generic.freeIndex(index)


def test_string():
    DATA_STRS = ["xyz", "beagcfa", "cea", "cb",
                  "d", "c", "bdaf", "ddcd",
                  "egbfa", "a", "fba", "bcccfe",
                  "ab", "bfgbfdc", "bcbbgf", "bfbb"
    ]
    QUERY_STRS = ["abc", "def", "ghik"]
    space_type = 'leven'
    space_param = []
    method_name = 'small_world_rand'

    index = nmslib_generic.initIndex(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_generic.DistType.INT)
    for id, data in enumerate(DATA_STRS):
        nmslib_generic.addDataPoint(index, id, data)

    print 'Let\'s invoke the index-build process'

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=1']

    nmslib_generic.buildIndex(index, index_param)
    nmslib_generic.setQueryTimeParams(index,query_time_param)

    print 'The index is created'

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_generic.knnQuery(index, k, data)
    nmslib_generic.freeIndex(index)

if __name__ == '__main__':

    print nmslib_generic.DistType.INT
    print nmslib_generic.DistType.FLOAT

    test_vector()
    test_string()



