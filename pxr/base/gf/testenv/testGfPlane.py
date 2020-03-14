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
from pxr import Gf, Tf

def err( msg ):
    return "ERROR: " + msg + " failed"

class TestGfPlane(unittest.TestCase):

    def test_Constructors(self):
        self.assertIsInstance(Gf.Plane(), Gf.Plane, err( "constructor" ))
        self.assertIsInstance(Gf.Plane(Gf.Vec3d(1,1,1), 1), Gf.Plane, err( "constructor" ))
        self.assertIsInstance(Gf.Plane(Gf.Vec3d(1,1,1), Gf.Vec3d(1,1,1)), Gf.Plane, err( "constructor" ))
        self.assertIsInstance(Gf.Plane(Gf.Vec3d(0,0,1), Gf.Vec3d(0,1,0), Gf.Vec3d(1,0,0)), Gf.Plane,
            err( "constructor" ))
        self.assertIsInstance(Gf.Plane(Gf.Vec4d(3,4,0,5)), Gf.Plane, err("constructor" ))

    def test_Properties(self):
        p = Gf.Plane()
        self.assertEqual(p.Set(Gf.Vec3d(1,1,1), 1), Gf.Plane(Gf.Vec3d(1,1,1), 1), err("Set"))
        self.assertEqual(p.Set(Gf.Vec3d(1,1,1), Gf.Vec3d(1,1,1)), Gf.Plane(Gf.Vec3d(1,1,1), Gf.Vec3d(1,1,1)),
            err("Set"))
        self.assertEqual(p.Set(Gf.Vec3d(0,0,1), Gf.Vec3d(0,1,0), Gf.Vec3d(1,0,0)),
            Gf.Plane(Gf.Vec3d(0,0,1), Gf.Vec3d(0,1,0), Gf.Vec3d(1,0,0)), err("Set"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 1)
        self.assertEqual(p.normal, Gf.Vec3d(1,1,1).GetNormalized(), err("normal"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 10)
        self.assertEqual(p.distanceFromOrigin, 10, err("distanceFromOrigin"))

    def test_vec4d(self):
        p = Gf.Plane(Gf.Vec4d(3,4,0,5))
        self.assertEqual(p.normal, Gf.Vec3d(3, 4, 0) / 5, err("normal"))
        self.assertEqual(p.distanceFromOrigin, -1, err("distanceFromOrigin"))
        
        pt0 = Gf.Vec3d(2,3,1)
        pt1 = Gf.Vec3d(5,1,2)
        pt2 = Gf.Vec3d(6,0,7)

        p = Gf.Plane(pt0, pt1, pt2)
        eqn = p.GetEquation()
        
        for pt in [pt0, pt1, pt2]:
            v = eqn[0] * pt[0] + eqn[1] * pt[1] + eqn[2] * pt[2] + eqn[3]
            self.assertTrue(Gf.IsClose(v, 0, 1e-12))

    def test_Operators(self):
        p1 = Gf.Plane(Gf.Vec3d(1,1,1), 10)
        p2 = Gf.Plane(Gf.Vec3d(1,1,1), 20)
        self.assertEqual(p1, Gf.Plane(Gf.Vec3d(1,1,1), 10), err("equality"))
        self.assertTrue(not p1 == p2, err("equality"))
        self.assertTrue(not p1 != Gf.Plane(Gf.Vec3d(1,1,1), 10), err("inequality"))
        self.assertTrue(p1 != p2, err("inequality"))

    def test_Methods(self):
        p = Gf.Plane(Gf.Vec3d(1,1,1), 1)
        self.assertTrue(Gf.IsClose(p.GetDistance(Gf.Vec3d(2,2,2)), 2.4641016151377553, 0.00001), 
            err("GetDistance"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 0)
        self.assertTrue(Gf.IsClose(p.GetDistance(Gf.Vec3d(0,0,0)), 0, 0.00001), \
            err("GetDistance"))


        p = Gf.Plane(Gf.Vec3d(0,1,0), 0)
        self.assertEqual(p.Project(Gf.Vec3d(3, 3, 3)), Gf.Vec3d(3, 0, 3), err("Project"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 0)
        p.Transform(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), 30)))
        self.assertTrue(Gf.IsClose(p.normal, Gf.Vec3d(0.57735, 0.211325, 0.788675), 0.0001))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 1)
        self.assertEqual(p.normal * -1, p.Reorient(Gf.Vec3d()).normal, err("Reorient"))

        b = Gf.Range3d()
        self.assertFalse(Gf.Plane(Gf.Vec3d(1,1,1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))

        b = Gf.Range3d(Gf.Vec3d(-1,-1,-1), Gf.Vec3d(1,1,1))
        self.assertTrue(Gf.Plane(Gf.Vec3d(1,1,1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(1,1,-1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(1,-1,1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(1,-1,-1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(-1,1,1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(-1,1,-1), 1).IntersectsPositiveHalfSpace(b), 
                err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(-1,-1,1), 1).IntersectsPositiveHalfSpace(b), 
            err("IntersectsPositiveHalfSpace"))
        self.assertTrue(Gf.Plane(Gf.Vec3d(-1,-1,-1), 1).IntersectsPositiveHalfSpace(b), 
            err("IntersectsPositiveHalfSpace"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 10)
        self.assertFalse(p.IntersectsPositiveHalfSpace(Gf.Range3d(Gf.Vec3d(), Gf.Vec3d(1,1,1))), 
            err("IntersectsPositiveHalfSpace"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 1)
        self.assertTrue(p.IntersectsPositiveHalfSpace(Gf.Vec3d(1,1,1)), 
            err("IntersectsPositiveHalfSpace"))

        p = Gf.Plane(Gf.Vec3d(1,1,1), 10)
        self.assertFalse(p.IntersectsPositiveHalfSpace(Gf.Vec3d()), 
            err("IntersectsPositiveHalfSpace"))

        self.assertEqual(p, eval(repr(p)), err("repr"))

        self.assertTrue(len(str(Gf.Plane())), err("str"))

    def test_Fitting(self):
        # Collinear points should not define a plane.
        a = Gf.Vec3d(0, 0, 0)
        b = Gf.Vec3d(1, 0, 0)
        c = Gf.Vec3d(2, 0, 0)
        self.assertIsNone(Gf.FitPlaneToPoints([a, b, c]), err("collinear"))

        # Cannot fit plane to 2 or fewer points.
        with self.assertRaises(Tf.ErrorException):
            Gf.FitPlaneToPoints([a, b])

        # Perfect fit (normal should be parallel to Z-axis, but OK if opposite
        # direction).
        c = Gf.Vec3d(0, 1, 0)
        p = Gf.FitPlaneToPoints([a, b, c])
        self.assertAlmostEqual(Gf.Dot(p.GetNormal(), Gf.Vec3d.ZAxis()), 1.0,
                msg=err("normal1"))
        self.assertAlmostEqual(p.GetDistanceFromOrigin(), 0.0,
                msg=err("distance1"))

        # Try the same plane but with non-unit vectors.
        b = Gf.Vec3d(1.5, 0, 0)
        c = Gf.Vec3d(0, 3.2, 0)
        p = Gf.FitPlaneToPoints([a, b, c])
        self.assertAlmostEqual(Gf.Dot(p.GetNormal(), Gf.Vec3d.ZAxis()), 1.0,
                msg=err("normal2"))
        self.assertAlmostEqual(p.GetDistanceFromOrigin(), 0.0,
                msg=err("distance2"))

        # Try a more complicated plane.
        p1 = Gf.Plane(Gf.Vec4d(3, 4, 0, 5)) # equation constructor
        a = p1.Project(Gf.Vec3d(2, 3, 6))
        b = p1.Project(Gf.Vec3d(34, -2, 2))
        c = p1.Project(Gf.Vec3d(-3, 7, -8))
        d = p1.Project(Gf.Vec3d(4, 1, 1))
        e = p1.Project(Gf.Vec3d(87, 67, 92))
        p2 = Gf.FitPlaneToPoints([a, b, c])
        self.assertAlmostEqual(
                Gf.Dot(p1.GetNormal(), p2.GetNormal()), 1.0,
                msg=err("p2 normal parallel to p1"))
        self.assertAlmostEqual(
                p1.GetDistanceFromOrigin(),
                p2.GetDistanceFromOrigin(),
                msg=err("p2 distance equals p1"))
        p3 = Gf.FitPlaneToPoints([a, b, c, d, e])
        self.assertAlmostEqual(
                Gf.Dot(p1.GetNormal(), p3.GetNormal()), 1.0,
                msg=err("p3 normal parallel to p1"))
        self.assertAlmostEqual(
                p1.GetDistanceFromOrigin(),
                p3.GetDistanceFromOrigin(),
                msg=err("p3 distance equals p1"))

        # Try fitting points that don't form a perfect plane.
        # This roughly forms the plane with normal (1, -1, 0) passing through
        # the origin.
        # Decrease the number of places of accuracy since these points are
        # fudged and don't form an exact plane.
        a = Gf.Vec3d(1.1, 1, 5)
        b = Gf.Vec3d(1, 1.1, 2)
        c = Gf.Vec3d(2, 2.1, -4)
        d = Gf.Vec3d(2.1, 2, 1)
        e = Gf.Vec3d(25.3, 25.2, 3)
        f = Gf.Vec3d(25.1, 25.4, 6)
        p = Gf.FitPlaneToPoints([a, b, c, d, e, f])
        expectedNormal = Gf.Vec3d(1, -1, 0).GetNormalized()
        self.assertAlmostEqual(
                Gf.Dot(p.GetNormal(), expectedNormal), 1.0,
                places=2,
                msg=err("normal3"))
        self.assertAlmostEqual(
                p.GetDistanceFromOrigin(), 0.0,
                places=2,
                msg=err("distance3"))

if __name__ == '__main__':
    unittest.main()
