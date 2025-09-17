#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Usd, UsdAbc, UsdGeom
import tempfile, unittest

class TestUsdAbcUvWrite(unittest.TestCase):
    def test_Write(self):
        with tempfile.NamedTemporaryFile(suffix='.abc') as tempAbcFile:
            tempAbcFile.close()
            testFile = 'testUsdAbcUvReadWrite_OldEncoding.usda'

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

            self.assertEqual(planeSt_pvAPI.GetPrimvar('st').GetTypeName(), 'float2[]') 
            self.assertEqual(rplaneSt_pvAPI.GetPrimvar('uv').GetTypeName(), 'float2[]')
            self.assertEqual(planeSt_pvAPI.GetPrimvar('st').Get(), rplaneSt_pvAPI.GetPrimvar('uv').Get(0))
            self.assertEqual(planeSt_pvAPI.GetPrimvar('st').GetIndices(), rplaneSt_pvAPI.GetPrimvar('uv').GetIndices(0))

            self.assertEqual(planeUv_pvAPI.GetPrimvar('uv').GetTypeName(), 'float2[]') 
            self.assertEqual(rplaneUv_pvAPI.GetPrimvar('uv').GetTypeName(), 'float2[]')
            self.assertEqual(planeUv_pvAPI.GetPrimvar('uv').Get(), rplaneUv_pvAPI.GetPrimvar('uv').Get(0))
            self.assertEqual(planeUv_pvAPI.GetPrimvar('uv').GetIndices(), rplaneUv_pvAPI.GetPrimvar('uv').GetIndices(0))

            self.assertEqual(planeStUv_pvAPI.GetPrimvar('st').GetTypeName(), 'float2[]') 
            self.assertEqual(rplaneStUv_pvAPI.GetPrimvar('uv').GetTypeName(), 'float2[]')
            self.assertEqual(planeStUv_pvAPI.GetPrimvar('st').Get(), rplaneStUv_pvAPI.GetPrimvar('uv').Get(0))
            self.assertEqual(planeStUv_pvAPI.GetPrimvar('st').GetIndices(), rplaneStUv_pvAPI.GetPrimvar('uv').GetIndices(0))

            del stage
            del roundStage

if __name__ == '__main__':
    unittest.main()
