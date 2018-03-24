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

# Maximum tolerated error between the values of extents during comparison.
EXTENT_TOLERANCE = 0.0001


def timeRange(time):
    """Iterate over all times within 1 unit of a given time in
    increments of 0.1 .
    """
    tr = []
    for i in range(-10, 11):
        delta = i * 0.1
        tr.append((time + delta, delta))
    return tr


class TestUsdGeomComputeAtTimeBase(object):
    """Base class for testing UsdGeomPointInstancer's
    ComputeInstanceTransformAtTime and ComputeExtentAtTime methods.
    """

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

    def assertAllMatrixListsEqual(self, lists1, lists2):
        for list1, list2 in zip(lists1, lists2):
            self.assertMatrixListsEqual(list1, list2)

    def assertExtentsEqual(self, ext1, ext2):
        try:
            # Each extent has 2 points: a min point and a max point. Each point
            # has x, y, and z dimensions which we compare between the given
            # extents.
            for i in range(2): # Loop over min and max.
                for d in range(3): # Loop over x, y, z dimensions.
                    self.assertTrue(
                        Gf.IsClose(ext1[i][d], ext2[i][d], EXTENT_TOLERANCE))
        except AssertionError:
            # Print a more descriptive message.
            raise AssertionError(
                "Extents not equal:\n{}\n{}".format(ext1, ext2))


    def computeInstanceTransforms(self, pi, tr, baseTime,
            xformInclusion=UsdGeom.IncludeProtoXform):
        """This method should be overridden to call both the single sample
        and multisample version of ComputeInstanceTransformAtTime.
        """
        raise NotImplementedError

    def test_NoInstances(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/NoInstances"))

        for baseTime, _ in timeRange(0):
            tr = timeRange(baseTime)
            xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
            for xforms in xformsArray:
                self.assertEqual(len(xforms), 0)

    def test_OneInstanceNoSamples(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceNoSamples"))

        baseTime = 1
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(1)] for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def test_OneInstanceNoVelocities(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceNoVelocities"))

        # Test directly on sample.
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = []
        for time, delta in tr:
            if time < 0:
                # Samples at times less than 0 should clamp to first sample.
                compare = [Gf.Matrix4d(1)]
            else:
                compare = [Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, time * 20))]
            compares.append(compare)
        self.assertAllMatrixListsEqual(xformsArray, compares)

        # Test in-between samples.
        baseTime = 2
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

        # Test with basetime before and after natural sample. Since we are
        # interpolating, these should always be the same.
        baseTime = 5
        tr = timeRange(baseTime)
        xformsArrayBefore = self.computeInstanceTransforms(pi, tr, baseTime - 1)
        xformsArrayAfter = self.computeInstanceTransforms(pi, tr, baseTime)
        self.assertAllMatrixListsEqual(xformsArrayBefore, xformsArrayAfter)

    def test_OneInstance(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstance"))

        # Test directly on sample.
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

        # Test in-between samples.
        baseTime = 2
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

        # Test with basetime before natural sample.
        baseTime = 5
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime - 1)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

        # Test with basetime on natural sample.
        baseTime = 5
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 - delta * 36),
                Gf.Vec3d(25 - delta * 5, 50 - delta * 10, 100 - delta * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def test_OneInstanceUnalignedData(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceUnalignedData"))

        # Test that unaligned positions/orientations are handled properly.
        baseTime = 3
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = []
        for time, delta in tr:
            rotationTime = time - 2
            velocityTime = time - 1
            compares.append([Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), rotationTime * 36),
                Gf.Vec3d(velocityTime * 5, velocityTime * 10, velocityTime * 20))])
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def test_OneInstanceVelocityScale(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceVelocityScale"))

        # Test when the velocityScale is set to 2.
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 72),
                Gf.Vec3d(time * 10, time * 20, time * 40))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def test_OneInstanceProtoXform(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceProtoXform"))

        # Test with prototype xforms (default).
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 + time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

        # Test without prototype xforms.
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(
            pi, tr, baseTime, UsdGeom.ExcludeProtoXform)
        compares = [[Gf.Matrix4d(
                Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                Gf.Vec3d(time * 5, time * 10, time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def test_MultiInstance(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/MultiInstance"))

        # Test with 3 instances.
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, time * 20)),
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, 1 + time * 20)),
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 - time * 36),
                    Gf.Vec3d(time * 5, time * 10, 2 + time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def test_Mask(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/MultiInstanceMask"))

        # Test with 3 instances with the second masked out.
        baseTime = 0
        tr = timeRange(baseTime)
        xformsArray = self.computeInstanceTransforms(pi, tr, baseTime)
        compares = [[
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), time * 36),
                    Gf.Vec3d(time * 5, time * 10, time * 20)),
                Gf.Matrix4d(
                    Gf.Rotation(Gf.Vec3d(0, 0, 1), 180 - time * 36),
                    Gf.Vec3d(time * 5, time * 10, 2 + time * 20))]
            for time, delta in tr]
        self.assertAllMatrixListsEqual(xformsArray, compares)

    def compareExtents(self, pi, times, baseTime, expectedExtents):
        """This method should be overridden to call both the single sample
        and multisample version of ComputeExtentAtTime, then compare the results
        to the expected extents.
        """
        raise NotImplementedError

    def test_Extent(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(
            stage.GetPrimAtPath("/MultiInstanceForExtents"))

        times = [0, 1, 2]
        expectedExtents = [
            [(-1, -1, -1), (1, 1, 1)],
            [(-3.7600734, 1.2399265, -1), (3.7600734, 6.2600737, 3.5)],
            [(-6.3968024, 3.6031978, -1), (6.3968024, 11.396802, 6)]
        ]
        self.compareExtents(pi, times, 0, expectedExtents)


class TestUsdGeomComputeAtTime(
        unittest.TestCase, TestUsdGeomComputeAtTimeBase):
    """Test UsdGeomPointInstancer's ComputeInstanceTransformAtTime and
    ComputeExtentAtTime methods."""

    def computeInstanceTransforms(self, pi, tr, baseTime,
            xformInclusion=UsdGeom.IncludeProtoXform):
        return [pi.ComputeInstanceTransformsAtTime(time, baseTime, xformInclusion)
                for time, delta in tr]

    def compareExtents(self, pi, times, baseTime, expectedExtents):
        for time, expectedExtent in zip(times, expectedExtents):
            computedExtent = pi.ComputeExtentAtTime(time, baseTime)
            self.assertExtentsEqual(computedExtent, expectedExtent)

    def test_NoInstancesDefault(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/NoInstances"))

        xforms = pi.ComputeInstanceTransformsAtTime(
            Usd.TimeCode.Default(), Usd.TimeCode.Default())
        self.assertEqual(len(xforms), 0)

    def test_OneInstanceNoSamplesDefault(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceNoSamples"))

        xforms = pi.ComputeInstanceTransformsAtTime(
            Usd.TimeCode.Default(), Usd.TimeCode.Default())
        compare = [Gf.Matrix4d(1)]
        self.assertMatrixListsEqual(xforms, compare)


class TestUsdGeomComputeAtTimeMultisampled(
        unittest.TestCase, TestUsdGeomComputeAtTimeBase):
    """Test the multisampling versions of UsdGeomPointInstancer's
    ComputeInstanceTransformAtTime and ComputeExtentAtTime methods.
    """

    def computeInstanceTransforms(self, pi, tr, baseTime,
            xformInclusion=UsdGeom.IncludeProtoXform):
        return pi.ComputeInstanceTransformsAtTimes(
            [time for time, delta in tr], baseTime, xformInclusion)

    def compareExtents(self, pi, times, baseTime, expectedExtents):
        computedExtents = pi.ComputeExtentAtTimes(times, baseTime)
        for computedExtent, expectedExtent in zip(
                computedExtents, expectedExtents):
            self.assertExtentsEqual(computedExtent, expectedExtent)

    def test_NoInstancesDefault(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/NoInstances"))

        xformsArray = pi.ComputeInstanceTransformsAtTimes(
            [Usd.TimeCode.Default()], Usd.TimeCode.Default())
        self.assertEqual(len(xformsArray), 1)
        self.assertEqual(len(xformsArray[0]), 0)

    def test_OneInstanceNoSamplesDefault(self):
        stage = Usd.Stage.Open("test.usda")
        pi = UsdGeom.PointInstancer(stage.GetPrimAtPath("/OneInstanceNoSamples"))

        xformsArray = pi.ComputeInstanceTransformsAtTimes(
            [Usd.TimeCode.Default()], Usd.TimeCode.Default())
        compares = [[Gf.Matrix4d(1)]]
        self.assertAllMatrixListsEqual(xformsArray, compares)


if __name__ == "__main__":
    unittest.main()
