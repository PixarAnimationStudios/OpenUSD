#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import UsdUtils, Sdf, Usd, Sdr, UsdShade, Tf, Plug
import os, sys
import unittest

class TestUsdUpdateSchemaWithSdrNode(unittest.TestCase):
    ErrorHandlingTest = False

    @classmethod
    def setUpClass(cls):
        # Register applied schemas 
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
                "Failed to load expected test plugin"
        assert testPlugins[0].name == \
            "TestUsdUtilsUpdateSchemaWithSdrAttrPruning", \
                "Failed to load expected test plugin"
        return True

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
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testSdrNodeAPISchema.usda", 
                "/TestSchemaAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./resultAPISchema.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_APIIdentifierMissing(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.")
            return
        sdrNode = self._GetSdrNode("testAPIIdentifierMissing.usda", 
                "/APIIdentifierMissing")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./resultAPIIdentifierMissing.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_OverrideAPISchemaGen(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testSdrNodeAPISchema.usda", 
                "/TestSchemaAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.FindOrOpen("./result_override.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_OmitDuplicateProperties(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testDuplicateProps.usda",
                "/TestDuplicatePropsAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./duplicateProp.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_OmitDuplicatePropertiesTypeMismatch(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testDuplicatePropsTypeMismatch.usda",
                "/TestDuplicatePropsAPI")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./duplicatePropTypeMisMatch.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_rmanConcreteSchema(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testSdrNodeConcreteSchema.usda",
                "/TestSchemaConcrete")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew("./schemaConcrete.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_UsdShadeConnectableAPIMetadata(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testUsdShadeConnectableAPI.usda", 
                "/TestUsdShadeConnectableAPIMetadataAPI")
        self.assertTrue(sdrNode)
        resultLayer = \
            Sdf.Layer.CreateNew("./resultUsdShadeConnectableAPIMetadata.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_UsdShadeConnectableAPIMetadata2(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testUsdShadeConnectableAPI2.usda", 
                "/TestUsdShadeConnectableAPIMetadataAPI")
        self.assertTrue(sdrNode)
        resultLayer = \
            Sdf.Layer.CreateNew("./resultUsdShadeConnectableAPIMetadata2.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_NamespacePrefixNonShaderNode(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testSdrNodeNonShaderNamespacePrefix.usda", 
                "/TestNamespacePrefix")
        self.assertTrue(sdrNode)
        resultLayer = \
            Sdf.Layer.CreateNew(
                "./testSdrNodeNonShaderNamespacePrefixResult.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, 
                "myRenderContext")

    def test_NamespacePrefixShaderNode(self):
        if self.ErrorHandlingTest:
            self.skipTest("Running Error Handling Test, skipping.");
            return
        sdrNode = self._GetSdrNode("testSdrNodeShaderNamespacePrefix.usda", 
                "/TestNamespacePrefix")
        self.assertTrue(sdrNode)
        resultLayer = \
            Sdf.Layer.CreateNew(
                "./testSdrNodeShaderNamespacePrefixResult.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, "myRenderContext")

    def test_NamespacePrefixOutputTerminal(self):
        if not self.ErrorHandlingTest:
            self.skipTest("Running normal tests, skipping error handling test");
            return
        sdrNode = self._GetSdrNode("testSdrNodeWithOutputAndNSPrefix.usda", 
                "/TestNamespacePrefix")
        self.assertTrue(sdrNode)
        resultLayer = Sdf.Layer.CreateNew(
                "./testSdrNodeShaderNamespaceOutputTerminal.usda")
        UsdUtils.UpdateSchemaWithSdrNode(resultLayer, sdrNode, 
                "myRenderContext")

if __name__ == "__main__":
    # Since py2 currently does not support -k option for test matching, we need
    # to use argument to check if the ErrorHandling tests need to be run or not
    # Eg: 
    # testUsdUtilsUpdateSchemaWithSdrNode TestUsdUpdateSchemaWithSdrNode True
    # will set self.ErrorHandlingTest and skip normal schema gen tests.
    if len(sys.argv) > 2:
        TestUsdUpdateSchemaWithSdrNode.ErrorHandlingTest = sys.argv.pop()
    unittest.main(verbosity=2)
