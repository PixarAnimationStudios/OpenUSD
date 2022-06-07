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
from pxr import Usd, UsdAbc, UsdGeom
import tempfile, unittest

class TestUsdAbcUvWrite(unittest.TestCase):
    def test_Write(self):
        with tempfile.NamedTemporaryFile(suffix='.abc') as tempAbcFile:
            tempAbcFile.close()
            testFile = 'testUsdAbcUvReadWrite.usda'

            planeStPath = '/pPlaneSt'
            planeUvPath = '/pPlaneUv'
            planeStUvPath = '/pPlaneStUv'

            UsdAbc._WriteAlembic(testFile, tempAbcFile.name)
            stage = Usd.Stage.Open(testFile)
            self.assertTrue(stage)
            roundStage = Usd.Stage.Open(tempAbcFile.name)
            self.assertTrue(roundStage)

            planeSt = UsdGeom.Mesh.Get(stage, planeStPath)
            planeUv = UsdGeom.Mesh.Get(stage, planeUvPath)
            planeStUv = UsdGeom.Mesh.Get(stage, planeStUvPath)

            self.assertTrue(planeSt)
            self.assertTrue(planeUv)
            self.assertTrue(planeStUv)

            rplaneSt = UsdGeom.Mesh.Get(roundStage, planeStPath)
            rplaneUv = UsdGeom.Mesh.Get(roundStage, planeUvPath)
            rplaneStUv = UsdGeom.Mesh.Get(roundStage, planeStUvPath)

            self.assertTrue(rplaneSt)
            self.assertTrue(rplaneUv)
            self.assertTrue(rplaneStUv)

            planeSt_pvAPI = UsdGeom.PrimvarsAPI(planeSt)
            planeUv_pvAPI = UsdGeom.PrimvarsAPI(planeUv)
            planeStUv_pvAPI = UsdGeom.PrimvarsAPI(planeStUv)
            rplaneSt_pvAPI = UsdGeom.PrimvarsAPI(rplaneSt)
            rplaneUv_pvAPI = UsdGeom.PrimvarsAPI(rplaneUv)
            rplaneStUv_pvAPI = UsdGeom.PrimvarsAPI(rplaneStUv)

            self.assertEqual(planeSt_pvAPI.GetPrimvar('st').GetTypeName(), 'texCoord2f[]')
            self.assertEqual(rplaneSt_pvAPI.GetPrimvar('st').GetTypeName(), 'texCoord2f[]')
            self.assertEqual(planeSt_pvAPI.GetPrimvar('st').Get(), rplaneSt_pvAPI.GetPrimvar('st').Get(0))
            self.assertEqual(planeSt_pvAPI.GetPrimvar('st').GetIndices(), rplaneSt_pvAPI.GetPrimvar('st').GetIndices(0))

            self.assertEqual(planeUv_pvAPI.GetPrimvar('uv').GetTypeName(), 'texCoord2f[]')
            self.assertEqual(rplaneUv_pvAPI.GetPrimvar('st').GetTypeName(), 'texCoord2f[]')
            self.assertEqual(planeUv_pvAPI.GetPrimvar('uv').Get(), rplaneUv_pvAPI.GetPrimvar('st').Get(0))
            self.assertEqual(planeUv_pvAPI.GetPrimvar('uv').GetIndices(), rplaneUv_pvAPI.GetPrimvar('st').GetIndices(0))

            self.assertEqual(planeStUv_pvAPI.GetPrimvar('st').GetTypeName(), 'texCoord2f[]')
            self.assertEqual(rplaneStUv_pvAPI.GetPrimvar('st').GetTypeName(), 'texCoord2f[]')
            self.assertEqual(planeStUv_pvAPI.GetPrimvar('st').Get(), rplaneStUv_pvAPI.GetPrimvar('st').Get(0))
            self.assertEqual(planeStUv_pvAPI.GetPrimvar('st').GetIndices(), rplaneStUv_pvAPI.GetPrimvar('st').GetIndices(0))

            del stage
            del roundStage

if __name__ == '__main__':
    unittest.main()
