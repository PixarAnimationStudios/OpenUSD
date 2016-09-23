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

class TestGfFrustum(unittest.TestCase):

    def test_Constructors(self):
        self.assertIsInstance(Gf.Frustum(), Gf.Frustum)
        self.assertIsInstance(Gf.Frustum(Gf.Frustum()), Gf.Frustum)

        # code coverage wonderfulness.
        f = Gf.Frustum()
        # force instantiation of the frustum planes
        f.Intersects(Gf.Vec3d())
        f2 = Gf.Frustum(f)

    def test_Operators(self):
        f1 = Gf.Frustum()
        f2 = Gf.Frustum(f1)
        self.assertEqual(f1, f2)

    def test_PlaneIntersection(self):
        f1 = Gf.Frustum()
        f2 = Gf.Frustum()
        # force plane instantiation.
        f1.Intersects(Gf.Vec3d())
        f2.Intersects(Gf.Vec3d())
        self.assertEqual(f1, f2)

    def test_Position(self):
        f1 = Gf.Frustum()
        f2 = Gf.Frustum()
        f1.position = Gf.Vec3d(1, 0, 0)
        f2.position = Gf.Vec3d(0, 1, 0)
        self.assertNotEqual(f1, f2)

    def test_Properties(self):
        f = Gf.Frustum()
        f.position = Gf.Vec3d(1, 2, 3)
        self.assertEqual(f.position, Gf.Vec3d(1, 2, 3))

        f.rotation = Gf.Rotation(Gf.Vec3d(1, 1, 1), 30)
        self.assertEqual(f.rotation, Gf.Rotation(Gf.Vec3d(1, 1, 1), 30))

        f.window = Gf.Range2d( Gf.Vec2d(0, 1), Gf.Vec2d(2, 3))
        self.assertEqual(f.window, Gf.Range2d( Gf.Vec2d(0, 1), Gf.Vec2d(2, 3)))

        f.nearFar = Gf.Range1d(1, 2)
        self.assertEqual(f.nearFar, Gf.Range1d(1, 2))

        f.viewDistance = 10
        self.assertEqual(f.viewDistance, 10)

        f.projectionType = Gf.Frustum.Orthographic
        self.assertEqual(f.projectionType, Gf.Frustum.Orthographic)
        f.projectionType = Gf.Frustum.Perspective
        self.assertEqual(f.projectionType, Gf.Frustum.Perspective)

    def test_Projection(self):
        f = Gf.Frustum()
        f.SetPerspective( 10, True, 20, 30, 40 )
        self.assertEqual(f.GetPerspective(True), (10, 20, 30, 40))
        self.assertEqual(f.GetFOV(True), 10)
        f.SetPerspective( 10, False, 20, 30, 40 )
        self.assertEqual(f.GetPerspective(False), (10, 20, 30, 40))
        self.assertEqual(f.GetFOV(False), 10)
        self.assertEqual(f.GetFOV(), 10)
        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        self.assertIsNone(f.GetPerspective(True))
        self.assertEqual(f.GetFOV(True), 0.0)
        self.assertEqual(f.GetFOV(False), 0.0)

        self.assertEqual(Gf.Frustum.GetReferencePlaneDepth(), 1.0)

        f.SetOrthographic( 10, 20, 30, 40, 50, 60 )
        self.assertEqual(f.GetOrthographic(), (10, 20, 30, 40, 50, 60))
        self.assertEqual(f.GetFOV(), 0.0)
        f = Gf.Frustum()
        f.projectionType = f.Perspective
        self.assertEqual(len(f.GetOrthographic()), 0)

    def test_Intersection(self):
        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        f.FitToSphere( Gf.Vec3d(0, 0, 0), 1 )
        self.assertTrue(f.Intersects( Gf.Vec3d(0, 0, 0) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(0.9, 0, 0) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(0, 0.9, 0) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(0, 0, 0.9) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(-0.9, 0, 0) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(0, -0.9, 0) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(0, 0, -0.9) ))
        f = Gf.Frustum()
        f.projectionType = f.Perspective
        f.FitToSphere( Gf.Vec3d(1, 1, 1), 1, 0.2 )    
        self.assertTrue(f.Intersects( Gf.Vec3d(1, 1, 1) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(2.1, 1, 1) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(1, 2.1, 1) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(1, 1, 2.1) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(-0.1, 1, 1) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(1, -0.1, 1) ))
        self.assertTrue(f.Intersects( Gf.Vec3d(1, 1, -0.1) ))

    def test_CodeCoverage(self):
        f = Gf.Frustum()
        f.projectionType = f.Perspective
        f.window = Gf.Range2d(Gf.Vec2d(10,20), Gf.Vec2d(30,40))
        f.FitToSphere(Gf.Vec3d(), 1)
        f.window = Gf.Range2d(Gf.Vec2d(30,40), Gf.Vec2d(10,20))
        f.FitToSphere(Gf.Vec3d(), 1)
        f.window = Gf.Range2d(Gf.Vec2d(), Gf.Vec2d())
        f.FitToSphere(Gf.Vec3d(), 1)
        f = Gf.Frustum()
        f.projectionType = f.Perspective
        f.window = Gf.Range2d(Gf.Vec2d(-30,-40), Gf.Vec2d(10,20))
        f.FitToSphere(Gf.Vec3d(), 1)

    def test_Transform(self):
        f = Gf.Frustum()
        f.Transform( Gf.Matrix4d(2) )
        self.assertEqual(f.nearFar, 2 * Gf.Frustum().nearFar)
        f.Transform( Gf.Matrix4d(-2) )
        f.window = Gf.Range2d(Gf.Vec2d(1,1), Gf.Vec2d(-1,-1))
        f.Transform(Gf.Matrix4d(1))

    def test_SetRotate(self):
        f = Gf.Frustum()
        self.assertEqual(f.ComputeViewDirection(), Gf.Vec3d(0,0,-1))
        m = Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1, 1, 1), 30))
        self.assertTrue(Gf.IsClose(Gf.Frustum().Transform(m).ComputeViewDirection(), \
            Gf.Vec3d(-0.333333, 0.244017, -0.910684), 0.0001))

        f = Gf.Frustum()
        self.assertEqual(f.ComputeUpVector(), Gf.Vec3d(0,1,0))
        m = Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1, 1, 1), 30))
        self.assertTrue(Gf.IsClose(Gf.Frustum().Transform(m).ComputeUpVector(), \
            Gf.Vec3d(-0.244017, 0.910684, 0.333333), 0.0001))

    def test_ComputeViewFrame(self):
        f = Gf.Frustum()
        (side, up, view) = f.ComputeViewFrame()
        self.assertEqual(side, Gf.Vec3d(1, 0, 0))
        self.assertEqual(up, Gf.Vec3d(0, 1, 0))
        self.assertEqual(view, Gf.Vec3d(0, 0, -1))

    def test_ComputeLookAtPoint(self):
        f = Gf.Frustum()
        self.assertEqual(f.ComputeLookAtPoint(), Gf.Vec3d(0, 0, -5))
        f.projectionType = f.Orthographic
        self.assertEqual(f.ComputeLookAtPoint(), Gf.Vec3d(0, 0, -5))

    def test_ComputeViewInverse(self):
        f = Gf.Frustum()
        f.Transform(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1),60))) 
        r1 = f.ComputeViewMatrix() * Gf.Vec4d(1, 2, 3, 4)
        r2 = Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1),60)).GetInverse() * Gf.Vec4d(1, 2, 3, 4)
        self.assertTrue(Gf.IsClose(r1, r2, 0.0001))
        r1 = f.ComputeViewInverse() * Gf.Vec4d(1, 2, 3, 4)
        r2 = Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,1,1),60)) * Gf.Vec4d(1, 2, 3, 4)
        self.assertTrue(Gf.IsClose(r1, r2, 0.0001))

    def test_ComputeProjectionMatrix(self):
        # FIXME how to test ComputeProjectionMatrix?
        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        f.ComputeProjectionMatrix()
        f.projectionType = f.Perspective
        f.ComputeProjectionMatrix()

    def test_ComputeAspectRatio(self):
        f = Gf.Frustum().Transform(Gf.Matrix4d(Gf.Vec4d(3,2,1,1)))
        self.assertEqual(f.ComputeAspectRatio(), 1.5)
        corners = f.ComputeCorners()
        self.assertEqual(corners[0], Gf.Vec3d(-3, -2, -1))
        self.assertEqual(corners[1], Gf.Vec3d(3, -2, -1))
        self.assertEqual(corners[2], Gf.Vec3d(-3, 2, -1))
        self.assertEqual(corners[3], Gf.Vec3d(3, 2, -1))
        self.assertEqual(corners[4], Gf.Vec3d(-30, -20, -10))
        self.assertEqual(corners[5], Gf.Vec3d(30, -20, -10))
        self.assertEqual(corners[6], Gf.Vec3d(-30, 20, -10))
        self.assertEqual(corners[7], Gf.Vec3d(30, 20, -10))

    def test_ComputeCorners(self):
        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        f.Transform(Gf.Matrix4d(Gf.Vec4d(3,2,1,1)))
        corners = f.ComputeCorners()
        self.assertEqual(corners[0], Gf.Vec3d(-3, -2, -1))
        self.assertEqual(corners[1], Gf.Vec3d(3, -2, -1))
        self.assertEqual(corners[2], Gf.Vec3d(-3, 2, -1))
        self.assertEqual(corners[3], Gf.Vec3d(3, 2, -1))
        self.assertEqual(corners[4], Gf.Vec3d(-3, -2, -10))
        self.assertEqual(corners[5], Gf.Vec3d(3, -2, -10))
        self.assertEqual(corners[6], Gf.Vec3d(-3, 2, -10))
        self.assertEqual(corners[7], Gf.Vec3d(3, 2, -10))

    def test_ComputeNarrowedFrustum(self):
        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        f.Transform(Gf.Matrix4d(Gf.Vec4d(3,2,1,1)))
        narrowF = f.ComputeNarrowedFrustum(Gf.Vec2d(0, 0), Gf.Vec2d(0.1, 0.1))
        self.assertTrue(Gf.IsClose(narrowF.window.min, Gf.Vec2d(-0.3, -0.2), 0.0001))
        self.assertTrue(Gf.IsClose(narrowF.window.max, Gf.Vec2d(0.3, 0.2), 0.0001))

        narrowF = f.ComputeNarrowedFrustum(Gf.Vec3d(0, 0, -1), Gf.Vec2d(0.1, 0.1))
        self.assertTrue(Gf.IsClose(narrowF.window.min, Gf.Vec2d(-0.3, -0.2), 0.0001))
        self.assertTrue(Gf.IsClose(narrowF.window.max, Gf.Vec2d(0.3, 0.2), 0.0001))

        # Given a point behind the eye should get the same frustum back
        narrowF = f.ComputeNarrowedFrustum(Gf.Vec3d(0, 0, 1), Gf.Vec2d(0.1, 0.1))
        self.assertTrue(Gf.IsClose(narrowF.window.min, Gf.Vec2d(-3.0,-2.0), 0.0001))
        self.assertTrue(Gf.IsClose(narrowF.window.max, Gf.Vec2d(3.0,2.0), 0.0001))

    def test_ComputePickRay(self):
        f = Gf.Frustum()
        f.window = Gf.Range2d(Gf.Vec2d(3,3),Gf.Vec2d(4,4))
        f.ComputeNarrowedFrustum(Gf.Vec2d(3,3), Gf.Vec2d(100,100))

        r = Gf.Frustum().ComputePickRay(Gf.Vec2d(2, 2))
        self.assertTrue(Gf.IsClose( r.startPoint, Gf.Vec3d(2./3, 2./3, -1./3), 0.00001 ))
        self.assertTrue(Gf.IsClose( r.direction, Gf.Vec3d(2./3, 2./3, -1./3), 0.00001 ))

        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        r = f.ComputePickRay(Gf.Vec2d(2, 2))
        self.assertTrue(Gf.IsClose( r.startPoint, Gf.Vec3d(2, 2, -1), 0.00001 ))
        self.assertTrue(Gf.IsClose( r.direction, Gf.Vec3d(0, 0, -1), 0.00001 ))

        r = Gf.Frustum().ComputePickRay(Gf.Vec3d(0, 0, -2))
        self.assertTrue(Gf.IsClose( r.startPoint, Gf.Vec3d(0, 0, -1), 0.00001 ))
        self.assertTrue(Gf.IsClose( r.direction, Gf.Vec3d(0, 0, -1), 0.00001 ))

        r = Gf.Frustum().ComputePickRay(Gf.Vec3d(2, 2, -1))
        self.assertTrue(Gf.IsClose( r.startPoint, Gf.Vec3d(2./3, 2./3, -1./3), 0.00001 ))
        self.assertTrue(Gf.IsClose( r.direction, Gf.Vec3d(2./3, 2./3, -1./3), 0.00001 ))

        f = Gf.Frustum()
        f.projectionType = f.Orthographic
        r = f.ComputePickRay(Gf.Vec3d(2, 2, -2))
        self.assertTrue(Gf.IsClose( r.startPoint, Gf.Vec3d(2, 2, -1), 0.00001 ))
        self.assertTrue(Gf.IsClose( r.direction, Gf.Vec3d(0, 0, -1), 0.00001 ))

    def test_EmptyFrustumIntersection(self):
        self.assertFalse(Gf.Frustum().Intersects(Gf.BBox3d()))
        self.assertTrue(Gf.Frustum().Intersects(Gf.BBox3d(Gf.Range3d(Gf.Vec3d(-1,-1,-1),
                                                Gf.Vec3d(1,1,1)))))
        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(0,0,-1)))
        self.assertFalse(Gf.Frustum().Intersects(Gf.Vec3d(0,0,0)))

        self.assertFalse(Gf.Frustum().Intersects(Gf.Vec3d(), Gf.Vec3d(1,1,1), Gf.Vec3d(0,0,1)))
        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(), Gf.Vec3d(-1,-1,-1), Gf.Vec3d(0,0,1)))

        self.assertFalse(Gf.Frustum().Intersects(Gf.Vec3d(0, 100, -100), \
                                        Gf.Vec3d(-100,-100,100), \
                                        Gf.Vec3d(100,-100,100)))
        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(0, 10, 100), \
                                    Gf.Vec3d(-100,-10,-10), \
                                    Gf.Vec3d(100,-10,-10)))
        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(0, 1, 1), \
                                    Gf.Vec3d(50,0,-50), \
                                    Gf.Vec3d(-50,0,-50)))

        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(), Gf.Vec3d(-1,-1,-1), Gf.Vec3d(0,0,1)))

        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(0,0,0), Gf.Vec3d(1,1,-1)))
        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(-100,0,-1), Gf.Vec3d(100,0,-1)))
        self.assertTrue(Gf.Frustum().Intersects(Gf.Vec3d(0,100,-1), Gf.Vec3d(0,-100,-1)))
        self.assertFalse(Gf.Frustum().Intersects(Gf.Vec3d(-100,0,1), Gf.Vec3d(100,0,-1)))
        self.assertFalse(Gf.Frustum().Intersects(Gf.Vec3d(0,0,0), Gf.Vec3d(1,1,1)))

    def test_Str(self):
        f = Gf.Frustum()
        f.projectionType = f.Perspective
        self.assertTrue(len(str(f)))
        f.projectionType = f.Orthographic
        self.assertTrue(len(str(f)))

    def test_IntersectionViewVolume(self):
    
        # a viewProjMat corresponding to a persp cam looking down the Y axis,
        # aimed at the origin.
        viewMat = Gf.Matrix4d(1.0, 0.0, 0.0, 0.0,
                            0.0, 0.0, -1.0, 0.0,
                            0.0, 1.0, 0.0, 0.0,
                            0.0, 0.0, -20, 1.0)
        projMat = Gf.Matrix4d(4.241894005673533, 0.0, 0.0, 0.0,
                            0.0, 4.2418940586972074, 0.0, 0.0,
                            0.0, 0.0, -1, -1.0,
                            0.0, 0.0, -20, 0.0)
        viewProjMat = viewMat * projMat

        # a typical box entirely in the view
        b = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 0, 0, 0 ), Gf.Vec3d( 1, 1, 1 ) ) )
        self.assertTrue(Gf.Frustum.IntersectsViewVolume(b,viewProjMat))

        # a typical box entirely out of the view
        b = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( 100, 0, 0 ), Gf.Vec3d( 101, 1, 1 ) ) )
        self.assertFalse(Gf.Frustum.IntersectsViewVolume(b,viewProjMat))

        # a large box entirely enclosing the view
        b = Gf.BBox3d( Gf.Range3d( Gf.Vec3d( -1e9, -1e9, -1e9 ),
                    Gf.Vec3d( 1e9, 1e9, 1e9 ) ) )
        self.assertTrue(Gf.Frustum.IntersectsViewVolume(b,viewProjMat))

    def test_Serialization(self):
        f = Gf.Frustum(Gf.Vec3d(3,4,5), Gf.Rotation((1,2,3),40),
                    Gf.Range2d(Gf.Vec2d(-0.2,-0.3), Gf.Vec2d(0.2, 0.33)),
                    Gf.Range1d(10, 100),
                    Gf.Frustum.Perspective,
                    viewDistance = 20)

        f2 = eval(repr(f))

        f3 = Gf.Frustum(rotation = Gf.Rotation((1,2,3),40),
                    nearFar = Gf.Range1d(10, 100),
                    projectionType = Gf.Frustum.Perspective,
                    position = Gf.Vec3d(3,4,5), 
                    viewDistance = 20,
                    window = Gf.Range2d(Gf.Vec2d(-0.2,-0.3), Gf.Vec2d(0.2, 0.33)))

        self.assertAlmostEqual(f, f2)
        self.assertAlmostEqual(f, f3)
        self.assertAlmostEqual(f.viewDistance, 20.0)
        self.assertAlmostEqual(f2.viewDistance, 20.0)

    def test_ConstructFromMatrix(self):
        m = Gf.Matrix4d(0.9987016645043332, -0.035803686178599, -0.036236464677155, 0.0,
                        0.0362364646771555,  0.999278702502407,  0.011357524061459, 0.0,
                        0.0358036861785999, -0.012655859557126,  0.999278702502407, 0.0,
                        3.0               , -6.0              ,  5.0              , 1.0)

        f = Gf.Frustum(m,
                    Gf.Range2d(Gf.Vec2d(-0.22,-0.2), Gf.Vec2d(0.2, 0.33)),
                    Gf.Range1d(20, 90),
                    Gf.Frustum.Perspective)

        f = Gf.Frustum(m,
                    Gf.Range2d(Gf.Vec2d(-0.22,-0.2), Gf.Vec2d(0.2, 0.33)),
                    Gf.Range1d(20, 90),
                    Gf.Frustum.Perspective)

        corners = f.ComputeCorners()
        results = (Gf.Vec3d( -2.255306906099,  -9.58646139968125, -14.8715637017144),
                   Gf.Vec3d(  6.133787075736,  -9.88721236358150, -15.1759500050026),
                   Gf.Vec3d( -1.871200380521,   1.00589284684426, -14.7511739466630),
                   Gf.Vec3d(  6.517893601314,   0.70514188294401, -15.0555602499511),
                   Gf.Vec3d(-20.648881077448, -22.13907629856565, -84.4220366577152),
                   Gf.Vec3d( 17.102041840815, -23.49245563611677, -85.7917750225117),
                   Gf.Vec3d(-18.920401712348,  25.52651781079917, -83.8802827599836),
                   Gf.Vec3d( 18.830521205915,  24.17313847324806, -85.2500211247801))

        self.assertEqual(len(corners), len(results))
        for i in range(len(results)):
            self.assertTrue(Gf.IsClose(corners[i], results[i], 0.0001))
            
        corners = f.ComputeCornersAtDistance(20)
        for i in range(len(corners)):
            self.assertTrue(Gf.IsClose(corners[i], results[i], 0.0001))

        corners = f.ComputeCornersAtDistance(90)
        for i in range(len(corners)):
            self.assertTrue(Gf.IsClose(corners[i], results[i+4], 0.0001))

        corners = f.ComputeCornersAtDistance((20 + 90) / 2.0)
        for i in range(len(corners)):
            self.assertTrue(
                Gf.IsClose(corners[i], (results[i] + results[i+4]) / 2.0,
                           0.0001))
        

if __name__ == '__main__':
    unittest.main()
