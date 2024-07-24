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

def err( msg ):
    return "ERROR: " + msg + " failed"

class TestGfLine(unittest.TestCase):

    def test_Constructors(self):
        self.assertIsInstance(Gf.Line(), Gf.Line)
        self.assertIsInstance(Gf.Line(Gf.Vec3d(), Gf.Vec3d()), Gf.Line)
    
    def test_Properties(self):
        l = Gf.Line()
        l.Set(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4))
        self.assertTrue(l.GetPoint(0) == Gf.Vec3d(1,2,3) and \
            l.direction == Gf.Vec3d(2,3,4).GetNormalized(), err("Set"))

        l.Set(Gf.Vec3d(0,0,0), Gf.Vec3d(0,1,0))
        self.assertTrue(l.GetPoint(0.5) == Gf.Vec3d(0, 0.5, 0) and \
            l.GetPoint(1.0) == Gf.Vec3d(0, 1, 0), err("GetPoint"))

        l.Set(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4))
        l.direction = Gf.Vec3d(1, 1, 1)
        self.assertEqual(l.direction, Gf.Vec3d(1, 1, 1).GetNormalized(), err("direction"))

    def test_Methods(self):
        l = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        (point, t) = l.FindClosestPoint(Gf.Vec3d(0.5, 0.5, 1))
        self.assertTrue(Gf.IsClose(point, Gf.Vec3d(2./3, 2./3, 2./3), 0.00001), err("FindClosestPoint"))
        self.assertTrue(Gf.IsClose(t, 1.1547, 0.0001), err("FindClosestPoint"))

        # (parallel case)
        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.Line(Gf.Vec3d(1, 0, 0), Gf.Vec3d(1, 1, 1))
        self.assertEqual(Gf.FindClosestPoints(l1, l2)[0], False, err("FindClosestPoints"))

        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.Line(Gf.Vec3d(1, 0, 0), Gf.Vec3d(1, -1, 1))
        (intersects, p1, p2, t1, t2) = Gf.FindClosestPoints(l1, l2)
        self.assertTrue(intersects, err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p1, Gf.Vec3d(0.25, 0.25, 0.25), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p2, Gf.Vec3d(0.75, 0.25, -0.25), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(t1, 0.433012701892, 0.00001))
        self.assertTrue(Gf.IsClose(t2, -0.433012701892, 0.00001))
    

    def test_Operators(self):
        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        self.assertEqual(l1, l2, err("equality"))

        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 2, 3))
        l2 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        self.assertNotEqual(l1, 2, err("inequality"))

        self.assertEqual(l1, eval(repr(l1)), err("repr"))
        self.assertTrue(len(str(Gf.Line())), err("str"))
    
if __name__ == '__main__':
    unittest.main()
