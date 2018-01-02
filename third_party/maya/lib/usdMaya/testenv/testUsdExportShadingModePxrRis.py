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

    def testShaderAttrsAuthoredSparsely(self):
        """
        Tests that only the attributes authored in Maya are exported to USD.
        """
        shaderPrimPath = '/MarbleCube/Looks/MarbleCubeSG/MarbleShader'
        shaderPrim = self._stage.GetPrimAtPath(shaderPrimPath)
        self.assertTrue(shaderPrim)

        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)

        # XXX: Change this to use shader.GetIdAttr() when we transition from
        # UsdRi to UsdShade.
        shaderId = shaderPrim.GetAttribute('info:filePath').Get()
        self.assertEqual(shaderId.path, 'PxrMayaMarble')

        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 3)

        inputOutAlpha = shader.GetInput('outAlpha').Get()
        self.assertTrue(Gf.IsClose(inputOutAlpha, 0.0894, 1e-6))

        inputOutColor = shader.GetInput('outColor').Get()
        self.assertTrue(Gf.IsClose(inputOutColor, Gf.Vec3f(0.298, 0.0, 0.0), 1e-6))

        inputPlacementMatrix = shader.GetInput('placementMatrix')
        (connectableAPI, outputName, outputType) = inputPlacementMatrix.GetConnectedSource()
        self.assertEqual(connectableAPI.GetPath().pathString,
            '/MarbleCube/Looks/MarbleCubeSG/MarbleCubePlace3dTexture')


if __name__ == '__main__':
    unittest.main(verbosity=2)
