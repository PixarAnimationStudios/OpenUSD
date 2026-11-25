#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import array
import sys
import unittest

from pxr import Gf

def _Grid(typecode, values, shape):
    """Return an array.array of values and a C-contiguous memoryview of it
    with the given shape."""
    storage = array.array(typecode, values)
    return storage, memoryview(storage).cast('B').cast(typecode, shape)

class TestGfPySpan(unittest.TestCase):

    def test_GfStandsAlone(self):
        # Gf's span conversions must not depend on other libraries, so that
        # behavior is the same whether or not they are loaded.
        self.assertNotIn('pxr.Vt', sys.modules)

    def test_Sequences(self):
        vecs = [Gf.Vec3f(1, 2, 3), Gf.Vec3f(4, 5, 6)]
        self.assertEqual(Gf._test_Vec3fSpanElements(vecs), vecs)
        self.assertEqual(Gf._test_Vec3fSpanElements([]), [])
        with self.assertRaises(TypeError):
            Gf._test_Vec3fSpanElements([Gf.Vec3f(), 'x'])

    def test_Vec3fBuffers(self):
        storage, grid = _Grid('f', range(6), [2, 3])
        self.assertEqual(Gf._test_Vec3fSpanAddress(grid),
                         storage.buffer_info()[0])
        self.assertEqual(Gf._test_Vec3fSpanElements(grid),
                         [Gf.Vec3f(0, 1, 2), Gf.Vec3f(3, 4, 5)])

        # Writes go through to the buffer.
        Gf._test_FillVec3fSpan(grid, Gf.Vec3f(7))
        self.assertEqual(list(storage), [7.0] * 6)

    def test_BufferMismatches(self):
        # Buffers whose layout does not match are not viewed.  These then
        # fail to convert at all, since multi-dimensional memoryviews do not
        # support item access for the sequence fallback.
        _, grid = _Grid('f', range(8), [2, 4])
        with self.assertRaises(TypeError):
            Gf._test_Vec3fSpanElements(grid)
        _, grid = _Grid('d', range(6), [2, 3])
        with self.assertRaises(TypeError):
            Gf._test_Vec3fSpanElements(grid)

        # A flat buffer has no element dimension, and its items are floats,
        # not Vec3fs.
        with self.assertRaises(TypeError):
            Gf._test_Vec3fSpanElements(array.array('f', range(6)))

    def test_MatrixBuffers(self):
        storage, grid = _Grid('f', range(18), [2, 3, 3])
        self.assertEqual(Gf._test_Matrix3fSpanAddress(grid),
                         storage.buffer_info()[0])
        self.assertEqual(Gf._test_Matrix3fSpanElements(grid),
                         [Gf.Matrix3f(*range(9)), Gf.Matrix3f(*range(9, 18))])

    def test_RangeBuffers(self):
        _, grid = _Grid('d', [0, 1, 2, 3], [2, 2])
        self.assertEqual(Gf._test_Range1dSpanElements(grid),
                         [Gf.Range1d(0, 1), Gf.Range1d(2, 3)])

    def test_GfTypeBuffers(self):
        # Gf objects export buffers too, and convert according to their
        # buffer's layout.  A Vec3f is a buffer of shape (3,), which is not a
        # span of Vec3f.
        with self.assertRaises(TypeError):
            Gf._test_Vec3fSpanElements(Gf.Vec3f(1, 2, 3))

        # A Matrix3f is a buffer of shape (3, 3), which is a span of 3 Vec3f
        # rows.
        m = Gf.Matrix3f(*range(9))
        self.assertEqual(Gf._test_Vec3fSpanElements(m),
                         [Gf.Vec3f(0, 1, 2), Gf.Vec3f(3, 4, 5),
                          Gf.Vec3f(6, 7, 8)])

    def test_HalfSpans(self):
        # memoryview cannot create half buffers, but a Vec3h exports one.
        # It is a 1-d buffer of 3 halfs, which is a span of 3 GfHalf.
        self.assertEqual(Gf._test_HalfSpanElements(Gf.Vec3h(1, 2, 3)),
                         [1.0, 2.0, 3.0])
        self.assertEqual(Gf._test_HalfSpanElements([0.5, 1.5]), [0.5, 1.5])

if __name__ == '__main__':
    unittest.main()
