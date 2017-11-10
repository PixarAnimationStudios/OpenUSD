#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Gf, Usd, UsdGeom
import unittest

class TestUsdGeomPointInstancer(unittest.TestCase):
    def _AddCubeModel(self, stage, primPath):
        cubeRefPrim = stage.DefinePrim(primPath)
        refs = cubeRefPrim.GetReferences()
        refs.AddReference("CubeModel.usda")
        return cubeRefPrim

    def _SetTransformComponentsAndIndices(self, instancer,
            positions=None, scales=None, orientations=None, indices=None):
        self.assertTrue(positions is not None)
        self.assertTrue(indices is not None)

        instancer.CreatePositionsAttr(positions)
        instancer.CreateProtoIndicesAttr(indices)

        if scales is not None:
            instancer.CreateScalesAttr(scales)

        if orientations is not None:
            instancer.CreateOrientationsAttr(orientations)

    def _ValidateExtent(self, instancer, expectedExtent):
        """
        Verifies that the extent of the given UsdGeomPointInstancer matches
        the given expected extent.
        """
        self.assertEqual(len(expectedExtent), 2)

        extent = instancer.ComputeExtentAtTime(
            Usd.TimeCode.Default(), Usd.TimeCode.Default())

        self.assertEqual(len(extent), 2)
        self.assertTrue(Gf.IsClose(extent[0], expectedExtent[0], 1e-6))
        self.assertTrue(Gf.IsClose(extent[1], expectedExtent[1], 1e-6))

    def _ValidateInstanceTransforms(self, instancer, expectedXforms):
        """
        Verifies that the instance transforms of the given
        UsdGeomPointInstancer match the given expected transforms.
        """
        xforms = instancer.ComputeInstanceTransformsAtTime(
            Usd.TimeCode.Default(), Usd.TimeCode.Default())

        self.assertEqual(len(xforms), len(expectedXforms))

        for i in xrange(len(xforms)):
            xf = xforms[i]
            ex = expectedXforms[i]
            for a, b in zip(xf, ex):
                self.assertTrue(Gf.IsClose(a, b, 1e-5))

    def setUp(self):
        self.stage = Usd.Stage.CreateInMemory()

        instancer = UsdGeom.PointInstancer.Define(self.stage, '/MyPointInstancer')
        prototypesPrim = self.stage.DefinePrim(instancer.GetPath().AppendChild('prototypes'))
        prototypesPrimPath = prototypesPrim.GetPath()

        # A cube at the origin that is of length 1 in each dimension.
        originCube = self._AddCubeModel(self.stage, prototypesPrimPath.AppendChild('OriginCube'))

        # A cube at the origin with a scale xformOp that makes it of length 5
        # in each dimension.
        originScaledCube = self._AddCubeModel(self.stage, prototypesPrimPath.AppendChild('OriginScaledCube'))
        xformable = UsdGeom.Xformable(originScaledCube)
        xformable.AddScaleOp().Set(Gf.Vec3f(5.0))

        # A cube with a translate xformOp that is of length 1 in each dimension.
        translatedCube = self._AddCubeModel(self.stage, prototypesPrimPath.AppendChild('TranslatedCube'))
        xformable = UsdGeom.Xformable(translatedCube)
        xformable.AddTranslateOp().Set(Gf.Vec3d(3.0, 6.0, 9.0))

        # A cube with a rotateZ xformOp that is of length 1 in each dimension.
        rotatedCube = self._AddCubeModel(self.stage, prototypesPrimPath.AppendChild('RotatedCube'))
        xformable = UsdGeom.Xformable(rotatedCube)
        xformable.AddRotateZOp().Set(45.0)

        instancer.CreatePrototypesRel().SetTargets([
            originCube.GetPath(),
            originScaledCube.GetPath(),
            translatedCube.GetPath(),
            rotatedCube.GetPath()
        ])

        self.instancer = instancer

    def test_ExtentOneOriginCubeInstance(self):
        """
        Tests that a UsdGeomPointInstancer that instances a single cube
        prototype at a particular location has the correct extent.
        """
        positions = [Gf.Vec3f(10.0, 15.0, 20.0)]
        indices = [0]
        self._SetTransformComponentsAndIndices(self.instancer,
            positions=positions, indices=indices)
        expectedExtent = [
            Gf.Vec3f(9.5, 14.5, 19.5),
            Gf.Vec3f(10.5, 15.5, 20.5),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

    def test_ExtentOneOriginScaledCubeInstance(self):
        """
        Tests that a UsdGeomPointInstancer that instances a single cube
        prototype with scale on it at a particular location has the correct
        extent.
        """
        positions = [Gf.Vec3f(20.0, 30.0, 40.0)]
        indices = [1]
        self._SetTransformComponentsAndIndices(self.instancer,
            positions=positions, indices=indices)
        expectedExtent = [
            Gf.Vec3f(17.5, 27.5, 37.5),
            Gf.Vec3f(22.5, 32.5, 42.5),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

    def test_ExtentOneTranslatedCubeInstance(self):
        """
        Tests that a UsdGeomPointInstancer that instances a single cube
        prototype with translate on it at a particular location has the correct
        extent.
        """
        positions = [Gf.Vec3f(-2.0, -4.0, -6.0)]
        indices = [2]
        self._SetTransformComponentsAndIndices(self.instancer,
            positions=positions, indices=indices)
        expectedExtent = [
            Gf.Vec3f(0.5, 1.5, 2.5),
            Gf.Vec3f(1.5, 2.5, 3.5),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

    def test_ExtentOneRotatedCubeInstance(self):
        """
        Tests that a UsdGeomPointInstancer that instances a single cube
        prototype with rotation on it at a particular location has the correct
        extent.
        """
        positions = [Gf.Vec3f(100.0, 100.0, 100.0)]
        indices = [3]
        self._SetTransformComponentsAndIndices(self.instancer,
            positions=positions, indices=indices)
        expectedExtent = [
            Gf.Vec3f(99.29289, 99.29289, 99.5),
            Gf.Vec3f(100.70711, 100.70711, 100.5),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

    def test_ExtentMultipleCubeInstances(self):
        """
        Tests that a UsdGeomPointInstancer with multiple instances of multiple
        cube prototypes has the correct extent.
        """
        positions = [
            Gf.Vec3f(-33.0, -33.0, 0.0),
            Gf.Vec3f(0.0, 0.0, -44.0),
            Gf.Vec3f(0.0, 222.0, 555.0),
            Gf.Vec3f(66.0, 111.0, 11.0)
        ]
        indices = [
            0,
            1,
            0,
            1
        ]
        self._SetTransformComponentsAndIndices(self.instancer,
            positions=positions, indices=indices)
        expectedExtent = [
            Gf.Vec3f(-33.5, -33.5, -46.5),
            Gf.Vec3f(68.5, 222.5, 555.5),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

    def test_InstanceTransformsAndExtentWithMaskedInstances(self):
        """
        Tests that a UsdGeomPointInstancer with multiple instances of multiple
        cube prototypes has the correct instance transforms and extent both
        with and without masking/deactivating particular instances.
        """
        positions = [
            Gf.Vec3f(-2.5, -2.5, -2.5),
            Gf.Vec3f(0.0, 0.0, 0.0),
            Gf.Vec3f(2.5, 2.5, 2.5),
        ]
        scales = [
            Gf.Vec3f(1.0),
            Gf.Vec3f(20.0),
            Gf.Vec3f(1.0),
        ]
        indices = [
            1,
            0,
            1,
        ]
        self._SetTransformComponentsAndIndices(self.instancer,
            positions=positions, scales=scales, indices=indices)

        expectedXforms = [
            Gf.Matrix4d(5.0,   0.0,  0.0, 0.0,
                        0.0,   5.0,  0.0, 0.0,
                        0.0,   0.0,  5.0, 0.0,
                        -2.5, -2.5, -2.5, 1.0),
            Gf.Matrix4d().SetScale(20.0),
            Gf.Matrix4d(5.0, 0.0, 0.0, 0.0,
                        0.0, 5.0, 0.0, 0.0,
                        0.0, 0.0, 5.0, 0.0,
                        2.5, 2.5, 2.5, 1.0),
        ]
        self._ValidateInstanceTransforms(self.instancer, expectedXforms)

        expectedExtent = [
            Gf.Vec3f(-10.0, -10.0, -10.0),
            Gf.Vec3f(10.0, 10.0, 10.0),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

        # Now de-activate some instances.
        self.instancer.DeactivateId(1)

        expectedXforms = [
            Gf.Matrix4d(5.0,   0.0,  0.0, 0.0,
                        0.0,   5.0,  0.0, 0.0,
                        0.0,   0.0,  5.0, 0.0,
                        -2.5, -2.5, -2.5, 1.0),
            Gf.Matrix4d(5.0, 0.0, 0.0, 0.0,
                        0.0, 5.0, 0.0, 0.0,
                        0.0, 0.0, 5.0, 0.0,
                        2.5, 2.5, 2.5, 1.0),
        ]
        self._ValidateInstanceTransforms(self.instancer, expectedXforms)

        expectedExtent = [
            Gf.Vec3f(-5.0, -5.0, -5.0),
            Gf.Vec3f(5.0, 5.0, 5.0),
        ]
        self._ValidateExtent(self.instancer, expectedExtent)

if __name__ == '__main__':
    unittest.main()
