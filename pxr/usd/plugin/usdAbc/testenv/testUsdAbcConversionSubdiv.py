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

class TestUsdAbcConversionSubdiv(unittest.TestCase):
    def test_RoundTrip(self):
        usdFile = 'original.usda'
        abcFile = 'converted.abc'

        self.assertTrue(UsdAbc._WriteAlembic(usdFile, abcFile))

        origStage = Usd.Stage.Open(usdFile)
        stage = Usd.Stage.Open(abcFile)

        prim = stage.GetPrimAtPath('/World/geom/CenterCross/UpLeft')
        creaseIndices = prim.GetAttribute('creaseIndices').Get()
        expectedCreaseIndices = [0, 1, 3, 2, 0, 4, 5, 7, 
                                 6, 4, 1, 5, 0, 4, 2, 6, 3, 7]
        for c, e in zip(creaseIndices, expectedCreaseIndices):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        creaseLengths = prim.GetAttribute('creaseLengths').Get()
        expectedCreaseLengths = [5, 5, 2, 2, 2, 2]
        for c, e in zip(creaseLengths, expectedCreaseLengths):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        creaseSharpnesses = prim.GetAttribute('creaseSharpnesses').Get()
        expectedCreaseSharpness = [1000, 1000, 1000, 1000, 1000, 1000]
        for c, e in zip(creaseSharpnesses, expectedCreaseSharpness):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        faceVertexCounts = prim.GetAttribute('faceVertexCounts').Get()
        expectedFaceVertexCounts = [4, 4, 4, 4, 4, 4]
        for c, e in zip(faceVertexCounts, expectedFaceVertexCounts):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        # The writer will revrse the orientation because alembic only supports
        # left handed winding order.
        faceVertexIndices = prim.GetAttribute('faceVertexIndices').Get()
        expectedFaceVertexIndices = [2, 6, 4, 0, 4, 5, 1, 0, 6, 7, 5, 
                                     4, 1, 5, 7, 3, 2, 3, 7, 6, 0, 1, 3, 2]
        for c, e in zip(faceVertexIndices, expectedFaceVertexIndices):
            self.assertTrue(Gf.IsClose(c, e, 1e-5))

        # Check layer/stage metadata transfer
        self.assertEqual(origStage.GetDefaultPrim().GetPath(),
                    stage.GetDefaultPrim().GetPath())
        self.assertEqual(origStage.GetTimeCodesPerSecond(),
                    stage.GetTimeCodesPerSecond())
        self.assertEqual(origStage.GetFramesPerSecond(),
                    stage.GetFramesPerSecond())
        self.assertEqual(origStage.GetStartTimeCode(),
                    stage.GetStartTimeCode())
        self.assertEqual(origStage.GetEndTimeCode(),
                    stage.GetEndTimeCode())
        self.assertEqual(UsdGeom.GetStageUpAxis(origStage),
                    UsdGeom.GetStageUpAxis(stage))
        

if __name__ == '__main__':
    unittest.main()
