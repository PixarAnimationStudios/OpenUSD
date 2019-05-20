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
#

from pxr import Gf
from pxr import Sdf
from pxr import Usd
from pxr import UsdShade
from pxr import UsdUtils

from maya import cmds
from maya import standalone

import os
import unittest


class testPxrUsdPreviewSurfaceExport(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')

        cmds.file(os.path.abspath('PxrUsdPreviewSurfaceExportTest.ma'),
            open=True, force=True)

        # Export to USD.
        usdFilePath = os.path.abspath('PxrUsdPreviewSurfaceExportTest.usda')

        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True, file=usdFilePath,
            shadingMode='useRegistry')

        cls.stage = Usd.Stage.Open(usdFilePath)

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testStagePrerequisites(self):
        self.assertTrue(self.stage)

    def _GetUsdMaterial(self, materialName):
        modelPrimPath = Sdf.Path.absoluteRootPath.AppendChild(
            'PxrUsdPreviewSurfaceExportTest')
        materialsRootPrimPath = modelPrimPath.AppendChild(
            UsdUtils.GetMaterialsScopeName())
        materialPrimPath = materialsRootPrimPath.AppendChild(materialName)
        materialPrim = self.stage.GetPrimAtPath(materialPrimPath)
        self.assertTrue(materialPrim)

        materialSchema = UsdShade.Material(materialPrim)
        self.assertTrue(materialSchema)

        return materialSchema

    def _GetSourceShader(self, inputOrOutput):
        (connectableAPI, _, _) = inputOrOutput.GetConnectedSource()
        self.assertTrue(connectableAPI.IsShader())
        shaderPrim = connectableAPI.GetPrim()
        self.assertTrue(shaderPrim)

        shader = UsdShade.Shader(shaderPrim)
        self.assertTrue(shader)

        return shader

    def _ValidateUsdShader(self, shader, expectedInputTuples, expectedOutputs):
        for expectedInputTuple in expectedInputTuples:
            (inputName, expectedValue) = expectedInputTuple

            shaderInput = shader.GetInput(inputName)
            self.assertTrue(shaderInput)

            if expectedValue is None:
                self.assertFalse(shaderInput.GetAttr().HasAuthoredValueOpinion())
                continue

            # Validate the input value
            value = shaderInput.Get()
            if (isinstance(value, float) or isinstance(value, Gf.Vec3f)):
                self.assertTrue(Gf.IsClose(value, expectedValue, 1e-6))
            else:
                self.assertEqual(value, expectedValue)

        outputs = {output.GetBaseName() : output.GetTypeName()
            for output in shader.GetOutputs()}

        self.assertEqual(outputs, expectedOutputs)

    def testExportStandalonePxrUsdPreviewSurface(self):
        """
        Tests that a pxrUsdPreviewSurface with attribute values but no
        connections authored exports correctly.
        """
        standaloneMaterial = self._GetUsdMaterial(
            'pxrUsdPreviewSurface_StandaloneSG')

        surfaceOutput = standaloneMaterial.GetOutput(UsdShade.Tokens.surface)
        previewSurfaceShader = self._GetSourceShader(surfaceOutput)

        expectedShaderPrimPath = standaloneMaterial.GetPath().AppendChild(
            'pxrUsdPreviewSurface_Standalone')

        self.assertEqual(previewSurfaceShader.GetPath(),
            expectedShaderPrimPath)

        self.assertEqual(previewSurfaceShader.GetShaderId(),
            'UsdPreviewSurface')

        expectedInputTuples = [
            ('clearcoat', 0.1),
            ('clearcoatRoughness', 0.2),
            ('diffuseColor', Gf.Vec3f(0.3, 0.4, 0.5)),
            ('displacement', 0.6),
            ('emissiveColor', Gf.Vec3f(0.07, 0.08, 0.09)),
            ('ior', 1.1),
            ('metallic', 0.11),
            ('normal', Gf.Vec3f(0.12, 0.13, 0.14)),
            ('occlusion', 0.9),
            ('opacity', 0.8),
            ('roughness', 0.7),
            ('specularColor', Gf.Vec3f(0.3, 0.2, 0.1)),
            ('useSpecularWorkflow', 1)
        ]

        expectedOutputs = {
            'surface': Sdf.ValueTypeNames.Token,
            'displacement': Sdf.ValueTypeNames.Token
        }

        self._ValidateUsdShader(previewSurfaceShader, expectedInputTuples,
            expectedOutputs)

        # There should not be any additional inputs.
        self.assertEqual(
            len(previewSurfaceShader.GetInputs()), len(expectedInputTuples))

    def testExportConnectedPxrUsdPreviewSurface(self):
        """
        Tests that a pxrUsdPreviewSurface with bindings to other shading nodes
        exports correctly.
        """
        connectedMaterial = self._GetUsdMaterial(
            'pxrUsdPreviewSurface_ConnectedSG')

        surfaceOutput = connectedMaterial.GetOutput(UsdShade.Tokens.surface)
        previewSurfaceShader = self._GetSourceShader(surfaceOutput)

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            'pxrUsdPreviewSurface_Connected')

        self.assertEqual(previewSurfaceShader.GetPath(),
            expectedShaderPrimPath)

        self.assertEqual(previewSurfaceShader.GetShaderId(),
            'UsdPreviewSurface')

        expectedInputTuples = [
            ('clearcoat', 0.1),
            ('specularColor', Gf.Vec3f(0.18, 0.18, 0.18)),
            ('useSpecularWorkflow', 1)
        ]

        expectedOutputs = {
            'surface': Sdf.ValueTypeNames.Token,
            'displacement': Sdf.ValueTypeNames.Token
        }

        self._ValidateUsdShader(previewSurfaceShader, expectedInputTuples,
            expectedOutputs)

        # There should be three more connected inputs in addition to the inputs
        # with authored values.
        self.assertEqual(len(previewSurfaceShader.GetInputs()),
            len(expectedInputTuples) + 3)

        # Validate the UsdUvTexture prim connected to the UsdPreviewSurface's
        # diffuseColor input.
        diffuseColorInput = previewSurfaceShader.GetInput('diffuseColor')
        difTexShader = self._GetSourceShader(diffuseColorInput)

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            'Brazilian_Rosewood_Texture')

        self.assertEqual(difTexShader.GetPath(), expectedShaderPrimPath)
        self.assertEqual(difTexShader.GetShaderId(), 'UsdUVTexture')

        # Validate the UsdUvTexture prim connected to the UsdPreviewSurface's
        # clearcoatRoughness and roughness inputs. They should both be fed by
        # the same shader prim.
        clearcoatRougnessInput = previewSurfaceShader.GetInput('clearcoatRoughness')
        bmpTexShader = self._GetSourceShader(clearcoatRougnessInput)
        rougnessInput = previewSurfaceShader.GetInput('roughness')
        roughnessShader = self._GetSourceShader(rougnessInput)

        self.assertEqual(bmpTexShader.GetPrim(), roughnessShader.GetPrim())

        expectedShaderPrimPath = connectedMaterial.GetPath().AppendChild(
            'Brazilian_Rosewood_Bump_Texture')

        self.assertEqual(bmpTexShader.GetPath(), expectedShaderPrimPath)
        self.assertEqual(bmpTexShader.GetShaderId(), 'UsdUVTexture')


if __name__ == '__main__':
    unittest.main(verbosity=2)
