import itertools
import tempfile
import unittest

import numpy as np
import numpy.testing as npt

import nmslib


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


class DenseIndexTestMixin(object):
    def _get_index(self, space='cosinesimil'):
        raise NotImplementedError()

    def testKnnQuery(self):
        np.random.seed(23)
        data = np.random.randn(1000, 10).astype(np.float32)

        index = self._get_index()
        index.addDataPointBatch(data)
        index.createIndex()

        row = np.array([1, 1, 1, 1, 1, 1, 1, 1, 1, 1.])
        ids, distances = index.knnQuery(row, k=10)
        self.assertTrue(get_hitrate(get_exact_cosine(row, data), ids) >= 5)

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
        with tempfile.NamedTemporaryFile() as tmp:
            original.saveIndex(tmp.name + ".index")

            reloaded = self._get_index()
            reloaded.addDataPointBatch(data)
            reloaded.loadIndex(tmp.name + ".index")

            original_results = original.knnQuery(data[0])
            reloaded_results = reloaded.knnQuery(data[0])
            npt.assert_allclose(original_results,
                                reloaded_results)


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

        # test out saving/reloading index
        with tempfile.NamedTemporaryFile() as tmp:
            original.saveIndex(tmp.name + ".index")

            reloaded = self._get_index()
            for ids, data in batches:
                reloaded.addDataPointBatch(ids=ids, data=data)
            reloaded.loadIndex(tmp.name + ".index")

            s = self.bit_vector_str_func(np.ones(512))
            original_results = original.knnQuery(s)
            reloaded_results = reloaded.knnQuery(s)
            original_results = list(zip(list(original_results[0]), list(original_results[1])))
            original_results = sorted(original_results, key=lambda x: x[1])
            reloaded_results = list(zip(list(reloaded_results[0]), list(reloaded_results[1])))
            reloaded_results = sorted(reloaded_results, key=lambda x: x[1])
            npt.assert_allclose(original_results, reloaded_results)


class HNSWTestCase(unittest.TestCase, DenseIndexTestMixin):
    def _get_index(self, space='cosinesimil'):
        return nmslib.init(method='hnsw', space=space)


class BitJaccardTestCase(unittest.TestCase, BitVectorIndexTestMixin):
    def _get_index(self, space='bit_jaccard'):
        return nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                           dtype=nmslib.DistType.FLOAT)


class SparseJaccardTestCase(unittest.TestCase, BitVectorIndexTestMixin):
    def _get_index(self, space='jaccard_sparse'):
        return nmslib.init(method='hnsw', space=space, data_type=nmslib.DataType.OBJECT_AS_STRING,
                           dtype=nmslib.DistType.FLOAT)


class BitHammingTestCase(unittest.TestCase, BitVectorIndexTestMixin):
    def _get_index(self, space='bit_hamming'):
        return nmslib.init(method='hnsw', space='bit_hamming', data_type=nmslib.DataType.OBJECT_AS_STRING,
                           dtype=nmslib.DistType.INT)


class SWGraphTestCase(unittest.TestCase, DenseIndexTestMixin):
    def _get_index(self, space='cosinesimil'):
        return nmslib.init(method='sw-graph', space=space)

    def testReloadIndex(self):
        np.random.seed(23)
        data = np.random.randn(1000, 10).astype(np.float32)

        original = self._get_index()
        original.addDataPointBatch(data)
        original.createIndex()

        # test out saving/reloading index
        with tempfile.NamedTemporaryFile() as tmp:
            original.saveIndex(tmp.name + ".index")

            reloaded = self._get_index()
            reloaded.addDataPointBatch(data)
            reloaded.loadIndex(tmp.name + ".index")

            original_results = original.knnQuery(data[0])
            reloaded_results = reloaded.knnQuery(data[0])
            npt.assert_allclose(original_results,
                                reloaded_results)


class BallTreeTestCase(unittest.TestCase, DenseIndexTestMixin):
    def _get_index(self, space='cosinesimil'):
        return nmslib.init(method='vptree', space=space)

    def testReloadIndex(self):
        return NotImplemented


class StringTestCase(unittest.TestCase):
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
            self.assertEqual(index.getDistance(0, i), distance)

        self.assertEqual(len(index), len(strings) + 2)
        self.assertEqual(index[0], strings[0])
        self.assertEqual(index[len(index)-2], 'atat')


class SparseTestCase(unittest.TestCase):
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
        self.assertEqual(distances[0], 0)

        self.assertEqual(len(index), 4)
        self.assertEqual(index[3], [(3, 1.0)])


class GlobalTestCase(unittest.TestCase):
    def testGlobal(self):
        # this is a one line reproduction of https://github.com/nmslib/nmslib/issues/327
        GlobalTestCase.index = nmslib.init()


if __name__ == "__main__":
    unittest.main()
