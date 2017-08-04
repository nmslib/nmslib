#!/usr/bin/python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import sys
import os
import time
import numpy as np
import nmslib
from common import *

MAX_PRINT_QTY=5

def read_data(fn):
    with open(fn) as f:
        for line in f:
            yield [float(v) for v in line.strip().split()]


def read_data_fast(fn, sep='\t'):
    import pandas
    df = pandas.read_csv(fn, sep=sep, header=None)
    return np.ascontiguousarray(df.as_matrix(), dtype=np.float32)


def read_data_fast_batch(fn, batch_size, sep='\t'):
    import pandas
    for df in pandas.read_csv(fn, sep=sep, header=None, chunksize=batch_size):
        yield np.ascontiguousarray(df.as_matrix(), dtype=np.float32)


def read_sparse_data(fn):
    with open(fn) as f:
        for line in f:
            yield [[i, float(v)] for i, v in enumerate(line.split()) if float(v) > 0]


def read_data_as_string(fn):
    with open(fn) as f:
        for line in f:
            yield line.strip()


def test_vector_load(fast=True, fast_batch=True, seq=True):
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    if os.path.isfile(index_name):
        os.remove(index_name)
    f = '/tmp/foo.txt'
    if not os.path.isfile(f):
        print('creating %s' % f)
        np.savetxt(f, np.random.rand(100000,1000), delimiter="\t")
        print('done')

    if fast:
        index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.DENSE_VECTOR,
                             nmslib.DistType.FLOAT)
        with TimeIt('fast add data point'):
            data = read_data_fast(f)
            nmslib.addDataPointBatch(index, np.arange(len(data), dtype=np.int32), data)
        nmslib.freeIndex(index)

    if fast_batch:
        index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.DENSE_VECTOR,
                             nmslib.DistType.FLOAT)
        with TimeIt('fast_batch add data point'):
            offset = 0
            for data in read_data_fast_batch(f, 10000):
                nmslib.addDataPointBatch(index, np.arange(len(data), dtype=np.int32) + offset, data)
                offset += data.shape[0]
        print('offset', offset)
        nmslib.freeIndex(index)

    if seq:
        index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.DENSE_VECTOR,
                             nmslib.DistType.FLOAT)
        with TimeIt('seq add data point'):
            for id, data in enumerate(read_data(f)):
                nmslib.addDataPoint(index, id, data)
        nmslib.freeIndex(index)


def test_vector_fresh(fast=True):
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    if os.path.isfile(index_name):
        os.remove(index_name)
    index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.DENSE_VECTOR,
                             nmslib.DistType.FLOAT)

    start = time.time()
    if fast:
        data = read_data_fast('sample_dataset.txt')
        print('data.shape', data.shape)
        positions = nmslib.addDataPointBatch(index, np.arange(len(data), dtype=np.int32), data)
    else:
        for id, data in enumerate(read_data('sample_dataset.txt')):
            pos = nmslib.addDataPoint(index, id, data)
            if id != pos:
                print('id %s != pos %s' % (id, pos))
                sys.exit(1)
    end = time.time()
    print('added data in %s secs' % (end - start))

    print('Let\'s print a few data entries')
    print('We have added %d data points' % nmslib.getDataPointQty(index))

    print("Distance between points (0,0) " + str(nmslib.getDistance(index, 0, 0)));
    print("Distance between points (1,1) " + str(nmslib.getDistance(index, 1, 1)));
    print("Distance between points (0,1) " + str(nmslib.getDistance(index, 0, 1)));
    print("Distance between points (1,0) " + str(nmslib.getDistance(index, 1, 0)));

    for i in range(0,min(MAX_PRINT_QTY,nmslib.getDataPointQty(index))):
       print(nmslib.getDataPoint(index, i))

    print('Let\'s invoke the index-build process')

    index_param = ['NN=17', 'efConstruction=50', 'indexThreadQty=4']
    query_time_param = ['efSearch=50']

    nmslib.createIndex(index, index_param)

    print('The index is created')

    nmslib.setQueryTimeParams(index,query_time_param)

    print('Query time parameters are set')

    print("Results for the freshly created index:")

    k = 3

    start = time.time()
    if fast:
        num_threads = 10
        query = read_data_fast('sample_queryset.txt')
        res = nmslib.knnQueryBatch(index, num_threads, k, query)
        for idx, v in enumerate(res):
            print(idx, v)
    else:
        for idx, data in enumerate(read_data('sample_queryset.txt')):
            print(idx, nmslib.knnQuery(index, k, data))
    end = time.time()
    print('querying done in %s secs' % (end - start))

    nmslib.saveIndex(index, index_name)

    print("The index %s is saved" % index_name)

    nmslib.freeIndex(index)

def test_vector_loaded():
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.DENSE_VECTOR,
                             nmslib.DistType.FLOAT)

    for id, data in enumerate(read_data('sample_dataset.txt')):
        pos = nmslib.addDataPoint(index, id, data)
        if id != pos:
            print('id %s != pos %s' % (id, pos))
            sys.exit(1)

    print('Let\'s print a few data entries')
    print('We have added %d data points' % nmslib.getDataPointQty(index))

    for i in range(0,min(MAX_PRINT_QTY,nmslib.getDataPointQty(index))):
       print(nmslib.getDataPoint(index,i))

    print('Let\'s invoke the index-build process')


    query_time_param = ['efSearch=50']

    nmslib.loadIndex(index, index_name)

    print("The index %s is loaded" % index_name)

    nmslib.setQueryTimeParams(index,query_time_param)

    print('Query time parameters are set')

    print("Results for the loaded index")

    k = 2
    for idx, data in enumerate(read_data('sample_queryset.txt')):
        print(idx, nmslib.knnQuery(index, k, data))

    nmslib.freeIndex(index)

def gen_sparse_data():
    n = 1000
    q = 100
    dim = 5000

    data = np.random.binomial(1, 0.01, size=(n, dim))
    print(data.shape)
    np.savetxt('sample_sparse_dataset.txt', data, delimiter='\t')

    query = np.random.binomial(1, 0.01, size=(q, dim))
    print(query.shape)
    np.savetxt('sample_sparse_queryset.txt', query, delimiter='\t')

def test_sparse_vector_fresh():
    space_type = 'cosinesimil_sparse_fast'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '_sparse.index'
    if os.path.isfile(index_name):
        os.remove(index_name)
    index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.SPARSE_VECTOR,
                             nmslib.DistType.FLOAT)

    for id, data in enumerate(read_sparse_data('sample_sparse_dataset.txt')):
        nmslib.addDataPoint(index, id, data)

    print('We have added %d data points' % nmslib.getDataPointQty(index))

    for i in range(0,min(MAX_PRINT_QTY,nmslib.getDataPointQty(index))):
       print(nmslib.getDataPoint(index,i))

    print('Let\'s invoke the index-build process')

    index_param = ['NN=17', 'efConstruction=50', 'indexThreadQty=4']
    query_time_param = ['efSearch=50']

    nmslib.createIndex(index, index_param)

    print('The index is created')

    nmslib.setQueryTimeParams(index,query_time_param)

    print('Query time parameters are set')

    print("Results for the freshly created index:")

    k = 3

    for idx, data in enumerate(read_sparse_data('sample_sparse_queryset.txt')):
        print(idx, nmslib.knnQuery(index, k, data))

    nmslib.saveIndex(index, index_name)

    print("The index %s is saved" % index_name)

    nmslib.freeIndex(index)


def test_string_fresh(batch=True):
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

    index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.OBJECT_AS_STRING,
                             nmslib.DistType.INT)

    if batch:
        print('DATA_STRS', DATA_STRS)
        positions = nmslib.addDataPointBatch(index, np.arange(len(DATA_STRS), dtype=np.int32), DATA_STRS)
    else:
        for id, data in enumerate(DATA_STRS):
            nmslib.addDataPoint(index, id, data)

    print('Let\'s print a few data entries')
    print('We have added %d data points' % nmslib.getDataPointQty(index))

    print("Distance between points (0,0) " + str(nmslib.getDistance(index, 0, 0)));
    print("Distance between points (1,1) " + str(nmslib.getDistance(index, 1, 1)));
    print("Distance between points (0,1) " + str(nmslib.getDistance(index, 0, 1)));
    print("Distance between points (1,0) " + str(nmslib.getDistance(index, 1, 0)));

    for i in range(0,min(MAX_PRINT_QTY,nmslib.getDataPointQty(index))):
        print(nmslib.getDataPoint(index,i))

    print('Let\'s invoke the index-build process')

    index_param = ['NN=17', 'efConstruction=50', 'indexThreadQty=4']
    query_time_param = ['efSearch=50']

    nmslib.createIndex(index, index_param)
    nmslib.setQueryTimeParams(index, query_time_param)

    print('Query time parameters are set')

    print("Results for the freshly created index:")

    k = 2
    if batch:
        num_threads = 10
        res = nmslib.knnQueryBatch(index, num_threads, k, QUERY_STRS)
    for idx, data in enumerate(QUERY_STRS):
        res = nmslib.knnQuery(index, k, data)
        print(idx, data, res, [DATA_STRS[i] for i in res])

    nmslib.saveIndex(index, index_name)

    print("The index %s is saved" % index_name)

    nmslib.freeIndex(index)


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

    index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.OBJECT_AS_STRING,
                             nmslib.DistType.INT)

    for id, data in enumerate(DATA_STRS):
        nmslib.addDataPoint(index, id, data)

    print('Let\'s print a few data entries')
    print('We have added %d data points' % nmslib.getDataPointQty(index))

    for i in range(0,min(MAX_PRINT_QTY,nmslib.getDataPointQty(index))):
        print(nmslib.getDataPoint(index,i))

    print('Let\'s invoke the index-build process')

    index_param = ['NN=17', 'efConstruction=50', 'indexThreadQty=4']
    query_time_param = ['efSearch=50']


    nmslib.loadIndex(index, index_name)

    print("The index %s is loaded" % index_name)

    nmslib.setQueryTimeParams(index, query_time_param)

    print('Query time parameters are set')

    print("Results for the loaded index:")

    k = 2
    for idx, data in enumerate(QUERY_STRS):
        print(idx, nmslib.knnQuery(index, k, data))

    nmslib.freeIndex(index)


def test_object_as_string_fresh(batch=True):
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    if os.path.isfile(index_name):
        os.remove(index_name)
    index = nmslib.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib.DataType.OBJECT_AS_STRING,
                             nmslib.DistType.FLOAT)

    if batch:
        data = [s for s in read_data_as_string('sample_dataset.txt')]
        positions = nmslib.addDataPointBatch(index, np.arange(len(data), dtype=np.int32), data)
    else:
        for id, data in enumerate(read_data_as_string('sample_dataset.txt')):
            nmslib.addDataPoint(index, id, data)

    print('Let\'s print a few data entries')
    print('We have added %d data points' % nmslib.getDataPointQty(index))

    for i in range(0,min(MAX_PRINT_QTY,nmslib.getDataPointQty(index))):
       print(nmslib.getDataPoint(index, i))

    print('Let\'s invoke the index-build process')

    index_param = ['NN=17', 'efConstruction=50', 'indexThreadQty=4']
    query_time_param = ['efSearch=50']

    nmslib.createIndex(index, index_param)

    print('The index is created')

    nmslib.setQueryTimeParams(index,query_time_param)

    print('Query time parameters are set')

    print("Results for the freshly created index:")

    k = 3

    for idx, data in enumerate(read_data_as_string('sample_queryset.txt')):
        print(idx, nmslib.knnQuery(index, k, data))

    nmslib.saveIndex(index, index_name)

    print("The index %s is saved" % index_name)

    nmslib.freeIndex(index)

if __name__ == '__main__':

    print('DENSE_VECTOR', nmslib.DataType.DENSE_VECTOR)
    print('SPARSE_VECTOR', nmslib.DataType.SPARSE_VECTOR)
    print('OBJECT_AS_STRING', nmslib.DataType.OBJECT_AS_STRING)

    print('DistType.INT', nmslib.DistType.INT)
    print('DistType.FLOAT', nmslib.DistType.FLOAT)


    test_vector_load()

    test_vector_fresh()
    test_vector_fresh(False)
    test_vector_loaded()

    gen_sparse_data()
    test_sparse_vector_fresh()

    test_string_fresh()
    test_string_fresh(False)
    test_string_loaded()

    test_object_as_string_fresh()
    test_object_as_string_fresh(False)


