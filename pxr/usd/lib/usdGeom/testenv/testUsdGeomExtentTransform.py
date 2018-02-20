#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

import unittest

from pxr import Vt, Gf, Sdf, Usd, UsdGeom


# A transform matrix which has rotation, translation, and scale. Since the scale
# squashes in the Z direction after the rotation, some skew is introduced.
STRANGE_TRANSFORM = (
    Gf.Matrix4d(Gf.Rotation(Gf.Vec3d(1, 1, 1), 45.0), Gf.Vec3d(1, 2, 3)) *
    Gf.Matrix4d(Gf.Vec4d(1, 1, 0.5, 1)))

# Maximum tolerated error between the dimensions of an extent during comparison.
EXTENT_TOLERANCE = 0.00001


class TestUsdGeomExtentTransform(unittest.TestCase):

    def ensureExtent(self, extent):
        """Ensure that a Vt.Vec3fArray or Python list is a valid extent."""
        self.assertEqual(len(extent), 2)
        return Vt.Vec3fArray(2, (Gf.Vec3f(extent[0]), Gf.Vec3f(extent[1])))

    def assertExtentsEqual(self, extent1, extent2):
        """Test that two extents are equal."""
        extent1 = self.ensureExtent(extent1)
        extent2 = self.ensureExtent(extent2)
        self.assertTrue(Gf.IsClose(extent1[0], extent2[0], EXTENT_TOLERANCE))
        self.assertTrue(Gf.IsClose(extent1[1], extent2[1], EXTENT_TOLERANCE))

    def verifyExtent(self, boundable, expectedExtent, transform=None):
        """Compute the extent for a boundable object, and verify that it is
        equal to an expected extend. If a transform matrix is given, it is
        passed to the extent computation.
        """
        if transform is None:
            computedExtent = UsdGeom.Boundable.ComputeExtentFromPlugins(
                boundable, Usd.TimeCode.Default())
        else:
            computedExtent = UsdGeom.Boundable.ComputeExtentFromPlugins(
                boundable, Usd.TimeCode.Default(), transform)
        self.assertExtentsEqual(computedExtent, expectedExtent)

    def test_Sphere(self):
        stage = Usd.Stage.CreateInMemory()
        sphere = UsdGeom.Sphere.Define(stage, "/Foo")
        sphere.CreateRadiusAttr(2.0)

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(sphere, [(-2, -2, -2), (2, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(sphere, [(-2, -2, -2), (2, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(sphere,
            [(-2.242468870104182, -1.2424688701041822, -0.12123443505209108),
             (4.242468870104182, 5.242468870104181, 3.121234435052091)],
            STRANGE_TRANSFORM)

    def test_Cube(self):
        stage = Usd.Stage.CreateInMemory()
        cube = UsdGeom.Cube.Define(stage, "/Foo")
        cube.CreateSizeAttr(4.0)

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(cube, [(-2, -2, -2), (2, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(cube, [(-2, -2, -2), (2, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(cube,
            [(-2.242468870104182, -1.2424688701041822, -0.12123443505209108),
             (4.242468870104182, 5.242468870104181, 3.121234435052091)],
            STRANGE_TRANSFORM)

    def test_Cylinder(self):
        stage = Usd.Stage.CreateInMemory()

        cylinder = UsdGeom.Cylinder.Define(stage, "/Foo")
        cylinder.CreateHeightAttr(4.0)
        cylinder.CreateRadiusAttr(2.0)
        cylinder.CreateAxisAttr("X")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(cylinder, [(-2, -2, -2), (2, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(cylinder, [(-2, -2, -2), (2, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(cylinder,
            [(-2.242468870104182, -1.2424688701041822, -0.12123443505209108),
             (4.242468870104182, 5.242468870104181, 3.121234435052091)],
            STRANGE_TRANSFORM)

        # Test that the extent computation changes when the height changes.
        longCylinder = UsdGeom.Cylinder.Define(stage, "/Foo")
        longCylinder.CreateHeightAttr(6.0)
        longCylinder.CreateRadiusAttr(2.0)
        longCylinder.CreateAxisAttr("X")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(longCylinder, [(-3, -2, -2), (3, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(longCylinder, [(-3, -2, -2), (3, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(longCylinder,
            [(-3.0472067242285474, -1.7483482335058629, -0.27654304381511374),
             (5.0472067242285465, 5.7483482335058635, 3.2765430438151135)],
            STRANGE_TRANSFORM)

        # Test that the extent computation changes when the radius or axis changes.
        wideCylinderZ = UsdGeom.Cylinder.Define(stage, "/Foo")
        wideCylinderZ.CreateHeightAttr(4.0)
        wideCylinderZ.CreateRadiusAttr(3.0)
        wideCylinderZ.CreateAxisAttr("Z")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(wideCylinderZ, [(-3, -3, -2), (3, 3, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(wideCylinderZ, [(-3, -3, -2), (3, 3, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(wideCylinderZ,
            [(-3.3578239417545923, -2.553086087630228, -0.5294827255159541),
             (5.357823941754592, 6.553086087630229, 3.529482725515954)],
            STRANGE_TRANSFORM)

    def test_Cone(self):
        stage = Usd.Stage.CreateInMemory()

        cone = UsdGeom.Cone.Define(stage, "/Foo")
        cone.CreateHeightAttr(4.0)
        cone.CreateRadiusAttr(2.0)
        cone.CreateAxisAttr("X")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(cone, [(-2, -2, -2), (2, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(cone, [(-2, -2, -2), (2, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(cone,
            [(-2.242468870104182, -1.2424688701041822, -0.12123443505209108),
             (4.242468870104182, 5.242468870104181, 3.121234435052091)],
            STRANGE_TRANSFORM)

        # Test that the extent computation changes when the height changes.
        longCone = UsdGeom.Cone.Define(stage, "/Foo")
        longCone.CreateHeightAttr(6.0)
        longCone.CreateRadiusAttr(2.0)
        longCone.CreateAxisAttr("X")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(longCone, [(-3, -2, -2), (3, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(longCone, [(-3, -2, -2), (3, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(longCone,
            [(-3.0472067242285474, -1.7483482335058629, -0.27654304381511374),
             (5.0472067242285465, 5.7483482335058635, 3.2765430438151135)],
            STRANGE_TRANSFORM)

        # Test that the extent computation changes when the radius or axis changes.
        wideConeZ = UsdGeom.Cone.Define(stage, "/Foo")
        wideConeZ.CreateHeightAttr(4.0)
        wideConeZ.CreateRadiusAttr(3.0)
        wideConeZ.CreateAxisAttr("Z")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(wideConeZ, [(-3, -3, -2), (3, 3, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(wideConeZ, [(-3, -3, -2), (3, 3, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(wideConeZ,
            [(-3.3578239417545923, -2.553086087630228, -0.5294827255159541),
             (5.357823941754592, 6.553086087630229, 3.529482725515954)],
            STRANGE_TRANSFORM)

    def test_Capsule(self):
        stage = Usd.Stage.CreateInMemory()

        capsule = UsdGeom.Capsule.Define(stage, "/Foo")
        capsule.CreateHeightAttr(4.0)
        capsule.CreateRadiusAttr(2.0)
        capsule.CreateAxisAttr("X")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(capsule, [(-4, -2, -2), (4, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(capsule, [(-4, -2, -2), (4, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(capsule,
            [(-3.851944578352912, -2.254227596907543, -0.4318516525781366),
             (5.851944578352912, 6.254227596907542, 3.4318516525781364)],
            STRANGE_TRANSFORM)

        # Test that the extent computation changes when the height changes.
        longCapsule = UsdGeom.Capsule.Define(stage, "/Foo")
        longCapsule.CreateHeightAttr(6.0)
        longCapsule.CreateRadiusAttr(2.0)
        longCapsule.CreateAxisAttr("X")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(longCapsule, [(-5, -2, -2), (5, 2, 2)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(longCapsule, [(-5, -2, -2), (5, 2, 2)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(longCapsule,
            [(-4.656682432477277, -2.760106960309224, -0.5871602613411594),
             (6.656682432477277, 6.760106960309223, 3.5871602613411593)],
            STRANGE_TRANSFORM)

        # Test that the extent computation changes when the radius or axis changes.
        wideCapsuleZ = UsdGeom.Capsule.Define(stage, "/Foo")
        wideCapsuleZ.CreateHeightAttr(4.0)
        wideCapsuleZ.CreateRadiusAttr(3.0)
        wideCapsuleZ.CreateAxisAttr("Z")

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(wideCapsuleZ, [(-3, -3, -5), (3, 3, 5)])

        # Apply the identity matrix. This should be identical to the extent
        # computed with no transform.
        self.verifyExtent(wideCapsuleZ, [(-3, -3, -5), (3, 3, 5)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix.
        self.verifyExtent(wideCapsuleZ,
            [(-4.875462031959634, -3.4849377402083643, -1.7365895067025015),
             (6.875462031959634, 7.484937740208365, 4.736589506702502)],
            STRANGE_TRANSFORM)

    def test_PointInstancer(self):
        stage = Usd.Stage.Open("testPointInstancer.usda")

        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/Instancer"))
        
        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(pi, [(-1, -1, -1), (3.5, 3.5, 3.5)])
        
        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(pi, [(-1, -1, -1), (3.5, 3.5, 3.5)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix. Note that this is different from
        # the extent displayed in Usdview. If you open testPointInstancer.usda
        # and select /StrangeInstancer, you will see the box is larger than it
        # needs to be (it transforms the bounding box around the entire
        # PointInstancer, not the boxes for each instance like
        # ComputeExtentFromPlugins). If you select
        # /StrangeInstancerComputedExtent, it will display the (tighter)
        # computed extent used below.
        self.verifyExtent(pi,
            [(-1.3977774381637573, 0.3787655532360077, 0.689382791519165),
             (5.12123441696167, 6.12123441696167, 3.560617208480835)],
            STRANGE_TRANSFORM)

    def test_PointBased(self):
        stage = Usd.Stage.Open("testPointBased.usda")

        pointBased = UsdGeom.PointBased(stage.GetPrimAtPath("/Points"))

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(pointBased, [(0, 0, 0), (3.0, 2.0, 2.0)])

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(
            pointBased, [(0, 0, 0), (3.0, 2.0, 2.0)], Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix. Note that this is different from
        # the extent displayed in Usdview. If you open testPoints.usda and
        # select /StrangePoints, you will see the box is larger than it needs to
        # be (it transforms the bounding box around the points, not the points
        # themselves like ComputeExtentFromPlugins). If you select
        # /StrangePointsComputedExtent, it will display the (much tighter)
        # computed extent used below.
        self.verifyExtent(pointBased,
            [(0.3787655532360077, 2.0, 1.5),
             (4.4259724617004395, 3.609475612640381, 2.0058794021606445)],
            STRANGE_TRANSFORM)

    def test_Points(self):
        stage = Usd.Stage.Open("testPoints.usda")

        points = UsdGeom.Points(stage.GetPrimAtPath("/Points"))

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(points, [(-0.5, -0.5, -0.5), (3.25, 2.0, 2.25)])

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(points, [(-0.5, -0.5, -0.5), (3.25, 2.0, 2.25)],
            Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix. Note that this is different from
        # the extent displayed in Usdview. If you open testPoints.usda and
        # select /StrangePoints, you will see the box is larger than it needs to
        # be (it transforms the bounding box around the points, not the points
        # themselves like ComputeExtentFromPlugins). If you select
        # /StrangePointsComputedExtent, it will display the (much tighter)
        # computed extent used below.
        self.verifyExtent(points,
            [(0.18938279151916504, 1.189382791519165, 1.0946913957595825),
             (4.8312811851501465, 3.609475612640381, 2.041466236114502)],
            STRANGE_TRANSFORM)

    def test_Curves(self):
        stage = Usd.Stage.Open("testCurves.usda")

        curves = UsdGeom.Curves(stage.GetPrimAtPath("/Curves"))

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(curves, [(-0.5, -0.5, -0.5), (3.5, 2.5, 2.5)])

        # Verify the extent computation when no transform matrix is given.
        self.verifyExtent(curves, [(-0.5, -0.5, -0.5), (3.5, 2.5, 2.5)],
            Gf.Matrix4d(1.0))

        # Apply an arbitrary transform matrix. Note that this is different from
        # the extent displayed in Usdview. If you open testPoints.usda and
        # select /StrangePoints, you will see the box is larger than it needs to
        # be (it transforms the bounding box around the curves, not the curves
        # themselves like ComputeExtentFromPlugins). If you select
        # /StrangePointsComputedExtent, it will display the (much tighter)
        # computed extent used below.
        self.verifyExtent(curves,
            [(-0.43185165524482727, 1.189382791519165, 1.0946913957595825),
             (5.236589431762695, 4.420092582702637, 2.4111881256103516)],
            STRANGE_TRANSFORM)


if __name__ == "__main__":
    unittest.main()
