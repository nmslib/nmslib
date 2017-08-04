#!/usr/bin/python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import os
import time
import numpy as np
import nmslib
from common import *


def bench_sparse_vector(batch=True):
    # delay importing these so CI can import module
    from scipy.sparse import csr_matrix
    from scipy.spatial import distance
    from pysparnn.cluster_index import MultiClusterIndex

    dim = 20000
    dataset = np.random.binomial(1, 0.01, size=(40000, dim))
    queryset = np.random.binomial(1, 0.009, size=(1000, dim))

    print('dataset[0]:', [[i, v] for i, v in enumerate(dataset[0]) if v > 0])

    k = 3

    q0 = queryset[0]
    res = []
    for i in range(dataset.shape[0]):
        res.append([i, distance.cosine(q0, dataset[i,:])])
    res.sort(key=lambda x: x[1])
    print('q0 res', res[:k])

    data_matrix = csr_matrix(dataset, dtype=np.float32)
    query_matrix = csr_matrix(queryset, dtype=np.float32)

    data_to_return = range(dataset.shape[0])

    with TimeIt('building MultiClusterIndex'):
        cp = MultiClusterIndex(data_matrix, data_to_return)

    with TimeIt('knn search'):
        res = cp.search(query_matrix, k=k, return_distance=False)

    print(res[:5])
    for i in res[0]:
        print(int(i), distance.cosine(q0, dataset[int(i),:]))

    #space_type = 'cosinesimil_sparse'
    space_type = 'cosinesimil_sparse_fast'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '_sparse.index'
    if os.path.isfile(index_name):
        os.remove(index_name)
    index = nmslib.init(space_type,
                        space_param,
                        method_name,
                        nmslib.DataType.SPARSE_VECTOR,
                        nmslib.DistType.FLOAT)

    if batch:
        with TimeIt('batch add'):
            positions = nmslib.addDataPointBatch(index, np.arange(len(dataset), dtype=np.int32), data_matrix)
        print('positions', positions)
    else:
        d = []
        q = []
        with TimeIt('preparing'):
            for data in dataset:
                d.append([[i, v] for i, v in enumerate(data) if v > 0])
            for data in queryset:
                q.append([[i, v] for i, v in enumerate(data) if v > 0])
        with TimeIt('adding points'):
            for id, data in enumerate(d):
                nmslib.addDataPoint(index, id, data)

    print('Let\'s invoke the index-build process')

    index_param = ['NN=17', 'efConstruction=50', 'indexThreadQty=4']
    query_time_param = ['efSearch=50']

    with TimeIt('building index'):
        nmslib.createIndex(index, index_param)

    print('The index is created')

    nmslib.setQueryTimeParams(index,query_time_param)

    print('Query time parameters are set')

    print("Results for the freshly created index:")

    with TimeIt('knn query'):
        if batch:
            num_threads = 10
            res = nmslib.knnQueryBatch(index, num_threads, k, query_matrix)
            for idx, v in enumerate(res):
                if idx < 5:
                    print(idx, v)
                if idx == 0:
                    for i in v:
                        print('q0', i, distance.cosine(q0, dataset[i,:]))
        else:
            for idx, data in enumerate(q):
                res = nmslib.knnQuery(index, k, data)
                if idx < 5:
                    print(idx, res)

    nmslib.saveIndex(index, index_name)

    print("The index %s is saved" % index_name)

    nmslib.freeIndex(index)



if __name__ == '__main__':

    # for debugging purpose!
    np.random.seed(17)

    bench_sparse_vector()


