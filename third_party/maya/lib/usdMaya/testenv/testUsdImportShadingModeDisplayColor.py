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


class testUsdImportShadingModeDisplayColor(unittest.TestCase):

    RED_COLOR = Gf.Vec3f(1.0, 0.0, 0.0)

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(new=True, force=True)

        # Import from USD.
        usdFilePath = os.path.abspath('RedCube.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdImport(file=usdFilePath, shadingMode='displayColor')

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

    def testImportDisplayColorShading(self):
        """
        Tests that importing a USD Mesh prim with a simple shading setup
        results in the correct shading on the Maya mesh.
        """
        meshFn = self._GetMayaMesh('|RedCube|Geom|Cube|CubeShape')

        # Validate the shadingEngine connected to the mesh.
        objectArray = OpenMaya.MObjectArray()
        indexArray = OpenMaya.MIntArray()
        meshFn.getConnectedShaders(0, objectArray, indexArray)
        self.assertEqual(objectArray.length(), 1)

        shadingEngineObj = objectArray[0]
        shadingEngineDepFn = OpenMaya.MFnDependencyNode(shadingEngineObj)
        self.assertEqual(shadingEngineDepFn.name(),
            'USD_Materials:RedLambertSG')

        # Validate the lambert plugged in as the surfaceShader of the
        # shadingEngine.
        surfaceShaderPlug = shadingEngineDepFn.findPlug('surfaceShader', True)
        plugArray = OpenMaya.MPlugArray()
        surfaceShaderPlug.connectedTo(plugArray, True, False)
        self.assertEqual(plugArray.length(), 1)

        shaderObj = plugArray[0].node()
        shaderLambertFn = OpenMaya.MFnLambertShader(shaderObj)
        self.assertEqual(shaderLambertFn.name(),
            'RedLambertSG_lambert')

        mayaColor = shaderLambertFn.color()
        mayaColor = Gf.Vec3f(mayaColor.r, mayaColor.g, mayaColor.b)
        self.assertTrue(Gf.IsClose(mayaColor, self.RED_COLOR, 1e-6))


if __name__ == '__main__':
    unittest.main(verbosity=2)
