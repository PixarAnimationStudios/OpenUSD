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

        pCubeShape1ST = pCubeShape1.GetPrimvar('st')
        pCubeShape2ST = pCubeShape2.GetPrimvar('st')
        
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

        self.assertEqual(UsdGeom.Mesh.Get(flatABC, '/pCubeShape1').GetPrimvar('st').Get(time), pCubeShape1ST.Get(time))
        self.assertEqual(UsdGeom.Mesh.Get(flatABC, '/pCubeShape2').GetPrimvar('st').Get(time), pCubeShape2ST.Get(time))

if __name__ == '__main__':
    unittest.main()
