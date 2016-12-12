#!/usr/bin/python
# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4

import unittest
import numpy.testing as nt
import sys
import os
import time
import numpy as np
from scipy.sparse import csr_matrix
import nmslib_vector


class DenseTests(unittest.TestCase):
    def setUp(self):
        space_type = 'cosinesimil'
        space_param = []
        method_name = 'small_world_rand'
        index_name  = method_name + '.index'
        if os.path.isfile(index_name):
            os.remove(index_name)
        self.index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.DENSE_VECTOR,
                             nmslib_vector.DistType.FLOAT)

    def test_add_points(self):
        self.assertEqual(0, nmslib_vector.addDataPoint(self.index, 1000, [0.5, 0.3, 0.4]))
        self.assertEqual(1, nmslib_vector.addDataPoint(self.index, 1001, [0.5, 0.3, 0.4]))

    def test_add_points_batch1(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          [0,1,2],
                          [[0.34,0.54], [0.55,0.52], [0.21,0.68]])

    def test_add_points_batch2(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          np.array([0,1,2]),
                          [[0.34,0.54], [0.55,0.52], [0.21,0.68]])

    def test_add_points_batch3(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          np.array([0,1,2], dtype=np.int32),
                          [[0.34,0.54], [0.55,0.52], [0.21,0.68]])

    def test_add_points_batch4(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          np.array([0,1,2], dtype=np.int32),
                          np.array([[0.34,0.54], [0.55,0.52], [0.21,0.68]]))

    def test_add_points_batch5(self):
        positions = nmslib_vector.addDataPointBatch(self.index,
                          np.array([0,1,2], dtype=np.int32),
                          np.array([[0.34,0.54], [0.55,0.52], [0.21,0.68]], dtype=np.float32))
        nt.assert_array_equal(np.array([0,1,2], dtype=np.int32), np.sort(positions))


class SparseTests(unittest.TestCase):
    def setUp(self):
        space_type = 'cosinesimil_sparse'
        space_param = []
        method_name = 'small_world_rand'
        index_name  = method_name + '.index'
        if os.path.isfile(index_name):
            os.remove(index_name)
        self.index = nmslib_vector.init(
                             space_type,
                             space_param,
                             method_name,
                             nmslib_vector.DataType.SPARSE_VECTOR,
                             nmslib_vector.DistType.FLOAT)

    def test_add_points(self):
        self.assertEqual(0, nmslib_vector.addDataPoint(self.index, 1000, [[0, 0.5], [5, 0.3], [6, 0.4]]))
        self.assertEqual(1, nmslib_vector.addDataPoint(self.index, 1001, [[0, 0.5], [3, 0.3], [5, 0.4]]))

    def test_add_points_batch1(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          [0,1,2],
                          [[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]])

    def test_add_points_batch2(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          np.array([0,1,2]),
                          [[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]])

    def test_add_points_batch3(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          np.array([0,1,2], dtype=np.int32),
                          [[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]])

    def test_add_points_batch4(self):
        self.assertRaises(ValueError, nmslib_vector.addDataPointBatch, self.index,
                          np.array([0,1,2], dtype=np.int32),
                          np.array([[0.1, 0, 0.2], [0, 0, 0.3], [0.4, 0.5, 0.6]], dtype=np.float32))

    def test_add_points_batch5(self):
        row = np.array([0, 0, 1, 2, 2])
        col = np.array([0, 2, 1, 1, 2])
        data = np.array([0.3, 0.2, 0.4, 0.1, 0.6])
        m = csr_matrix((data, (row, col)), dtype=np.float32, shape=(3, 3))
        print m.toarray()
        positions = nmslib_vector.addDataPointBatch(self.index,
                          np.array([0,1,2], dtype=np.int32),
                          m)
        nt.assert_array_equal(np.array([0,1,2], dtype=np.int32), np.sort(positions))


if __name__ == '__main__':
    unittest.main()


