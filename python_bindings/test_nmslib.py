#!/usr/bin/python

import sys
import nmslib

def read_data(f):
    for line in open(f).readlines():
        yield [float(v) for v in line.strip().split()]

def test_vector():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index = nmslib.initIndex(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.VECTOR,
                             nmslib.DistType.FLOAT)

    for pos, data in enumerate(read_data('sample_dataset.txt')):
        nmslib.addDataPoint(index, data)

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib.buildIndex(index, index_param)
    nmslib.setQueryTimeParams(index,query_time_param)

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print idx, nmslib.knnQuery(index, k, data)

    nmslib.freeIndex(index)


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
    index = nmslib.initIndex(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.STRING,
                             nmslib.DistType.INT)
    for pos, data in enumerate(DATA_STRS):
        nmslib.addDataPoint(index, data)

    index_param = ['NN=17', 'initIndexAttempts=3', 'indexThreadQty=4']
    query_time_param = ['initSearchAttempts=3']

    nmslib.buildIndex(index, index_param)
    nmslib.setQueryTimeParams(index, query_time_param)

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib.knnQuery(index, k, data)
    nmslib.freeIndex(index)

if __name__ == '__main__':

    print nmslib.DataType.VECTOR
    print nmslib.DataType.STRING
    print nmslib.DistType.INT
    print nmslib.DistType.FLOAT

    test_vector()
    test_string()



