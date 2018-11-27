#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#

import sys
import unittest
import math
from pxr import Gf

def err( msg ):
    return "ERROR: " + msg + " failed"

class TestGfLineSeg(unittest.TestCase):
    
    def test_Constructors(self):
        self.assertIsInstance(Gf.LineSeg(), Gf.LineSeg, err("constructor"))
        self.assertIsInstance(Gf.LineSeg(Gf.Vec3d(), Gf.Vec3d()), Gf.LineSeg, err("constructor"))

    def test_Properties(self):
        l = Gf.LineSeg(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4))
        self.assertTrue(l.GetPoint(0) == Gf.Vec3d(1,2,3) and \
            l.direction == (Gf.Vec3d(2,3,4) - Gf.Vec3d(1,2,3)).GetNormalized() and \
            l.length == Gf.GetLength(Gf.Vec3d(2,3,4)-Gf.Vec3d(1,2,3)),
            err("initializing constructor"))
        
        l = Gf.LineSeg(Gf.Vec3d(0,0,0), Gf.Vec3d(0,1,0))
        self.assertTrue(l.GetPoint(0.5) == Gf.Vec3d(0, 0.5, 0) and \
            l.GetPoint(1.0) == Gf.Vec3d(0, 1, 0), err("GetPoint"))
        
        l = Gf.LineSeg(Gf.Vec3d(1,2,3), Gf.Vec3d(1,1,1))
        self.assertEqual(l.direction, (Gf.Vec3d(1,1,1) - Gf.Vec3d(1,2,3)).GetNormalized(), err("direction"))
        
        l = Gf.LineSeg(Gf.Vec3d(0,0,0), Gf.Vec3d(0,3,4))
        self.assertEqual(l.length, 5, err("length"))
        
    def test_Methods(self):
        l = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        (point, t) = l.FindClosestPoint(Gf.Vec3d(0.5, 0.5, 1))
        self.assertTrue(Gf.IsClose(point, Gf.Vec3d(2./3, 2./3, 2./3), 0.00001), err("FindClosestPoint"))
        self.assertTrue(Gf.IsClose(t, 2./3, 0.0001), err("FindClosestPoint"))

        l = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(0, 0, 0))
        (point, t) = l.FindClosestPoint(Gf.Vec3d(0.5, 0.5, 1))
        self.assertTrue(Gf.IsClose(point, Gf.Vec3d(), 0.00001), err("FindClosestPoint"))
        self.assertTrue(Gf.IsClose(t, 0, 0.0001), err("FindClosestPoint"))

        # seg on seg
        # (parallel case)
        l1 = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.LineSeg(Gf.Vec3d(1, 0, 0), Gf.Vec3d(2, 1, 1))
        self.assertEqual(Gf.FindClosestPoints(l1, l2)[0], False, err("FindClosestPoints"))

        l1 = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.LineSeg(Gf.Vec3d(1, 0, 0), Gf.Vec3d(1, -1, 1))
        (intersects, p1, p2, t1, t2) = Gf.FindClosestPoints(l1, l2)
        self.assertTrue(intersects, err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p1, Gf.Vec3d(1./3, 1./3, 1./3), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p2, Gf.Vec3d(1, 0, 0), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(t1, 1./3, 0.00001))
        self.assertTrue(Gf.IsClose(t2, 0, 0.00001))

        l1 = Gf.LineSeg(Gf.Vec3d(), Gf.Vec3d())
        l2 = Gf.LineSeg(Gf.Vec3d(1,1,1), Gf.Vec3d(1,1,1))
        (intersects, p1, p2, t1, t2) = Gf.FindClosestPoints(l1, l2)
        self.assertFalse(intersects, err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p1, Gf.Vec3d(), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p2, Gf.Vec3d(), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(t1, 0, 0.00001))
        self.assertTrue(Gf.IsClose(t2, 0, 0.00001))
        (intersects, p1, p2, t1, t2) = Gf.FindClosestPoints(l2, l1)
        self.assertFalse(intersects, err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p1, Gf.Vec3d(), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p2, Gf.Vec3d(), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(t1, 0, 0.00001))
        self.assertTrue(Gf.IsClose(t2, 0, 0.00001))

        # line on seg
        # (parallel case)
        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.LineSeg(Gf.Vec3d(1, 0, 0), Gf.Vec3d(2, 1, 1))
        self.assertEqual(Gf.FindClosestPoints(l1, l2)[0], False, err("FindClosestPoints"))

        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.LineSeg(Gf.Vec3d(1, 0, 0), Gf.Vec3d(1, -1, 1))
        (intersects, p1, p2, t1, t2) = Gf.FindClosestPoints(l1, l2)
        self.assertTrue(intersects, err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p1, Gf.Vec3d(1./3, 1./3, 1./3), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p2, Gf.Vec3d(1, 0, 0), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(t1, 0.57735, 0.00001))
        self.assertTrue(Gf.IsClose(t2, 0, 0.00001))

        l1 = Gf.Line(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.LineSeg(Gf.Vec3d(), Gf.Vec3d())
        (intersects, p1, p2, t1, t2) = Gf.FindClosestPoints(l1, l2)
        self.assertFalse(intersects, err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p1, Gf.Vec3d(), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(p2, Gf.Vec3d(), 0.00001), err("FindClosestPoints"))
        self.assertTrue(Gf.IsClose(t1, 0, 0.00001))
        self.assertTrue(Gf.IsClose(t2, 0, 0.00001))
        
    def test_Operators(self):
        l1 = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        l2 = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        self.assertEqual(l1, l2, err("equality"))

        l1 = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 2, 3))
        l2 = Gf.LineSeg(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 1, 1))
        self.assertNotEqual(l1, 2, err("inequality"))

        self.assertEqual(l1, eval(repr(l1)), err("repr"))

        self.assertTrue(len(str(Gf.LineSeg())), err("str"))

if __name__ == '__main__':
    unittest.main()
