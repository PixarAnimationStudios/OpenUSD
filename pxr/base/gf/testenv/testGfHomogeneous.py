#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import division

import sys
import unittest
import math
from pxr import Gf

class TestGfHomogeneous(unittest.TestCase):

    def test_GetHomogenized(self):
        self.assertEqual(Gf.GetHomogenized(Gf.Vec4f(2, 4, 6, 2)), Gf.Vec4f(1, 2, 3, 1))
        self.assertEqual(Gf.GetHomogenized(Gf.Vec4f(1, 2, 3, 0)), Gf.Vec4f(1, 2, 3, 1))

        self.assertEqual(Gf.GetHomogenized(Gf.Vec4d(2, 4, 6, 2)), Gf.Vec4d(1, 2, 3, 1))
        self.assertEqual(Gf.GetHomogenized(Gf.Vec4d(1, 2, 3, 0)), Gf.Vec4d(1, 2, 3, 1))

    def test_GetHomogenizedCross(self):
        v1 = Gf.Vec4f(3, 1, 4, 1)
        v2 = Gf.Vec4f(5, 9, 2, 6)
        v3 = Gf.Vec3f(3, 1, 4)
        v4 = Gf.Vec3f(5./6, 9./6, 2./6)
        r = Gf.HomogeneousCross(v1, v2)
        result = Gf.Vec3f( r[0], r[1], r[2] )
        self.assertTrue(Gf.IsClose(result, v3 ^ v4, 0.00001))

        v1 = Gf.Vec4d(3, 1, 4, 1)
        v2 = Gf.Vec4d(5, 9, 2, 6)
        v3 = Gf.Vec3d(3, 1, 4)
        v4 = Gf.Vec3d(5./6, 9./6, 2./6)
        r = Gf.HomogeneousCross(v1, v2)
        result = Gf.Vec3d( r[0], r[1], r[2] )
        self.assertTrue(Gf.IsClose(result, v3 ^ v4, 0.00001))

    def test_Project(self):
        self.assertEqual(Gf.Project(Gf.Vec4d(2, 4, 6, 2)), Gf.Vec3d(1, 2, 3))
        self.assertEqual(Gf.Project(Gf.Vec4d(2, 4, 6, 0)), Gf.Vec3d(2, 4, 6))
        self.assertEqual(Gf.Project(Gf.Vec4f(2, 4, 6, 2)), Gf.Vec3f(1, 2, 3))
        self.assertEqual(Gf.Project(Gf.Vec4f(2, 4, 6, 0)), Gf.Vec3f(2, 4, 6))

if __name__ == '__main__':
    unittest.main()
