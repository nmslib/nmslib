#!/usr/bin/python

import sys
import os
import time
import numpy as np
import nmslib_vector

MAX_PRINT_QTY=50

def read_data(fn):
    with open(fn) as f:
      for line in f:
        yield [float(v) for v in line.strip().split()]


def read_data_fast(fn, sep='\t'):
    import pandas
    df = pandas.read_csv(fn, sep=sep, header=None)
    return df.as_matrix().astype(np.float32)


def read_data_fast_batch(fn, batch_size, sep='\t'):
    import pandas
    for df in pandas.read_csv(fn, sep=sep, header=None, chunksize=batch_size):
        yield df.as_matrix().astype(np.float32)


def test_vector_load(fast=True, fast_batch=True, seq=True):
    space_type = 'cosinesimil'
    space_param = []
    method_name = 'small_world_rand'
    index_name  = method_name + '.index'
    f = '/tmp/foo.txt'
    if not os.path.isfile(f):
        print 'creating %s' % f
	np.savetxt(f, np.random.rand(100000,1000), delimiter="\t")
	print 'done'

    if fast:
        index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.VECTOR,
                             nmslib_vector.DistType.FLOAT)
        start = time.time()
        data = read_data_fast(f)
	nmslib_vector.addDataPointBatch(index, np.arange(len(data), dtype=np.int32), data)
        end = time.time()
        print 'fast: added data in %s secs' % (end - start)
	nmslib_vector.freeIndex(index)

    if fast_batch:
        index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.VECTOR,
                             nmslib_vector.DistType.FLOAT)
        start = time.time()
	offset = 0
        for data in read_data_fast_batch(f, 10000):
	    nmslib_vector.addDataPointBatch(index, np.arange(len(data), dtype=np.int32) + offset, data)
	    offset += data.shape[0]
        end = time.time()
	print 'offset', offset
        print 'fast_batch: added data in %s secs' % (end - start)
	nmslib_vector.freeIndex(index)

    if seq:
        index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.VECTOR,
                             nmslib_vector.DistType.FLOAT)
        start = time.time()
        for id, data in enumerate(read_data(f)):
            nmslib_vector.addDataPoint(index, id, data)
        end = time.time()
        print 'seq: added data in %s secs' % (end - start)
	nmslib_vector.freeIndex(index)


def test_vector_fresh(fast=True):
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

    start = time.time()
    if fast:
        data = read_data_fast('sample_dataset.txt')
	nmslib_vector.addDataPointBatch(index, np.arange(len(data), dtype=np.int32), data)
    else:
        for id, data in enumerate(read_data('sample_dataset.txt')):
            nmslib_vector.addDataPoint(index, id, data)
    end = time.time()
    print 'added data in %s secs' % (end - start)

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

    k = 3

    start = time.time()
    if fast:
        num_threads = 10
        query = read_data_fast('sample_queryset.txt')
	res = nmslib_vector.knnQueryBatch(index, num_threads, k, query)
	for idx, v in enumerate(res):
            print idx, v
    else:
        for idx, data in enumerate(read_data('sample_queryset.txt')):
            print idx, nmslib_vector.knnQuery(index, k, data)
    end = time.time()
    print 'querying done in %s secs' % (end - start)

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


if __name__ == '__main__':

    print nmslib_vector.DataType.VECTOR
    print nmslib_vector.DataType.STRING
    print nmslib_vector.DistType.INT
    print nmslib_vector.DistType.FLOAT

    test_vector_load()

    test_vector_fresh()
    test_vector_fresh(False)
    test_vector_loaded()



