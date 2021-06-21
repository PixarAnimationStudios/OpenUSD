#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

from pxr import UsdUtils, Sdf, Usd, Sdr, UsdShade, Tf
import os
import unittest

class TestUsdUpdateSchemaWithSdrNode(unittest.TestCase):
    def _GetSdrNode(self, assetFile, shaderDefPrimPath):
        stage = Usd.Stage.Open(assetFile)
        self.assertTrue(stage)
        shaderDef = UsdShade.Shader.Get(stage, shaderDefPrimPath)
        results = UsdShade.ShaderDefUtils.GetNodeDiscoveryResults(shaderDef, 
                stage.GetRootLayer().realPath)
        self.assertEqual(len(results), 1)
        node = UsdShade.ShaderDefParserPlugin().Parse(results[0])
        self.assertTrue(node)
        return node
    
    def test_APISchemaGen(self):
        sdrNode = self._GetSdrNode("testSdrNodeAPISchema.usda", 
                "/TestSchemaAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./resultAPISchema.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode)

    def test_APIIdentifierMissing(self):
        sdrNode = self._GetSdrNode("testAPIIdentifierMissing.usda", 
                "/APIIdentifierMissing")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./resultAPIIdentifierMissing.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode)

    def test_OverrideAPISchemaGen(self):
        sdrNode = self._GetSdrNode("testSdrNodeAPISchema.usda", 
                "/TestSchemaAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.FindOrOpen("./result_override.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode)

    def test_OmitDuplicateProperties(self):
        sdrNode = self._GetSdrNode("testDuplicateProps.usda",
                "/TestDuplicatePropsAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./duplicateProp.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode)

    def test_OmitDuplicatePropertiesTypeMismatch(self):
        sdrNode = self._GetSdrNode("testDuplicatePropsTypeMismatch.usda",
                "/TestDuplicatePropsAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./duplicatePropTypeMisMatch.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode)

    def test_rmanConcreteSchema(self):
        sdrNode = self._GetSdrNode("testSdrNodeConcreteSchema.usda",
                "/TestSchemaConcrete")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./schemaConcrete.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode)

if __name__ == "__main__":
    unittest.main()
