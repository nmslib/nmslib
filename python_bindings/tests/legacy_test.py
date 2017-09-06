#!/usr/bin/python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import unittest
import numpy.testing as nt
import os
import numpy as np
try:
    from scipy.sparse import csr_matrix
    has_scipy = True
except ImportError:
    has_scipy = False

import nmslib


class DenseTests(unittest.TestCase):

    def setUp(self):
        space_type = 'cosinesimil'
        space_param = []
        method_name = 'small_world_rand'
        index_name = method_name + '.index'
        if os.path.isfile(index_name):
            os.remove(index_name)
        self.index = nmslib.init(
            space_type,
            space_param,
            method_name,
            nmslib.DataType.DENSE_VECTOR,
            nmslib.DistType.FLOAT)

    def test_add_points(self):
        self.assertEqual(0, nmslib.addDataPoint(self.index, 1000, [0.5, 0.3, 0.4]))
        self.assertEqual(1, nmslib.addDataPoint(self.index, 1001, [0.5, 0.3, 0.4]))

    def test_add_points_batch1(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          [0, 1, 2],
                          [[0.34, 0.54], [0.55, 0.52], [0.21, 0.68]])

    @unittest.skip("temporarily disable")
    def test_add_points_batch2(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2]),
                          [[0.34, 0.54], [0.55, 0.52], [0.21, 0.68]])

    def test_add_points_batch3(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2], dtype=np.int32),
                          [[0.34, 0.54], [0.55, 0.52], [0.21, 0.68]])

    def test_add_points_batch4(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2], dtype=np.int32),
                          np.array([[0.34, 0.54], [0.55, 0.52], [0.21, 0.68]]))

    def test_add_points_batch5(self):
        data = np.array([[0.34, 0.54], [0.55, 0.52], [0.21, 0.68]], dtype=np.float32)
        positions = nmslib.addDataPointBatch(self.index,
                                             np.array([0, 1, 2], dtype=np.int32),
                                             data)
        nt.assert_array_equal(np.array([0, 1, 2], dtype=np.int32), positions)


class SparseTests(unittest.TestCase):

    def setUp(self):
        space_type = 'cosinesimil_sparse'
        space_param = []
        method_name = 'small_world_rand'
        index_name = method_name + '.index'
        if os.path.isfile(index_name):
            os.remove(index_name)
        self.index = nmslib.init(
            space_type,
            space_param,
            method_name,
            nmslib.DataType.SPARSE_VECTOR,
            nmslib.DistType.FLOAT)

    def test_add_points(self):
        self.assertEqual(0, nmslib.addDataPoint(self.index, 1000, [[0, 0.5], [5, 0.3], [6, 0.4]]))
        self.assertEqual(1, nmslib.addDataPoint(self.index, 1001, [[0, 0.5], [3, 0.3], [5, 0.4]]))

    def test_add_points_batch1(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          [0, 1, 2],
                          [[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]])

    @unittest.skip("temporarily disable")
    def test_add_points_batch2(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2]),
                          [[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]])

    def test_add_points_batch3(self):
        self.assertRaises(TypeError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2], dtype=np.int32),
                          [[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]])

    def test_add_points_batch4(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2], dtype=np.int32),
                          np.array([[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]], dtype=np.float32))

    def test_add_points_batch5(self):
        if not has_scipy:
            return
        row = np.array([0, 0, 1, 2, 2])
        col = np.array([0, 2, 1, 1, 2])
        data = np.array([0.3, 0.2, 0.4, 0.1, 0.6])
        m = csr_matrix((data, (row, col)), dtype=np.float32, shape=(3, 3))
        # print m.toarray()
        positions = nmslib.addDataPointBatch(self.index,
                                             np.array([0, 1, 2], dtype=np.int32),
                                             m)
        nt.assert_array_equal(np.array([0, 1, 2], dtype=np.int32), positions)


class StringTests1(unittest.TestCase):

    def setUp(self):
        space_type = 'leven'
        space_param = []
        method_name = 'small_world_rand'
        index_name = method_name + '.index'
        if os.path.isfile(index_name):
            os.remove(index_name)
        self.index = nmslib.init(
            space_type,
            space_param,
            method_name,
            nmslib.DataType.OBJECT_AS_STRING,
            nmslib.DistType.INT)

    def test_add_points(self):
        self.assertEqual(0, nmslib.addDataPoint(self.index, 1000, "string1"))
        self.assertEqual(1, nmslib.addDataPoint(self.index, 1001, "string2"))

    def test_add_points_batch1(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          [0, 1, 2],
                          ["string1", "string2", "string3"])

    @unittest.skip("temporarily disable")
    def test_add_points_batch2(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2]),
                          ["string1", "string2", "string3"])

    def test_add_points_batch5(self):
        positions = nmslib.addDataPointBatch(self.index,
                                             np.array([0, 1, 2], dtype=np.int32),
                                             ["string1", "string2", "string3"])
        nt.assert_array_equal(np.array([0, 1, 2], dtype=np.int32), positions)


class StringTests2(unittest.TestCase):

    def setUp(self):
        space_type = 'normleven'
        space_param = []
        method_name = 'small_world_rand'
        index_name = method_name + '.index'
        if os.path.isfile(index_name):
            os.remove(index_name)
        self.index = nmslib.init(
            space_type,
            space_param,
            method_name,
            nmslib.DataType.OBJECT_AS_STRING,
            nmslib.DistType.FLOAT)

    def test_add_points(self):
        self.assertEqual(0, nmslib.addDataPoint(self.index, 1000, "string1"))
        self.assertEqual(1, nmslib.addDataPoint(self.index, 1001, "string2"))

    def test_add_points_batch1(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          [0, 1, 2],
                          ["string1", "string2", "string3"])

    @unittest.skip("temporarily disable")
    def test_add_points_batch2(self):
        self.assertRaises(ValueError, nmslib.addDataPointBatch, self.index,
                          np.array([0, 1, 2]),
                          ["string1", "string2", "string3"])

    def test_add_points_batch5(self):
        positions = nmslib.addDataPointBatch(self.index,
                                             np.array([0, 1, 2], dtype=np.int32),
                                             ["string1", "string2", "string3"])
        nt.assert_array_equal(np.array([0, 1, 2], dtype=np.int32), positions)


if __name__ == '__main__':
    unittest.main()
