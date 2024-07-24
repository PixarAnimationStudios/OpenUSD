#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from pxr import Usd, UsdGeom
import unittest

class TestUsdAbcSDFArguments(unittest.TestCase):
    def test_Layers(self):
        layeredFile = 'testUsdAbcSDFArguments.usda'
        topologyFile = 'testUsdAbcSDFArgumentsMesh.abc'
        flatFile = 'testUsdAbcSDFArgumentsFlat.abc'
        time = Usd.TimeCode()
        xformCache = UsdGeom.XformCache(time)

        stage = Usd.Stage.Open(layeredFile)
        self.assertTrue(stage)

        pCubeShape1 = UsdGeom.Mesh.Get(stage, '/AlembicRoot/pCubeShape1')
        pCubeShape2 = UsdGeom.Mesh.Get(stage, '/AlembicRoot/pCubeShape2')
        self.assertTrue(pCubeShape1)
        self.assertTrue(pCubeShape2)


        topoABC = Usd.Stage.Open(topologyFile)
        self.assertTrue(topoABC)
        pCubeShape1ABCPure = UsdGeom.Mesh.Get(topoABC, '/pCubeShape1')
        pCubeShape2ABCPure = UsdGeom.Mesh.Get(topoABC, '/pCubeShape2')
        self.assertTrue(pCubeShape1ABCPure)
        self.assertTrue(pCubeShape2ABCPure)

        # Test the Xforms got layered in properly
        #
        self.assertEqual(xformCache.GetLocalToWorldTransform(pCubeShape1.GetPrim()),
                         xformCache.GetLocalToWorldTransform(pCubeShape1ABCPure.GetPrim()))

        self.assertEqual(xformCache.GetLocalToWorldTransform(pCubeShape2.GetPrim()),
                         xformCache.GetLocalToWorldTransform(pCubeShape2ABCPure.GetPrim()))

        # Test various attributes & primavars came through
        #
        self.assertEqual(len(pCubeShape1.GetPointsAttr().Get(time)),
                         len(pCubeShape2.GetPointsAttr().Get(time)))

        self.assertEqual(len(pCubeShape1.GetNormalsAttr().Get(time)), 8)
        self.assertEqual(len(pCubeShape2.GetNormalsAttr().Get(time)), 24)

        pCubeShape1ST = UsdGeom.PrimvarsAPI(pCubeShape1).GetPrimvar('st')
        pCubeShape2ST = UsdGeom.PrimvarsAPI(pCubeShape2).GetPrimvar('st')
        
        self.assertEqual(pCubeShape1ST.GetTypeName(), 'texCoord2f[]')
        self.assertEqual(pCubeShape2ST.GetTypeName(), 'texCoord2f[]')

        self.assertEqual(pCubeShape1ST.GetInterpolation() , 'varying')
        self.assertEqual(len(pCubeShape1ST.Get(time)),  8)

        self.assertEqual(pCubeShape2ST.GetInterpolation(), 'faceVarying')
        self.assertEqual(len(pCubeShape2ST.Get(time)), 14)
        self.assertEqual(len(pCubeShape2ST.GetIndices(time)), 24)

        # Test against the known flattened version
        #
        flatABC = Usd.Stage.Open(flatFile)
        self.assertTrue(flatABC)

        flat_pCubeShape1PvAPI = UsdGeom.PrimvarsAPI(UsdGeom.Mesh.Get(flatABC, '/pCubeShape1'))
        flat_pCubeShape2PvAPI = UsdGeom.PrimvarsAPI(UsdGeom.Mesh.Get(flatABC, '/pCubeShape2'))
        self.assertEqual(flat_pCubeShape1PvAPI.GetPrimvar('st').Get(time), pCubeShape1ST.Get(time))
        self.assertEqual(flat_pCubeShape2PvAPI.GetPrimvar('st').Get(time), pCubeShape2ST.Get(time))

if __name__ == '__main__':
    unittest.main()
