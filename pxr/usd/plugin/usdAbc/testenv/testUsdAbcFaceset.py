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

from pxr import Usd, UsdAbc, UsdGeom, Gf
import unittest

class TestUsdAbcFaceset(unittest.TestCase):
    def test_RoundTrip(self):
        usdFile = 'original.usda'
        abcFile = 'converted.abc'
        self.assertTrue(UsdAbc._WriteAlembic(usdFile, abcFile))

        stage = Usd.Stage.Open(abcFile)
        prim = stage.GetPrimAtPath('/cube1')
        self.assertTrue(prim.IsValid())

        faceSets = UsdGeom.FaceSetAPI().GetFaceSets(prim)
        self.assertEqual(len(faceSets), 2)

        fs0 = faceSets[0]
        fs1 = faceSets[1]
        self.assertEqual(fs0.GetFaceSetName(), 'part1')
        self.assertEqual(fs1.GetFaceSetName(), 'part2')

        # Validate the face counts and values for part1 (which is animated)
        countsAttr = fs0.GetFaceCountsAttr()
        timeSamples = countsAttr.GetTimeSamples()
        expectedTimeSamples = [0.0, 1.0]
        for c, e in zip(timeSamples, expectedTimeSamples):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        expectedFaceCounts = {0.0: [3], 1.0: [2]}
        for time, expectedValue in expectedFaceCounts.items():
            faceCounts = countsAttr.Get(time)
            for c, e in zip(faceCounts, expectedValue):
                self.assertEqual(c, e)

        indicesAttr = fs0.GetFaceIndicesAttr()
        timeSamples = indicesAttr.GetTimeSamples()
        expectedTimeSamples = [0.0, 1.0]
        for c, e in zip(timeSamples, expectedTimeSamples):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        expectedFaceIndices = {0.0: [0, 1, 4], 1.0: [0, 1]}
        for time, expectedValue in expectedFaceIndices.items():
            faceIndices = indicesAttr.Get(time)
            for c, e in zip(faceIndices, expectedValue):
                self.assertEqual(c, e)

        # Validate the face counts and values for part2 (which was constant,
        # but promoted to the same samples as part1 during conversion)
        countsAttr = fs1.GetFaceCountsAttr()
        timeSamples = countsAttr.GetTimeSamples()
        expectedTimeSamples = [0.0, 1.0]
        for c, e in zip(timeSamples, expectedTimeSamples):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        expectedFaceCounts = {0.0: [3], 1.0: [3]}
        for time, expectedValue in expectedFaceCounts.items():
            faceCounts = countsAttr.Get(time)
            for c, e in zip(faceCounts, expectedValue):
                self.assertEqual(c, e)

        indicesAttr = fs1.GetFaceIndicesAttr()
        expectedTimeSamples = [0.0, 1.0]
        for c, e in zip(timeSamples, expectedTimeSamples):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        expectedFaceIndices = {0.0: [2, 3, 5], 1.0: [2, 3, 5]}
        for time, expectedValue in expectedFaceIndices.items():
            faceIndices = indicesAttr.Get(time)
            for c, e in zip(faceIndices, expectedValue):
                self.assertEqual(c, e)

if __name__ == '__main__':
    unittest.main()
