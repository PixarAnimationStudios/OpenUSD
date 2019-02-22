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
#

import os
import unittest

from pxr import Gf
from pxr import Usd
from pxr import UsdShade

from maya import OpenMaya
from maya import cmds
from maya import standalone


class testUsdImportShadingModePxrRis(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(new=True, force=True)

        # Import from USD.
        usdFilePath = os.path.abspath('MarbleCube.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdImport(file=usdFilePath, shadingMode='pxrRis')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage being imported opens successfully.
        """
        self.assertTrue(self._stage)

    def _GetMayaMesh(self, meshName):
         selectionList = OpenMaya.MSelectionList()
         selectionList.add(meshName)
         mObj = OpenMaya.MObject()
         selectionList.getDependNode(0, mObj)

         meshFn = OpenMaya.MFnMesh(mObj)
         self.assertTrue(meshFn)

         return meshFn

    def testImportPxrRisShading(self):
        """
        Tests that importing a USD Mesh prim with a simple shading setup
        results in the correct shading on the Maya mesh.
        """
        meshFn = self._GetMayaMesh('|MarbleCube|Geom|Cube|CubeShape')

        self.assertTrue('MarbleShader' in cmds.listConnections('defaultShaderList1.s'))

        # Validate the shadingEngine connected to the mesh.
        objectArray = OpenMaya.MObjectArray()
        indexArray = OpenMaya.MIntArray()
        meshFn.getConnectedShaders(0, objectArray, indexArray)
        self.assertEqual(objectArray.length(), 1)

        shadingEngineObj = objectArray[0]
        shadingEngineDepFn = OpenMaya.MFnDependencyNode(shadingEngineObj)
        self.assertEqual(shadingEngineDepFn.name(),
            'USD_Materials:MarbleCubeSG')

        # Validate the shader plugged in as the surfaceShader of the
        # shadingEngine.
        surfaceShaderPlug = shadingEngineDepFn.findPlug('surfaceShader', True)
        plugArray = OpenMaya.MPlugArray()
        surfaceShaderPlug.connectedTo(plugArray, True, False)
        self.assertEqual(plugArray.length(), 1)

        shaderObj = plugArray[0].node()
        self.assertTrue(shaderObj.hasFn(OpenMaya.MFn.kMarble))
        shaderDepFn = OpenMaya.MFnDependencyNode(shaderObj)
        self.assertEqual(shaderDepFn.name(), 'MarbleShader')

        # Validate the shader that is connected to an input of the surface
        # shader.
        placementMatrixPlug = shaderDepFn.findPlug('placementMatrix', True)
        plugArray = OpenMaya.MPlugArray()
        placementMatrixPlug.connectedTo(plugArray, True, False)
        self.assertEqual(plugArray.length(), 1)

        placement3dObj = plugArray[0].node()
        self.assertTrue(placement3dObj.hasFn(OpenMaya.MFn.kPlace3dTexture))
        placement3dDepFn = OpenMaya.MFnDependencyNode(placement3dObj)
        self.assertEqual(placement3dDepFn.name(), 'MarbleCubePlace3dTexture')


if __name__ == '__main__':
    unittest.main(verbosity=2)
