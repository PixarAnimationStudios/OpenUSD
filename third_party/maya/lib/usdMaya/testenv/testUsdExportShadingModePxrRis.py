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

from maya import cmds
from maya import standalone


class testUsdExportShadingModePxrRis(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('MarbleCube.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('MarbleCube.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='pxrRis')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def testExportPxrRisShading(self):
        """
        Tests that exporting a Maya mesh with a simple Maya shading setup
        results in the correct shading on the USD mesh.
        """
        cubePrim = self._stage.GetPrimAtPath('/MarbleCube/Geom/Cube')
        self.assertTrue(cubePrim)

        # Validate the Material prim bound to the Mesh prim.
        material = UsdShade.Material.GetBoundMaterial(cubePrim)
        self.assertTrue(material)
        materialPath = material.GetPath().pathString
        self.assertEqual(materialPath, '/MarbleCube/Materials/MarbleCubeSG')

        # Validate the surface shader that is connected to the material.
        materialOutputs = material.GetOutputs()
        self.assertEqual(len(materialOutputs), 4)
        print self._stage.ExportToString()
        materialOutput = material.GetOutput('ri:surface')
        (connectableAPI, outputName, outputType) = materialOutput.GetConnectedSource()
        self.assertEqual(outputName, 'out')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        # Validate the connected input on the surface shader.
        shaderInput = shader.GetInput('placementMatrix')
        self.assertTrue(shaderInput)

        (connectableAPI, outputName, outputType) = shaderInput.GetConnectedSource()
        self.assertEqual(outputName, 'worldInverseMatrix')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaPlacement3d')

    def testShaderAttrsAuthoredSparsely(self):
        """
        Tests that only the attributes authored in Maya are exported to USD.
        """
        shaderPrimPath = '/MarbleCube/Materials/MarbleCubeSG/MarbleShader'
        shaderPrim = self._stage.GetPrimAtPath(shaderPrimPath)
        self.assertTrue(shaderPrim)

        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrMayaMarble')

        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 3)

        inputOutAlpha = shader.GetInput('outAlpha').Get()
        self.assertTrue(Gf.IsClose(inputOutAlpha, 0.0894, 1e-6))

        inputOutColor = shader.GetInput('outColor').Get()
        self.assertTrue(Gf.IsClose(inputOutColor, Gf.Vec3f(0.298, 0.0, 0.0), 1e-6))

        inputPlacementMatrix = shader.GetInput('placementMatrix')
        (connectableAPI, outputName, outputType) = inputPlacementMatrix.GetConnectedSource()
        self.assertEqual(connectableAPI.GetPath().pathString,
            '/MarbleCube/Materials/MarbleCubeSG/MarbleCubePlace3dTexture')

        shaderOutputs = shader.GetOutputs()
        self.assertEqual(len(shaderOutputs), 1)


if __name__ == '__main__':
    unittest.main(verbosity=2)
