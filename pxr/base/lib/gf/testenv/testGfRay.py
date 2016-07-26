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
import math
import random
import unittest

from pxr import Gf

epsilon = 1e-6

class TestGfRay(unittest.TestCase):

    def test_Ray(self):
        self.assertIsInstance(Gf.Ray(), Gf.Ray)
        self.assertIsInstance(Gf.Ray(Gf.Vec3d(), Gf.Vec3d()), Gf.Ray)

    def test_SetPointAndDirection(self):
        r = Gf.Ray()
        r.SetPointAndDirection(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4))
        self.assertEqual(Gf.Vec3d(1,2,3), r.startPoint)
        self.assertEqual(Gf.Vec3d(2,3,4), r.direction)

    def test_SetEnds(self):
        r = Gf.Ray()
        r.SetEnds(Gf.Vec3d(3,2,1), Gf.Vec3d(4,3,2))
        self.assertEqual(Gf.Vec3d(3,2,1), r.startPoint)
        self.assertEqual(Gf.Vec3d(4,3,2)-Gf.Vec3d(3,2,1), r.direction)
        self.assertEqual(Gf.Vec3d(3,2,1), r.GetPoint(0))
        self.assertEqual(Gf.Vec3d(4,3,2), r.GetPoint(1))
        self.assertEqual(Gf.Lerp(0.5, Gf.Vec3d(3,2,1), Gf.Vec3d(4,3,2)), r.GetPoint(0.5))

    def test_Properties(self):
        r = Gf.Ray()
        r.startPoint = Gf.Vec3d(1,2,3)
        self.assertEqual(Gf.Vec3d(1,2,3), r.startPoint)
        r.direction = Gf.Vec3d(3,2,1)
        self.assertEqual(Gf.Vec3d(3,2,1), r.direction)

    def test_Transform(self):
        r = Gf.Ray(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4))
        r.Transform(Gf.Matrix4d().SetRotate(Gf.Rotation(Gf.Vec3d(1,0,0), 30)))
        self.assertTrue(Gf.IsClose(r.startPoint, Gf.Vec3d(1.0, 0.23205080756887764, 3.598076211353316), 0.00001))
        self.assertTrue(Gf.IsClose(r.direction, Gf.Vec3d(2.0, 0.59807621135331623, 4.9641016151377544), 0.00001))

    def test_FindClosestPoint(self):
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(3,4,5))
        (p, dist) = r.FindClosestPoint(Gf.Vec3d(1,1,1))
        self.assertTrue(Gf.IsClose(p, Gf.Vec3d(0.72, 0.96, 1.2), 0.00001))
        self.assertTrue(Gf.IsClose(dist, 0.24, 0.00001))

        # non-intersecting case
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(3,4,5))
        l = Gf.Line(Gf.Vec3d(1,0,0), Gf.Vec3d(3,3,3))
        (intersects, rayPoint, linePoint, rayDistance, lineDistance) = \
                    Gf.FindClosestPoints(r, l)
        self.assertTrue(intersects and \
            rayPoint == Gf.Vec3d() and \
            Gf.IsClose(linePoint, Gf.Vec3d(-4./3,-7./3,-7./3), 0.00001) and \
            rayDistance == 0 and \
            Gf.IsClose(lineDistance, -4.04145188433, 0.00001))

        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(3,4,5))
        l = Gf.Line(Gf.Vec3d(1,0,0), Gf.Vec3d(3,4,5))
        (intersects, rayPoint, linePoint, rayDistance, lineDistance) = \
                    Gf.FindClosestPoints(r, l)
        self.assertFalse(intersects)

        # closest point on theis line segment to the ray is going to 
        # be the near end, (1,0,0)
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(3,4,5))
        l = Gf.LineSeg(Gf.Vec3d(1,0,0), Gf.Vec3d(3,3,3))
        (intersects, rayPoint, linePoint, rayDistance, lineDistance) = \
                    Gf.FindClosestPoints(r, l)
        (closestRayPtToNearEnd,rayDistToNearEnd) = r.FindClosestPoint(Gf.Vec3d(1,0,0))

        self.assertTrue(intersects and \
            Gf.IsClose(rayPoint, closestRayPtToNearEnd, 0.00001) and \
            Gf.IsClose(linePoint, Gf.Vec3d(1,0,0), 0.00001) and \
            Gf.IsClose(rayDistance, rayDistToNearEnd, 0.00001) and \
            Gf.IsClose(lineDistance, 0, 0.00001))

        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(3,4,5))
        l = Gf.LineSeg(Gf.Vec3d(1,0,0), Gf.Vec3d(4,4,5))
        (intersects, rayPoint, linePoint, rayDistance, lineDistance) = \
                    Gf.FindClosestPoints(r, l)
        self.assertFalse(intersects)

    def test_Equality(self):
        r = Gf.Ray()
        r.SetPointAndDirection(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4))
        self.assertTrue(r == Gf.Ray(Gf.Vec3d(1,2,3), Gf.Vec3d(2,3,4)))
        self.assertTrue(r != Gf.Ray(Gf.Vec3d(1,2,3), Gf.Vec3d(4,3,2)))

    def test_Intersect(self):
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        (hit, distance, barycentricCoords, frontFacing) = \
            r.Intersect(Gf.Vec3d(1,0,0), Gf.Vec3d(0,1,0), Gf.Vec3d(0,0,1))
        self.assertTrue(hit and Gf.IsClose(distance, 1./3, 0.00001) and \
            Gf.IsClose(barycentricCoords, Gf.Vec3d(1./3, 1./3, 1./3), 0.00001) and \
            frontFacing == False)

        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        (hit, distance, barycentricCoords, frontFacing) = \
            r.Intersect(Gf.Vec3d(5,0,0), Gf.Vec3d(5,5,0), Gf.Vec3d(5,0,5))
        self.assertFalse(hit)

        r = Gf.Ray(Gf.Vec3d(57.171, 91.9027, 6.53313),
                Gf.Vec3d(0.0386784, 0.682603, 0.696032))

        p1 = Gf.Vec3d(38.1018, 62.1285, 70.9721)
        p2 = Gf.Vec3d(66.1114, 30.6822, 38.1398)
        p3 = Gf.Vec3d(72.8148, 83.26, 74.535)

        self.assertEqual((False, 0.0, Gf.Vec3d(0.0, 0.0, 0.0), False), r.Intersect(p1, p2, p3))

    def test_IntersectNumericalPrecision(self):
        # These tests are designed to tickle the numerics of triangle edge intersections.
        # Note that they may not activate the numerical fuzz handling on all architectures.
        r = Gf.Ray(Gf.Vec3d(-3.4691279999999995454,-14.354708800000000934,163.16137119999999072), Gf.Vec3d(1,0,0))
        p1 = Gf.Vec3d(4.0969199999999998951,-17.020839999999999748,167.70099999999999341)
        p2 = Gf.Vec3d(0.093717998399999835613,-13.818282001600001863,163.69779799840000578)
        p3 = Gf.Vec3d(4.0969199999999998951,-15.219400000000000261,162.29667999999998074)
        r.Intersect(p1,p2,p3)
        r.Intersect(p3,p1,p2)
        r.Intersect(p2,p3,p1)

    def test_IntersectTriangle(self):
        '''Ray originates on surface of triangle, direction equal to triangle normal
        (CCW winding)'''
        r = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d.ZAxis())
        (hit, distance, barycentricCoords, frontFacing) = \
            r.Intersect(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 0, 0), Gf.Vec3d(0, 1, 0)) 
        self.assertTrue(hit and Gf.IsClose(distance, 0.0, 0.00001) and not frontFacing)

    def test_IntersectPlane(self):
        p = Gf.Plane(Gf.Vec3d(1,1,1), 2)
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        (hit, distance, frontFacing) = r.Intersect(p)
        self.assertTrue(hit and Gf.IsClose(distance, 1.1547005383792515, 0.00001) and \
            not frontFacing)

        p = Gf.Plane(Gf.Vec3d(1,0,0), 2)
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(0,1,0))
        (hit, distance, frontFacing) = r.Intersect(p)
        self.assertFalse(hit)

        # Ray originates on surface of plane, direction equal to plane normal
        p = Gf.Plane(Gf.Vec3d.ZAxis(), Gf.Vec3d(0, 0, 0)) 
        r = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d.ZAxis())
        (hit, distance, frontFacing) = r.Intersect(p)
        self.assertTrue(hit and Gf.IsClose(distance, 0.0, 0.00001) and not frontFacing)

    def test_IntersectBox(self):
        box = Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d(2,2,2))
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(1.5,1.5,2))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertTrue(hit and Gf.IsClose(enterDistance, 2./3, 0.00001) and \
            Gf.IsClose(exitDistance, 1, 0.00001))

        box = Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d())
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(1.5,1.5,2))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertFalse(hit)

        box = Gf.Range3d(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        r = Gf.Ray(Gf.Vec3d(2,2,2), Gf.Vec3d(1,0,0))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertFalse(hit)

        box = Gf.Range3d(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        r = Gf.Ray(Gf.Vec3d(2,0.5,0.5), Gf.Vec3d(-1,0,0))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertTrue(hit)

        box = Gf.Range3d(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        r = Gf.Ray(Gf.Vec3d(0.5,0.5,0.5), Gf.Vec3d(1,0,0))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertTrue(hit and Gf.IsClose(enterDistance, -0.5, 0.00001) and \
            Gf.IsClose(exitDistance, 0.5, 0.00001))

        box = Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d(2,2,2))
        r = Gf.Ray(Gf.Vec3d(), Gf.Vec3d(5,1,1))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertFalse(hit)

        # Ray originates on surface of box
        box = Gf.Range3d(Gf.Vec3d(), Gf.Vec3d(1,1,1))
        r = Gf.Ray(Gf.Vec3d(0.5, 0.5, 1), Gf.Vec3d.ZAxis())
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertTrue(hit and Gf.IsClose(enterDistance, -1.0, 0.00001) and \
            Gf.IsClose(exitDistance, 0.0, 0.00001))

        # test rays parallel to bboxes
        box = Gf.Range3d(Gf.Vec3d(1,1,1), Gf.Vec3d(2,2,2))
        r = Gf.Ray(Gf.Vec3d(1.5, 0, 0), Gf.Vec3d(0,1,0))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertFalse(hit)
        r = Gf.Ray(Gf.Vec3d(1.5, 0, 1.5), Gf.Vec3d(0,1,0))
        (hit, enterDistance, exitDistance) = r.Intersect(box)
        self.assertTrue(hit)

    def test_IntersectSphere(self):
        center = Gf.Vec3d(0,1,0)
        radius = 0.5
        r = Gf.Ray(Gf.Vec3d(0,0,0), Gf.Vec3d(0,1,0)) # direct hit
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertTrue(hit and Gf.IsClose(enterDistance, 0.5, 0.00001) and \
            Gf.IsClose(exitDistance, 1.5, 0.00001))

        r = Gf.Ray(Gf.Vec3d(0, 1.5, 0), Gf.Vec3d(0,1,0)) # ray on surface of sphere
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertTrue(hit and Gf.IsClose(enterDistance, -1.0, 0.00001) and \
            Gf.IsClose(exitDistance, 0.0, 0.00001))

        r = Gf.Ray(Gf.Vec3d(0,2,0), Gf.Vec3d(0,1,0)) # intersects behind start point
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertFalse(hit)

        r = Gf.Ray(Gf.Vec3d(0,1,0), Gf.Vec3d(0,1,0)) # ray inside sphere
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertTrue(hit and Gf.IsClose(enterDistance, -0.5, 0.00001) and \
            Gf.IsClose(exitDistance, 0.5, 0.00001))

        # Infinite ray, the ray intersects the sphere greater than the magnitude of the
        # direction vector
        r = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d(0, 0.1, 0))
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertTrue(hit and Gf.IsClose(enterDistance, 5.0, 0.00001) and \
            Gf.IsClose(exitDistance, 15.0, 0.00001))

        r = Gf.Ray(Gf.Vec3d(2,0,0), Gf.Vec3d(0,1,0)) # total miss
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertFalse(hit)

        r = Gf.Ray(Gf.Vec3d(0.5,0,0), Gf.Vec3d(0,1,0)) # tangent
        (hit, enterDistance, exitDistance) = r.Intersect(center, radius)
        self.assertTrue(hit and Gf.IsClose( enterDistance, 1.0, 0.00001) and \
            Gf.IsClose( exitDistance, 1.0, 0.00001))

        # Ray is tangential to sphere
        z = -1.7695346483064331
        direction = Gf.Vec3d.ZAxis()

        startPoint = Gf.Vec3d(0.0, 1.0, z)
        ray = Gf.Ray(startPoint, direction)

        center = Gf.Vec3d(0.0, 0.0, 0.0)
        radius = 1.0
        (hit, enter, exit) = ray.Intersect(center, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -z, delta=epsilon)
        self.assertAlmostEqual(exit, -z, delta=epsilon)

    def test_IntersectCylinder(self):
        # Z-axis aligned cylinder
        origin = Gf.Vec3d(0, 0, 0)
        axis = Gf.Vec3d().ZAxis()
        radius = 1

        # Ray inside cylinder, orthogonal to surface
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Ray inside cylinder, orthogonal to surface (parametric distance)
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis() / 2.0)
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -2.0 * radius, delta=epsilon)
        self.assertAlmostEqual(exit, 2.0 * radius, delta=epsilon)

        # Ray inside cylinder, orthogonal to surface (scaled axis)
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, 2.0 * axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Ray behind cylinder, orthogonal to surface
        x = radius * (1 + random.uniform(epsilon, 1))
        distance = x - radius;
        ray = Gf.Ray(Gf.Vec3d(-x, 0, 0), Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, distance, delta=epsilon)
        self.assertAlmostEqual(exit, distance + 2.0 * radius, delta=epsilon)

        # Ray infront of cylinder, orthogonal to surface
        ray = Gf.Ray(Gf.Vec3d(x, 0, 0), Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)
        self.assertFalse(hit)

        # Ray inside cylinder, along z-axis
        ray = Gf.Ray(origin, Gf.Vec3d().ZAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)
        self.assertFalse(hit)

        # Ray inside cylinder, parallel to surface
        ray = Gf.Ray(Gf.Vec3d(radius / 2.0, 0, 0), Gf.Vec3d().ZAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)
        self.assertFalse(hit)

        # Ray on surface, parallel to surface
        ray = Gf.Ray(Gf.Vec3d(radius, 0, 0), Gf.Vec3d().ZAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)
        self.assertFalse(hit)

        # Ray outside cylinder, parallel to surface
        ray = Gf.Ray(Gf.Vec3d(x, 0, 0), Gf.Vec3d().ZAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)
        self.assertFalse(hit)

        # Ray behind cylinder, not orthogonal to surface
        direction = Gf.Vec3d(1, 0, 1).GetNormalized()
        ray = Gf.Ray(Gf.Vec3d(-x, 0, 0), direction)
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, distance * math.sqrt(2), delta=epsilon)
        self.assertAlmostEqual(exit, (distance + 2.0 * radius) * math.sqrt(2), delta=epsilon)

        # Non-uniform origin
        origin = Gf.Vec3d()
        for i in range(3):
            origin[i] = random.uniform(epsilon, 1)

        # Ray inside cylinder, orthogonal to surface
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Non-uniform radius
        radius = random.uniform(epsilon, 10)
        origin = Gf.Vec3d(0, 0, 0)

        # Ray inside cylinder, orthogonal to surface
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Y-axis aligned cylinder
        origin = Gf.Vec3d(0, 0, 0)
        axis = Gf.Vec3d().YAxis()
        radius = 1

        # Ray inside cylinder, orthogonal to surface
        ray = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d().ZAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        # Rotated frame
        origin = Gf.Vec3d(0, 0, 0)
        axis = Gf.Vec3d(1, 0, 1)
        radius = 1

        # Ray inside cylinder, orthogonal to surface
        ray = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 0, -1).GetNormalized())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

    def test_IntersectCone(self):
        # Z-axis aligned cone
        origin = Gf.Vec3d(0, 0, 0)
        axis = Gf.Vec3d().ZAxis()
        radius = 1

        x = radius * (1 + random.uniform(epsilon, 1))

        # Varying heights
        heights = (1, 2)

        for height in heights:
            # Ray inside cone, orthogonal to axis
            ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

            self.assertTrue(hit)
            self.assertAlmostEqual(enter, -radius, delta=epsilon)
            self.assertAlmostEqual(exit, radius, delta=epsilon)

            # Ray inside cone, orthogonal to axis (parametric distance)
            ray = Gf.Ray(origin, Gf.Vec3d().XAxis() / 2.0)
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

            self.assertTrue(hit)
            self.assertAlmostEqual(enter, -2.0 * radius, delta=epsilon)
            self.assertAlmostEqual(exit, 2.0 * radius, delta=epsilon)

            # Ray inside cone, orthogonal to surface (scaled axis)
            ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
            (hit, enter, exit) = ray.Intersect(origin, 2.0 * axis, radius)

            self.assertTrue(hit)
            self.assertAlmostEqual(enter, -radius, delta=epsilon)
            self.assertAlmostEqual(exit, radius, delta=epsilon)
            
            # Ray behind cone, orthogonal to axis
            distance = x - radius;
            ray = Gf.Ray(Gf.Vec3d(-x, 0, 0), Gf.Vec3d().XAxis())
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

            self.assertTrue(hit)
            self.assertAlmostEqual(enter, distance, delta=epsilon)
            self.assertAlmostEqual(exit, distance + 2.0 * radius, delta=epsilon)

            # Ray infront of cone, orthogonal to axis
            ray = Gf.Ray(Gf.Vec3d(x, 0, 0), Gf.Vec3d().XAxis())
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
            self.assertFalse(hit)

            # Ray inside cone, along z-axis
            ray = Gf.Ray(origin, Gf.Vec3d().ZAxis())
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
            
            self.assertTrue(hit)
            self.assertAlmostEqual(enter, height, delta=epsilon)
            self.assertAlmostEqual(exit, height, delta=epsilon)
            
            # Ray above cone, along z-axis
            z = height * random.uniform(1 + epsilon, 10)
            ray = Gf.Ray(Gf.Vec3d(0, 0, z), Gf.Vec3d().YAxis())
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
            self.assertFalse(hit)

            # Ray inside cone, parallel to z-axis
            ray = Gf.Ray(Gf.Vec3d(radius / 2.0, 0, 0), Gf.Vec3d().ZAxis())
            distance = height / 2.0
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
            
            self.assertTrue(hit)
            self.assertAlmostEqual(enter, distance, delta=epsilon)
            self.assertAlmostEqual(exit, distance, delta=epsilon)
            
            # Ray on surface, parallel to surface
            ray = Gf.Ray(Gf.Vec3d(radius, 0, 0), Gf.Vec3d(-radius, 0, height))
            (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
            self.assertFalse(hit)

        # Ray parallel to cone
        height = 1
        direction = Gf.Vec3d(1, 0, height / radius).GetNormalized()

        # Ray inside and parallel to cone, a = 0 in quadratic
        ray = Gf.Ray(origin, direction)
        distance = math.sqrt((radius ** 2 + height ** 2)) / 2.0
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, distance, delta=epsilon)
        self.assertAlmostEqual(exit, distance, delta=epsilon)

        # Ray outside and parallel to cone, a = 0 in quadratic
        ray = Gf.Ray(Gf.Vec3d(x, 0, 0), direction)
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
        self.assertFalse(hit)

        # Ray parallel to cone, a = c = 0 in quadratic
        ray = Gf.Ray(Gf.Vec3d(radius, 0, 0), direction)
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, 0.0, delta=epsilon)
        self.assertAlmostEqual(exit, 0.0, delta=epsilon)

        # Ray parallel to cone, a = b = c = 0 in quadratic
        ray = Gf.Ray(Gf.Vec3d(-radius, 0, 0), direction)
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
        self.assertFalse(hit)

        # Ray above cone, orthognal to axis
        z = height * random.uniform(1 + epsilon, 10)
        ray = Gf.Ray(Gf.Vec3d(0, 0, z), Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)
        self.assertFalse(hit)

        # Non-uniform origin
        origin = Gf.Vec3d()
        for i in range(3):
            origin[i] = random.uniform(epsilon, 1)

        # Ray inside cone, orthogonal to axis
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Non-uniform radius
        radius = random.uniform(epsilon, 10)
        origin = Gf.Vec3d(0, 0, 0)

        # Ray inside cone, orthogonal to axis
        ray = Gf.Ray(origin, Gf.Vec3d().XAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Y-axis aligned cone
        origin = Gf.Vec3d(0, 0, 0)
        axis = Gf.Vec3d().YAxis()
        radius = 1

        # Ray inside cone, orthogonal to axis
        ray = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d().ZAxis())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius, height)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

        # Rotated frame
        origin = Gf.Vec3d(0, 0, 0)
        axis = Gf.Vec3d(1, 0, 1)
        radius = 1

        # Ray inside cone, orthogonal to surface
        ray = Gf.Ray(Gf.Vec3d(0, 0, 0), Gf.Vec3d(1, 0, -1).GetNormalized())
        (hit, enter, exit) = ray.Intersect(origin, axis, radius)

        self.assertTrue(hit)
        self.assertAlmostEqual(enter, -radius, delta=epsilon)
        self.assertAlmostEqual(exit, radius, delta=epsilon)

    def test_Str(self):
        r = Gf.Ray(Gf.Vec3d(0.5,0,0), Gf.Vec3d(0,1,0))
        self.assertEqual(r, eval(repr(r)))
        self.assertGreater(len(str(Gf.Ray())), 0)

if __name__ == '__main__':
    unittest.main()
