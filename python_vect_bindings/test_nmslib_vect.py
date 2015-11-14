#!/usr/bin/python

import sys
import nmslib_vector

def read_data(f):
    for line in open(f).readlines():
        yield [float(v) for v in line.strip().split()]

def test_vector():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index = nmslib_vector.initIndex(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.VECTOR,
                             nmslib_vector.DistType.FLOAT)

    for pos, data in enumerate(read_data('sample_dataset.txt')):
        nmslib_vector.addDataPoint(index, data)

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_vector.buildIndex(index, index_param)
    nmslib_vector.setQueryTimeParams(index,query_time_param)

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib_vector.knnQuery(index, k, data)

    nmslib_vector.freeIndex(index)


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
    index = nmslib_vector.initIndex(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.STRING,
                             nmslib_vector.DistType.INT)
    for pos, data in enumerate(DATA_STRS):
        nmslib_vector.addDataPoint(index, data)

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib_vector.buildIndex(index, index_param)
    nmslib_vector.setQueryTimeParams(index, query_time_param)

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_vector.knnQuery(index, k, data)
    nmslib_vector.freeIndex(index)

if __name__ == '__main__':

    print nmslib_vector.DataType.VECTOR
    print nmslib_vector.DataType.STRING
    print nmslib_vector.DistType.INT
    print nmslib_vector.DistType.FLOAT

    test_vector()
    test_string()



