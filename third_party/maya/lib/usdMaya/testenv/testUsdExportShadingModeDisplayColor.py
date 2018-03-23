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
from pxr import UsdGeom
from pxr import UsdShade

from maya import cmds
from maya import standalone


class testUsdExportShadingModeDisplayColor(unittest.TestCase):

    RED_COLOR = Gf.Vec3f(1.0, 0.0, 0.0)

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        mayaFile = os.path.abspath('RedCube.ma')
        cmds.file(mayaFile, open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('RedCube.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='displayColor')

        cls._stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)

    def testExportDisplayColorShading(self):
        """
        Tests that exporting a Maya mesh with a simple Maya shading setup
        results in the correct shading on the USD mesh.
        """
        # Validate the displayColor on the mesh prim.
        cubePrim = self._stage.GetPrimAtPath('/RedCube/Geom/Cube')
        self.assertTrue(cubePrim)

        cubeMesh = UsdGeom.Mesh(cubePrim)
        self.assertTrue(cubeMesh)

        meshDisplayColors = cubeMesh.GetDisplayColorPrimvar().Get()
        self.assertEqual(len(meshDisplayColors), 1)
        self.assertTrue(Gf.IsClose(meshDisplayColors[0], self.RED_COLOR, 1e-6))

        # Validate the Material prim bound to the Mesh prim.
        material = UsdShade.Material.GetBoundMaterial(cubePrim)
        self.assertTrue(material)
        materialPath = material.GetPath().pathString
        self.assertEqual(materialPath, '/RedCube/Looks/RedLambertSG')

        materialInputs = material.GetInputs()
        self.assertEqual(len(materialInputs), 3)

        materialInput = material.GetInput('displayColor')
        matDisplayColor = materialInput.Get()
        self.assertTrue(Gf.IsClose(matDisplayColor, self.RED_COLOR, 1e-6))

        # Just verify that displayOpacity and transparency exist.
        materialInput = material.GetInput('displayOpacity')
        self.assertTrue(materialInput)

        materialInput = material.GetInput('transparency')
        self.assertTrue(materialInput)

        # Validate the surface shader that is connected to the material.
        materialOutputs = material.GetOutputs()
        self.assertEqual(len(materialOutputs), 4)
        print self._stage.ExportToString()
        materialOutput = material.GetOutput('ri:surface')
        (connectableAPI, outputName, outputType) = materialOutput.GetConnectedSource()
        self.assertEqual(outputName, 'out')
        shader = UsdShade.Shader(connectableAPI)
        self.assertTrue(shader)
        self.assertEqual(shader.GetPrim().GetName(), 'RedLambertSG_lambert')

        shaderId = shader.GetIdAttr().Get()
        self.assertEqual(shaderId, 'PxrDiffuse')

        shaderInputs = shader.GetInputs()
        self.assertEqual(len(shaderInputs), 2)

        diffuseInput = shader.GetInput('diffuseColor')
        self.assertTrue(diffuseInput)
        (connectableAPI, outputName, outputType) = diffuseInput.GetConnectedSource()
        self.assertEqual(outputName, 'displayColor')
        self.assertTrue(connectableAPI)
        self.assertEqual(connectableAPI.GetPath().pathString, materialPath)

        transmissionInput = shader.GetInput('transmissionColor')
        self.assertTrue(transmissionInput)
        (connectableAPI, outputName, outputType) = transmissionInput.GetConnectedSource()
        self.assertEqual(outputName, 'transparency')
        self.assertTrue(connectableAPI)
        self.assertEqual(connectableAPI.GetPath().pathString, materialPath)


if __name__ == '__main__':
    unittest.main(verbosity=2)
