# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import nmslib_generic

def read_data(file_name):
    f=open(file_name) 
    for line in f:
        yield line.strip()

def test_vector():
    n = 4500
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    method_param = ['NN=17', 'initIndexAttempts=3', 'initSearchAttempts=1', 'indexThreadQty=4']
    index = nmslib_generic.initIndex(n,
                             space_type,
                             space_param,
                             method_name,
                             method_param,
                             nmslib_generic.DistType.FLOAT)

    for pos, data in enumerate(read_data('sample_dataset.txt')):
        if pos >= n:
            break
        nmslib_generic.addDataPoint(index, pos, data)
    print 'Let\'s invoke the index-build process'
    nmslib_generic.buildIndex(index)
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
    method_param = ['NN=17', 'initIndexAttempts=3', 'initSearchAttempts=1', 'indexThreadQty=4']
    index = nmslib_generic.initIndex(len(DATA_STRS),
                             space_type,
                             space_param,
                             method_name,
                             method_param,
                             nmslib_generic.DistType.INT)
    for pos, data in enumerate(DATA_STRS):
        nmslib_generic.addDataPoint(index, pos, data)

    print 'Let\'s invoke the index-build process'
    nmslib_generic.buildIndex(index)
    print 'The index is created'

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib_generic.knnQuery(index, k, data)
    nmslib_generic.freeIndex(index)

if __name__ == '__main__':

    print nmslib_generic.DistType.INT
    print nmslib_generic.DistType.FLOAT

    test_vector()
    #test_string()



