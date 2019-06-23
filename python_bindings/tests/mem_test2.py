import itertools
import tempfile
import unittest

import numpy as np
import numpy.testing as npt

import nmslib
import time

import os, gc, psutil

from .bindings_test import *

class MemoryLeak2TestCase(TestCaseBase):
    def testMemoryLeak2(self):
        process = psutil.Process(os.getpid())

        np.random.seed(23)
        data = np.random.randn(MEM_TEST_DATA_QTY, 10).astype(np.float32)
        query = np.random.randn(MEM_TEST_QUERY_QTY, 10).astype(np.float32)
        space_name = 'l2'

        num_threads=4

        index_time_params = {'M': 20, 
                             'efConstruction': 100, 
                             'indexThreadQty': num_threads,
                             'post' : 0,
                             'skip_optimized_index' : 1} # using non-optimized index!

        query_time_params = {'efSearch': 100}

        gc.collect()
        time.sleep(0.25)

        init_mem = process.memory_info().rss


        fail_qty = 0
        test_qty = 0
        delta_first = None

        for tid in range(MEM_TEST_REPEAT_QTY2):
            with tempfile.NamedTemporaryFile() as tmp:

                gc.collect()
                init_mem = process.memory_info().rss

                delta1 = None

                for iter_id in range(MEM_TEST_ITER2):

                    index = nmslib.init(method='hnsw', space=space_name, data_type=nmslib.DataType.DENSE_VECTOR)
                    index.addDataPointBatch(data)
                    index.createIndex(index_time_params) 

                    if iter_id == 0 and tid == 0:
                      delta_first = process.memory_info().rss - init_mem

                    delta_curr = process.memory_info().rss - init_mem

                    #print('Step %d mem deltas current: %d first: %d ratio %f' % (iter_id, delta_curr, delta_first, float(delta_curr)/max(delta_first, 1)))

                    index.setQueryTimeParams(query_time_params)
                    nbrs = index.knnQueryBatch(query, k = 10, num_threads = num_threads)
                
                    nbrs = None 
                    index = None

                    gc.collect()


                gc.collect()
                time.sleep(0.25)
                delta_last = process.memory_info().rss - init_mem
                #print('Delta last %d' % delta_last)
             
                test_qty += 1
                if delta_last >= delta_first * MEM_GROW_COEFF:
                  fail_qty += 1


        print('Fail qty %d out of %d' % (fail_qty, test_qty))
        self.assertTrue(fail_qty < MEM_TEST_ITER2 * MEM_TEST_CRIT_FAIL_RATE)



if __name__ == "__main__":
    unittest.main()
