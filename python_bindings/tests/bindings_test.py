import itertools
import tempfile
import unittest
import shutil

import numpy as np
import numpy.testing as npt

import nmslib
import time

import os, gc, psutil

MEM_TEST_CRIT_FAIL_RATE=0.25 

MEM_TEST_REPEAT_QTY1=4
MEM_TEST_ITER1=10

MEM_TEST_REPEAT_QTY2=4
MEM_TEST_ITER2=5

# The key to stable memory testing is using a reasonably large number of points
MEM_TEST_DATA_QTY=25000
MEM_TEST_QUERY_QTY=200
MEM_GROW_COEFF=1.5 # This is a bit adhoc but seems to work in practice
MEM_TEST_DATA_DIM=4

def get_exact_cosine(row, data, N=10):
    scores = data.dot(row) / np.linalg.norm(data, axis=-1)
    best = np.argpartition(scores, -N)[-N:]
    return sorted(zip(best, scores[best] / np.linalg.norm(row)), key=lambda x: -x[1])


def get_hitrate(ground_truth, ids):
    return len(set(i for i, _ in ground_truth).intersection(ids))


def bit_vector_to_str(bit_vect):
    return " ".join(["1" if e else "0" for e in bit_vect])


def bit_vector_sparse_str(bit_vect):
    return " ".join([str(k) for k, b in enumerate(bit_vect) if b])

  
class TestCaseBase(unittest.TestCase):
    # Each result is a tuple (ids, dists) 
    # This version deals properly with ties by resorting the second result set 
    # to be in the same order as the first one
    def assert_allclose(self, orig, comp):
      qty = len(orig[0])
      self.assertEqual(qty, len(orig[1]))
      self.assertEqual(qty, len(comp[0]))
      ids2dist = { comp[0][k] : comp[1][k] for k in range(qty) }

      comp_resort_ids = []
      comp_resort_dists = []

      for i in range(qty):
        one_id = orig[0][i]
        comp_resort_ids.append(one_id)
        self.assertTrue(one_id in ids2dist)
        comp_resort_dists.append(ids2dist[one_id])

      npt.assert_allclose(orig,
                          (comp_resort_ids, comp_resort_dists))

class TestSparseZeroVect(TestCaseBase):
    def testSparseCosine1(self):
        X = np.array([[4., 2., 3., 1., 0., 0., 0., 0., 0.],
                      [2., 1., 0., 0., 3., 0., 1., 2., 1.],
                      [4., 2., 0., 0., 3., 1., 0., 0., 0.],
                      [0., 0., 0., 0., 0., 0., 0., 0., 0.]], dtype=np.float32)

        q = np.array([[0., 0., 0., 0., 0., 0., 0., 0., 0.]], dtype=np.float32)

        index = nmslib.init(method='hnsw', space='cosinesimil')
        index.addDataPointBatch(X)
        index.createIndex()
        nns, dists = index.knnQueryBatch(q, k=3)[0]
        self.assertEqual(len(nns), 3)
        self.assertEqual(len(dists), 3)
        npt.assert_allclose(dists, [1, 1, 1])
        self.assertEqual(index.getDistance(3, 3), 1)
        # The last element is all zeros, so any distance to it must be equal to one
        for k in range(len(X)):
            self.assertEqual(index.getDistance(k, len(X)-1), 1)


    def testSparseNoFailOnEmpty(self):
        # These are three sparse vectors, the last one is empty
        data = [ [(0,1)], [(1,1)], [] ]
        qty = len(data)

        for space in ['cosinesimil_sparse', 'cosinesimil_sparse_fast', 'cosinesimil_sparse_bin_fast', 'l2_sparse', 'l1_sparse']:
            index = nmslib.init(method='hnsw', space='cosinesimil_sparse', data_type=nmslib.DataType.SPARSE_VECTOR) 
            index.addDataPointBatch(data)
            self.assertEqual(len(index), qty)
        
            index.createIndex({'M': 30, 'indexThreadQty': 4, 'efConstruction': 100, 'post' : 0}) 
  
            if space.startswith('cosine'):
                # The last element is all zeros, so any distance to it must be equal to one
                for k in range(qty):
                    self.assertEqual(index.getDistance(k, qty - 1), 1)
        

class DenseIndexTestMixin(object):
    def _get_index(self, space='cosinesimil'):
        raise NotImplementedError()
      

    def testKnnQuery(self):
        np.random.seed(23)
        data = np.asfortranarray(np.random.randn(1000, 10).astype(np.float32))

        index = self._get_index()
        index.addDataPointBatch(data)
        index.createIndex()

        query = data[0]

        ids, distances = index.knnQuery(query, k=10)
        self.assertTrue(get_hitrate(get_exact_cosine(query, data), ids) >= 5)

        # There is a bug when different ways to specify the input query data
        # were causing the trouble: https://github.com/nmslib/nmslib/issues/370
        query = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1.])

        ids, distances = index.knnQuery(query, k=10)
        self.assertTrue(get_hitrate(get_exact_cosine(query, data), ids) >= 5)

    def testKnnQueryBatch(self):
        np.random.seed(23)
        data = np.random.randn(1000, 10).astype(np.float32)

        index = self._get_index()
        index.addDataPointBatch(data)
        index.createIndex()

        queries = data[:10]
        results = index.knnQueryBatch(queries, k=10)
        for query, (ids, distances) in zip(queries, results):
            self.assertTrue(get_hitrate(get_exact_cosine(query, data), ids) >= 5)

        # test col-major arrays
        queries = np.asfortranarray(queries)
        results = index.knnQueryBatch(queries, k=10)
        for query, (ids, distances) in zip(queries, results):
            self.assertTrue(get_hitrate(get_exact_cosine(query, data), ids) >= 5)

        # test custom ids (set id to square of each row)
        index = self._get_index()
        index.addDataPointBatch(data, ids=np.arange(data.shape[0]) ** 2)
        index.createIndex()

        queries = data[:10]
        results = index.knnQueryBatch(queries, k=10)
        for query, (ids, distances) in zip(queries, results):
            # convert from square back to row id
            ids = np.sqrt(ids).astype(int)
            self.assertTrue(get_hitrate(get_exact_cosine(query, data), ids) >= 5)

    def testReloadIndex(self):
        np.random.seed(23)
        data = np.random.randn(1000, 10).astype(np.float32)

        original = self._get_index()
        original.addDataPointBatch(data)
        original.createIndex()

        # test out saving/reloading index
        temp_dir = tempfile.mkdtemp()
        temp_file_pref = os.path.join(temp_dir, 'index')

        for save_data in [0, 1]:

            original.saveIndex(temp_file_pref, save_data=save_data)

            reloaded = self._get_index()
            if save_data == 0:
                reloaded.addDataPointBatch(data)
            reloaded.loadIndex(temp_file_pref, load_data=save_data)

            original_results = original.knnQuery(data[0])
            reloaded_results = reloaded.knnQuery(data[0])
            self.assert_allclose(original_results, reloaded_results)

        shutil.rmtree(temp_dir)


class BitVectorIndexTestMixin(object):
    def _get_index(self, space='bit_jaccard'):
        raise NotImplementedError()

    def _get_batches(self, index, nbits, num_elems, chunk_size):
        if "bit_" in str(index):
            self.bit_vector_str_func = bit_vector_to_str
        else:
            self.bit_vector_str_func = bit_vector_sparse_str

        batches = []
        for i in range(0, num_elems, chunk_size):
            strs = []
            for j in range(chunk_size):
                a = np.random.rand(nbits) > 0.5
                strs.append(self.bit_vector_str_func(a))
            batches.append([np.arange(i, i + chunk_size), strs])
        return batches

    def testKnnQuery(self):
        np.random.seed(23)

        index = self._get_index()
        batches = self._get_batches(index, 512, 2000, 1000)
        for ids, data in batches:
            index.addDataPointBatch(ids=ids, data=data)

        index.createIndex()

        s = self.bit_vector_str_func(np.ones(512))
        index.knnQuery(s, k=10)

    def testReloadIndex(self):
        np.random.seed(23)

        original = self._get_index()
        batches = self._get_batches(original, 512, 2000, 1000)
        for ids, data in batches:
            original.addDataPointBatch(ids=ids, data=data)
        original.createIndex()

        temp_dir = tempfile.mkdtemp()
        temp_file_pref = os.path.join(temp_dir, 'index')

        for save_data in [0, 1]:
            # test out saving/reloading index

            original.saveIndex(temp_file_pref, save_data=save_data)

            reloaded = self._get_index()
            if save_data == 0:
                for ids, data in batches:
                    reloaded.addDataPointBatch(ids=ids, data=data)
            reloaded.loadIndex(temp_file_pref, load_data=save_data)

            s = self.bit_vector_str_func(np.ones(512))
            original_results = original.knnQuery(s)
            reloaded_results = reloaded.knnQuery(s)
            self.assert_allclose(original_results, reloaded_results)
  
        shutil.rmtree(temp_dir)


class HNSWTestCase(TestCaseBase, DenseIndexTestMixin):
    def _get_index(self, space='cosinesimil'):
        return nmslib.init(method='hnsw', space=space)


class BitJaccardTestCase(TestCaseBase, BitVectorIndexTestMixin):
    def _get_index(self, space='bit_jaccard'):
        return nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                           dtype=nmslib.DistType.FLOAT)


class SparseJaccardTestCase(TestCaseBase, BitVectorIndexTestMixin):
    def _get_index(self, space='jaccard_sparse'):
        return nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                           dtype=nmslib.DistType.FLOAT)


class BitHammingTestCase(TestCaseBase, BitVectorIndexTestMixin):
    def _get_index(self, space='bit_hamming'):
        return nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                           dtype=nmslib.DistType.INT)


class SWGraphTestCase(TestCaseBase, DenseIndexTestMixin):
    def _get_index(self, space='cosinesimil'):
        return nmslib.init(method='sw-graph', space=space)

    def testReloadIndex(self):
        np.random.seed(23)
        data = np.random.randn(1000, 10).astype(np.float32)

        original = self._get_index()
        original.addDataPointBatch(data)
        original.createIndex()

        temp_dir = tempfile.mkdtemp()
        temp_file_pref = os.path.join(temp_dir, 'index')

        # test out saving/reloading index
        for save_data in [0, 1]:

            original.saveIndex(temp_file_pref, save_data=save_data)

            reloaded = self._get_index()
            if save_data == 0:
                reloaded.addDataPointBatch(data)
            reloaded.loadIndex(temp_file_pref, load_data=save_data)

            original_results = original.knnQuery(data[0])
            reloaded_results = reloaded.knnQuery(data[0])
            self.assert_allclose(original_results, reloaded_results)

        shutil.rmtree(temp_dir)


class BallTreeTestCase(TestCaseBase, DenseIndexTestMixin):
    def _get_index(self, space='cosinesimil'):
        return nmslib.init(method='vptree', space=space)

    def testReloadIndex(self):
        return NotImplemented


class StringTestCase(TestCaseBase):
    def testStringLeven(self):
        index = nmslib.init(space='leven',
                            dtype=nmslib.DistType.INT,
                            data_type=nmslib.DataType.OBJECT_AS_STRING,
                            method='small_world_rand')

        strings = [''.join(x) for x in itertools.permutations(['a', 't', 'c', 'g'])]

        index.addDataPointBatch(strings)

        index.addDataPoint(len(index), "atat")
        index.addDataPoint(len(index), "gaga")
        index.createIndex()

        for i, distance in zip(*index.knnQuery(strings[0])):
            npt.assert_allclose(index.getDistance(0, i), distance)

        self.assertEqual(len(index), len(strings) + 2)
        self.assertEqual(index[0], strings[0])
        self.assertEqual(index[len(index)-2], 'atat')


class SparseTestCase(TestCaseBase):
    def testSparse(self):
        index = nmslib.init(method='small_world_rand', space='cosinesimil_sparse',
                            data_type=nmslib.DataType.SPARSE_VECTOR)

        index.addDataPoint(0, [(1, 2.), (2, 3.)])
        index.addDataPoint(1, [(0, 1.), (1, 2.)])
        index.addDataPoint(2, [(2, 3.), (3, 3.)])
        index.addDataPoint(3, [(3, 1.)])

        index.createIndex()

        ids, distances = index.knnQuery([(1, 2.), (2, 3.)])
        self.assertEqual(ids[0], 0)
        npt.assert_allclose(distances[0], 0)

        self.assertEqual(len(index), 4)
        self.assertEqual(index[3], [(3, 1.0)])

class MemoryLeak1TestCase(TestCaseBase):
    def testMemoryLeak1(self):
        process = psutil.Process(os.getpid())

        np.random.seed(23)
        data = np.random.randn(MEM_TEST_DATA_QTY, MEM_TEST_DATA_DIM).astype(np.float32)
        query = np.random.randn(MEM_TEST_QUERY_QTY, MEM_TEST_DATA_DIM).astype(np.float32)
        space_name = 'l2'

        num_threads=4

        index_time_params = {'M': 20, 
                             'efConstruction': 100, 
                             'indexThreadQty': num_threads,
                             'post' : 0,
                             'skip_optimized_index' : 1} # using non-optimized index!

        query_time_params = {'efSearch': 100}

        fail_qty = 0
        test_qty = 0
        delta_first = None

        gc.collect()
        time.sleep(0.25)

        init_mem = process.memory_info().rss

        temp_dir = tempfile.mkdtemp()
        temp_file_pref = os.path.join(temp_dir, 'index')

        for tid in range(MEM_TEST_REPEAT_QTY1):


            index = nmslib.init(method='hnsw', space=space_name, data_type=nmslib.DataType.DENSE_VECTOR)
            index.addDataPointBatch(data)
    
            index.createIndex(index_time_params) 
            index.saveIndex(temp_file_pref, save_data=True)

            index = None
            gc.collect()

            for iter_id in range(MEM_TEST_ITER1):
    
                index = nmslib.init(method='hnsw', space=space_name, data_type=nmslib.DataType.DENSE_VECTOR)
                index.loadIndex(temp_file_pref, load_data=True)
                index.setQueryTimeParams(query_time_params)

                if iter_id == 0 and tid == 0:
                    delta_first = process.memory_info().rss - init_mem

                delta_curr = process.memory_info().rss - init_mem

                #print('Step %d mem deltas current: %d first: %d ratio %f' % (iter_id, delta_curr, delta_first, float(delta_curr)/max(delta_first, 1)))

                nbrs = index.knnQueryBatch(query, k = 10, num_threads = num_threads)
                
                nbrs = None 
                index = None

                gc.collect()

            gc.collect()
            time.sleep(0.25)
            delta_last = process.memory_info().rss - init_mem
            print('Delta first/last %d/%d' % (delta_first, delta_last))
         
            test_qty += 1
            if delta_last >= delta_first * MEM_GROW_COEFF:
              fail_qty += 1


        shutil.rmtree(temp_dir)
        print('')
        print('Fail qty %d out of %d' % (fail_qty, test_qty))
        self.assertTrue(fail_qty < MEM_TEST_ITER1 * MEM_TEST_CRIT_FAIL_RATE)

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

        temp_dir = tempfile.mkdtemp()
        temp_file_pref = os.path.join(temp_dir, 'index')

        for tid in range(MEM_TEST_REPEAT_QTY2):

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


        shutil.rmtree(temp_dir)
        print('')
        print('Fail qty %d out of %d' % (fail_qty, test_qty))
        self.assertTrue(fail_qty < MEM_TEST_ITER2 * MEM_TEST_CRIT_FAIL_RATE)

class TestLoadEmptyIndex(TestCaseBase):
    # A regression test for issue #452
    def testRegression452(self):
        temp_dir = tempfile.mkdtemp()
        temp_file_pref = os.path.join(temp_dir, 'index')

        for skip_optim in [0, 1]:
            print(skip_optim)
            index_params = {'skip_optimized_index' : skip_optim}
            index = nmslib.init(method='hnsw', space='l2')
            index.createIndex()
            index.saveIndex(temp_file_pref, True)
            index.loadIndex(temp_file_pref, True)

        shutil.rmtree(temp_dir)


class GlobalTestCase(TestCaseBase):
    def testGlobal(self):
        # this is a one line reproduction of https://github.com/nmslib/nmslib/issues/327
        GlobalTestCase.index = nmslib.init()


if __name__ == "__main__":
    unittest.main()

