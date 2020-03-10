#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
from pxr import Usd, UsdGeom
import unittest

class TestUsdAbcIsConstant(unittest.TestCase):
    def test_Read(self):
      testFile = 'testUsdAbcIsConstant.abc'
      stage = Usd.Stage.Open(testFile)
      self.assertTrue(stage)

      animPoints = UsdGeom.Mesh.Get(stage, "/sphere_object1")
      self.assertTrue(animPoints)
      self.assertEqual(animPoints.GetPrim().GetProperty(UsdGeom.Tokens.points).GetNumTimeSamples(), 240)
      self.assertEqual(animPoints.GetOrderedXformOps()[0].GetNumTimeSamples(), 1)

      animXform = UsdGeom.Mesh.Get(stage, "/sphere_object2")
      self.assertTrue(animXform)
      self.assertEqual(animXform.GetPrim().GetProperty(UsdGeom.Tokens.points).GetNumTimeSamples(), 1)
      self.assertEqual(animXform.GetOrderedXformOps()[0].GetNumTimeSamples(), 240)

if __name__ == '__main__':
    unittest.main()
