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


# Maximum tolerated error between the values of a matrix during comparison.
MATRIX_TOLERANCE = 0.01


def timeRange(time):
    """Iterate over all times within 1 unit of a given time in
    increments of 0.1 .
    """
    for i in range(-10, 11):
        delta = i * 0.1
        yield time + delta, delta


class TestUsdGeomComputeInstanceTransformAtTime(unittest.TestCase):

    def assertMatrixListsEqual(self, list1, list2):
        """Assert that two Gf.Matrix4d objects are equal."""
        self.assertEqual(len(list1), len(list2))
        for matrix1, matrix2 in zip(list1, list2):
            try:
                for row1, row2 in zip(matrix1, matrix2):
                    self.assertTrue(Gf.IsClose(row1, row2, MATRIX_TOLERANCE))
            except AssertionError:
                # Print a more descriptive message.
                raise AssertionError(
                    "Matrices not equal:\n{}\n{}".format(matrix1, matrix2))

    def test_NoInstances(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/NoInstances"))

        xforms = pi.ComputeInstanceTransformsAtTime(
            Usd.TimeCode.Default(), Usd.TimeCode.Default())

        for baseTime, _ in timeRange(0):
            for time, _ in timeRange(0):
                xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
                self.assertEqual(len(xforms), 0)

    def test_OneInstanceNoSamples(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceNoSamples"))

        xforms = pi.ComputeInstanceTransformsAtTime(
            Usd.TimeCode.Default(), Usd.TimeCode.Default())
        compare = [Gf.Matrix4d(1)]
        self.assertMatrixListsEqual(xforms, compare)

        xforms = pi.ComputeInstanceTransformsAtTime(1, 1)
        compare = [Gf.Matrix4d(1)]
        self.assertMatrixListsEqual(xforms, compare)

    def test_OneInstanceNoVelocities(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceNoVelocities"))

        # Test directly on sample.
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            if time < 0:
                # Samples at times less than 0 should clamp to first sample.
                compare = [Gf.Matrix4d(1)]
            else:
                compare = [Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

        # Test in-between samples.
        baseTime = 2
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

        # Test with basetime before and after natural sample. Since we are
        # interpolating, these should always be the same.
        baseTime = 5
        for time, delta in timeRange(baseTime):
            xformsBefore = pi.ComputeInstanceTransformsAtTime(time, baseTime - 1)
            xformsAfter = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            self.assertMatrixListsEqual(xformsBefore, xformsAfter)

    def test_OneInstance(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstance"))

        # Test directly on sample.
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

        # Test in-between samples.
        baseTime = 2
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

        # Test with basetime before natural sample.
        baseTime = 5
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime - 1)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

        # Test with basetime on natural sample.
        baseTime = 5
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 - delta * 36),
                Gf.Vec3d(25 - delta * 5, 50 - delta * 10, 100 - delta * 20))]
            self.assertMatrixListsEqual(xforms, compare)

    def test_OneInstanceUnalignedData(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceUnalignedData"))

        # Test that unaligned positions/orientations are handled properly.
        baseTime = 3
        for time, delta in timeRange(baseTime):
            rotationTime = time - 2
            velocityTime = time - 1
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), rotationTime * 36),
                Gf.Vec3d(velocityTime * 5, velocityTime * 10, velocityTime * 20))]
            self.assertMatrixListsEqual(xforms, compare)

    def test_OneInstanceVelocityScale(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceVelocityScale"))

        # Test when the velocityScale is set to 2.
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 72),
                Gf.Vec3d(time * 10, time * 20, time * 40))]
            self.assertMatrixListsEqual(xforms, compare)

    def test_OneInstanceProtoXform(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceProtoXform"))

        # Test with prototype xforms (default).
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 + time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

        # Test without prototype xforms.
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(
                time, baseTime, UsdGeom.ExcludeProtoXform)
            compare = [Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

    def test_MultiInstance(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/MultiInstance"))

        # Test with 3 instances.
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, time * 20)),
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, 1 + time * 20)),
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 - time * 36),
                    Gf.Vec3d(time * 5, time * 10, 2 + time * 20))]
            self.assertMatrixListsEqual(xforms, compare)

    def test_Mask(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/MultiInstanceMask"))

        # Test with 3 instances with the second masked out.
        baseTime = 0
        for time, delta in timeRange(baseTime):
            xforms = pi.ComputeInstanceTransformsAtTime(time, baseTime)
            compare = [
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, time * 20)),
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 - time * 36),
                    Gf.Vec3d(time * 5, time * 10, 2 + time * 20))]
            self.assertMatrixListsEqual(xforms, compare)


if __name__ == "__main__":
    unittest.main()
