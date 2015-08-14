# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import nmslib

def read_data(f):
    for line in open(f).readlines():
        yield [float(v) for v in line.strip().split()]

def test_vector():
    n = 4500
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    method_param = ['NN=17', 'initIndexAttempts=3', 'initSearchAttempts=1', 'indexThreadQty=4']
    index = nmslib.initIndex(n,
                             space_type,
                             space_param,
                             method_name,
                             method_param,
                             nmslib.DataType.VECTOR,
                             nmslib.DistType.FLOAT)

    for pos, data in enumerate(read_data('sample_dataset.txt')):
        if pos >= n:
            break
        #print pos, data
        nmslib.setData(index, pos, data)
    print 'here'
    nmslib.buildIndex(index)

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
    method_param = ['NN=17', 'initIndexAttempts=3', 'initSearchAttempts=1', 'indexThreadQty=4']
    index = nmslib.initIndex(len(DATA_STRS),
                             space_type,
                             space_param,
                             method_name,
                             method_param,
                             nmslib.DataType.STRING,
                             nmslib.DistType.INT)
    for pos, data in enumerate(DATA_STRS):
        #print pos, data
        nmslib.setData(index, pos, data)
    nmslib.buildIndex(index)

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print idx, nmslib.knnQuery(index, k, data)
    nmslib.freeIndex(index)

if __name__ == '__main__':

    print nmslib.DataType.VECTOR
    print nmslib.DataType.STRING
    print nmslib.DataType.SQFD
    #print nmslib.test([5,2,[1,2],[3,4],[5,6],[7,8],[9,10]])
    #print nmslib.test([0,1,2,3,4,5,7,8,9])
    #print nmslib.test('abcdef ghij')
    print nmslib.DistType.INT
    print nmslib.DistType.FLOAT
    #sys.exit(0)

    test_vector()
    test_string()



