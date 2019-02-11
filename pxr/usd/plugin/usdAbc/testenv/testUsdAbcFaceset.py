#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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
        prim1 = stage.GetPrimAtPath('/cube1')
        self.assertTrue(prim1.IsValid())

        faceset1prim1 = stage.GetPrimAtPath('/cube1/faceset1')
        faceset2prim1 = stage.GetPrimAtPath('/cube1/faceset2')
        self.assertTrue(faceset1prim1.IsValid())
        self.assertTrue(faceset2prim1.IsValid())

        faceset1_1 = UsdGeom.Subset(faceset1prim1)
        faceset2_1 = UsdGeom.Subset(faceset2prim1)

        self.assertEqual(faceset1_1.GetElementTypeAttr().Get(), 'face')
        self.assertEqual(faceset2_1.GetElementTypeAttr().Get(), 'face')

        # Validate the indices for faceset1 (which is animated)
        indices = faceset1_1.GetIndicesAttr()

        timeSamples = indices.GetTimeSamples()
        expectedTimeSamples = [0.0, 1.0]

        self.assertEqual(len(timeSamples), len(expectedTimeSamples))

        for c, e in zip(timeSamples, expectedTimeSamples):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        expectedFaceIndices = {0.0: [0, 3, 5], 1.0: [3]}
        for time, expectedValue in expectedFaceIndices.iteritems():
            faceIndices = indices.Get(time)
            for c, e in zip(faceIndices, expectedValue):
                self.assertEqual(c, e)

        # Validate the indices for faceset2.
        indices = faceset2_1.GetIndicesAttr()

        timeSamples = indices.GetTimeSamples()

        self.assertEqual(len(timeSamples), 0)

        expectedFaceIndices = {0.0: [1, 2, 4]}
        for time, expectedValue in expectedFaceIndices.items():
            faceIndices = indices.Get(time)
            for c, e in zip(faceIndices, expectedValue):
                self.assertEqual(c, e)

        # Initialize checks for /cube2
        prim2 = stage.GetPrimAtPath('/cube2')
        self.assertTrue(prim2.IsValid())
        faceset1prim2 = stage.GetPrimAtPath('/cube2/faceset1')
        faceset2prim2 = stage.GetPrimAtPath('/cube2/faceset2')
        faceset3prim2 = stage.GetPrimAtPath('/cube2/faceset3')
        self.assertTrue(faceset1prim2.IsValid())
        self.assertTrue(faceset2prim2.IsValid())
        self.assertTrue(faceset3prim2.IsValid())

        faceset1_2 = UsdGeom.Subset(faceset1prim2)
        faceset2_2 = UsdGeom.Subset(faceset2prim2)
        faceset3_2 = UsdGeom.Subset(faceset3prim2)

        # Initialize checks for /cube3
        prim3 = stage.GetPrimAtPath('/cube3')
        self.assertTrue(prim3.IsValid())
        faceset1prim3 = stage.GetPrimAtPath('/cube3/faceset')
        self.assertTrue(faceset1prim3.IsValid())
        faceset1_3 = UsdGeom.Subset(faceset1prim3)

        # Check the round-tripping of familyTypes.
        imageable1 = UsdGeom.Imageable(prim1)
        imageable2 = UsdGeom.Imageable(prim2)
        imageable3 = UsdGeom.Imageable(prim3)

        # In cube1 we've used "partition" as the familyType. This should be
        # converted to "nonOverlapping" as it more closely matches the Alembic
        # definition of "partition".
        self.assertEqual(faceset1_1.GetFamilyNameAttr().Get(), 'materialBind')
        self.assertEqual(faceset2_1.GetFamilyNameAttr().Get(), 'materialBind')
        self.assertEqual(UsdGeom.Subset.GetFamilyType(imageable1, "materialBind"),
                         "nonOverlapping")

        # In cube2 we've used "unrestricted". This should come across directly.
        self.assertEqual(faceset1_2.GetFamilyNameAttr().Get(), 'materialBind')
        self.assertEqual(faceset2_2.GetFamilyNameAttr().Get(), 'materialBind')
        self.assertEqual(UsdGeom.Subset.GetFamilyType(imageable2, "materialBind"),
                         "unrestricted")
        # We've also added another familyName in addition. We should see this
        # being lost in the round-trip and converted to "materialBind".
        self.assertEqual(faceset3_2.GetFamilyNameAttr().Get(), 'materialBind')

        # In cube3, no familyName or familyType has been specified. Upon
        # round-tripping, a default value of "unrestricted" will appear on the
        # the default "materialBind" familyName.
        self.assertEqual(faceset1_3.GetFamilyNameAttr().Get(), 'materialBind')
        self.assertEqual(UsdGeom.Subset.GetFamilyType(imageable3, "materialBind"),
                         "unrestricted")

if __name__ == '__main__':
    unittest.main()
