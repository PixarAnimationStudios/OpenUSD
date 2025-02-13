#!/pxrpythonsubst                                                                   
#                                                                                   
# Copyright 2017 Pixar                                                              
#                                                                                   
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Sdf, Sdr, Usd, UsdShade
from pxr.Sdr import shaderParserTestUtils as utils

import unittest
import os

class TestUsdShadeShaderDef(unittest.TestCase):
    def test_ShaderDefParser_NodeDefAPI(self):
        # Test the NodeDefAPI path.
        self.test_ShaderDefParser(useForwardedAPI=False)

    def test_ShaderDefParser(self, useForwardedAPI=True):
        stage = Usd.Stage.CreateNew('shaderDef.usda' if useForwardedAPI else 'shaderDef2.usda')
        shaderPrim = UsdShade.Shader.Define(stage, "/Primvar_float_2")
        if useForwardedAPI:
            # UsdShadeShader has API that covers UsdShadeNodeDefAPI methods;
            # test that they work.
            nodeDefAPI = shaderPrim
        else:
            # Use the methods as implemented on UsdShadeNodeDefAPI.
            nodeDefAPI = UsdShade.NodeDefAPI(shaderPrim)

        shaderPrim.SetSdrMetadataByKey(Sdr.NodeMetadata.Role, 
                                      Sdr.NodeRole.Primvar)
        nodeDefAPI.GetImplementationSourceAttr().Set(UsdShade.Tokens.sourceAsset)

        # Create the files referenced by the sourceAsset attributes.
        osoPath = os.path.normpath(os.path.join(os.getcwd(), 'primvar_2.oso'))
        glslfxPath = os.path.normpath(
            os.path.join(os.getcwd(), 'primvar_2.glslfx'))

        # Create the files referenced by the sourceAsset attributes.
        # These files need to exist for node discovery to succeed.
        open(osoPath, "a").close()
        open(glslfxPath, "a").close()

        nodeDefAPI.SetSourceAsset(Sdf.AssetPath(osoPath), "OSL")
        nodeDefAPI.SetSourceAsset(Sdf.AssetPath(glslfxPath), "glslfx")

        primvarNameInput = shaderPrim.CreateInput('primvarName', 
                Sdf.ValueTypeNames.Token)
        primvarNameInput.SetConnectability(UsdShade.Tokens.interfaceOnly)
        primvarNameInput.SetSdrMetadataByKey('primvarProperty', "1")

        primvarFileInput = shaderPrim.CreateInput('primvarFile', 
                Sdf.ValueTypeNames.Asset)
        primvarFileInput.SetConnectability(UsdShade.Tokens.interfaceOnly)

        fallbackInput = shaderPrim.CreateInput('fallback', 
                Sdf.ValueTypeNames.Float)
        fallbackInput.SetSdrMetadataByKey('defaultInput', "1")

        # Create dummy inputs of other types for testing.
        float2Input = shaderPrim.CreateInput('float2Val', 
                Sdf.ValueTypeNames.Float2)
        float3Input = shaderPrim.CreateInput('float3Val', 
                Sdf.ValueTypeNames.Float3)
        float4Input = shaderPrim.CreateInput('float4Val', 
                Sdf.ValueTypeNames.Float4)

        colorInput = shaderPrim.CreateInput('someColor', 
                Sdf.ValueTypeNames.Color3f)
        vectorInput = shaderPrim.CreateInput('someVector', 
                Sdf.ValueTypeNames.Vector3f)
        normalInput = shaderPrim.CreateInput('normalVector', 
                Sdf.ValueTypeNames.Normal3f)
        matrixInput = shaderPrim.CreateInput('someVector', 
                Sdf.ValueTypeNames.Matrix4d)

        resultOutput = shaderPrim.CreateOutput('result', 
                Sdf.ValueTypeNames.Float)
        result2Output = shaderPrim.CreateOutput('result2', 
                Sdf.ValueTypeNames.Float2)

        discoveryResults = UsdShade.ShaderDefUtils.GetDiscoveryResults(
                shaderPrim, stage.GetRootLayer().realPath)
        self.assertEqual(len(discoveryResults), 2)

        parserPlugin = UsdShade.ShaderDefParserPlugin()

        nodes = [parserPlugin.ParseShaderNode(discResult) for discResult in 
                 discoveryResults]
        self.assertEqual(len(nodes), 2)

        for n in nodes:
            self.assertEqual(n.GetShaderVersion(), Sdr.Version(2, 0))
            self.assertTrue(n.IsValid())
            self.assertEqual(n.GetFamily(), 'Primvar')
            self.assertEqual(n.GetIdentifier(), 'Primvar_float_2')
            self.assertEqual(n.GetImplementationName(), 'Primvar_float')
            self.assertEqual(n.GetRole(), Sdr.NodeRole.Primvar)
            
            assetIdentifierInputNames = n.GetAssetIdentifierInputNames()
            self.assertEqual(len(assetIdentifierInputNames), 1)

            self.assertEqual(n.GetDefaultInput().GetName(), 'fallback')

            self.assertEqual(assetIdentifierInputNames[0], 'primvarFile')
            self.assertEqual(n.GetMetadata(), 
                    {'primvars': '$primvarName',
                     'role': 'primvar'})
            self.assertEqual(n.GetShaderInputNames(), 
                ['fallback', 'float2Val', 'float3Val', 
                 'float4Val', 'normalVector', 'primvarFile', 'primvarName', 
                 'someColor', 'someVector'])
            self.assertEqual(n.GetShaderOutputNames(), ['result', 'result2'])
            if n.GetSourceType() == "OSL":
                self.assertEqual(
                    os.path.normcase(n.GetResolvedImplementationURI()),
                    os.path.normcase(osoPath))
            elif n.GetSourceType() == "glslfx":
                self.assertEqual(
                    os.path.normcase(n.GetResolvedImplementationURI()),
                    os.path.normcase(glslfxPath))

        # Clean-up files.
        os.remove(stage.GetRootLayer().realPath)
        os.remove(osoPath)
        os.remove(glslfxPath)

    def test_ShaderProperties(self):
        """
        Test property correctness on the "TestShaderPropertiesNodeUSD" node.

        See shaderParserTestUtils TestShaderPropertiesNode method for detailed
        description of the test.
        """
        stage = Usd.Stage.Open('shaderDefs.usda')
        shaderDef = UsdShade.Shader.Get(stage,
                                           "/TestShaderPropertiesNodeUSD")

        discoveryResults = UsdShade.ShaderDefUtils.GetDiscoveryResults(
                shaderDef, stage.GetRootLayer().realPath)
        self.assertEqual(len(discoveryResults), 1)

        discoveryResult = discoveryResults[0]
        node = UsdShade.ShaderDefParserPlugin().ParseShaderNode(discoveryResult)
        assert node is not None

        self.assertEqual(os.path.basename(node.GetResolvedImplementationURI()),
                "TestShaderPropertiesNode.glslfx")
        self.assertEqual(os.path.basename(node.GetResolvedDefinitionURI()),
                "shaderDefs.usda")
        
        # Test GetOptions on an attribute via allowdTokens and 
        # sdrMetadata["options"]
        expectedOptionsList = [('token1', ''), ('token2', '')]
        self.assertEqual(
                node.GetShaderInput("testAllowedTokens").GetOptions(),
                expectedOptionsList)
        self.assertEqual(
                node.GetShaderInput("testMetadataOptions").GetOptions(), 
                expectedOptionsList)

        # sdrMetadata options will win over explicitly specified allowedTokens
        # on the attr.
        attr = shaderDef.GetPrim(). \
                GetAttribute('inputs:testAllowedTokenAndMetdataOptions')
        expectedMetdataOptions = "token3|token4"
        expectedAttrAllowedTokens = ["token1", "token2"]
        expectedOptionsList = [('token3', ''), ('token4', '')]
        self.assertEqual(
                node.GetShaderInput("testAllowedTokenAndMetdataOptions"). \
                GetMetadata()["options"], expectedMetdataOptions)
        self.assertEqual([t for t in attr.GetMetadata('allowedTokens')], 
                expectedAttrAllowedTokens)
        self.assertEqual(
                node.GetShaderInput("testAllowedTokenAndMetdataOptions"). \
                GetOptions(), expectedOptionsList)

        # UsdShadeShaderDef already have the types in SdfValueTypeNames
        # conformance, so we do not need any sdrUsdDefinitionType mapping. A
        # bool type gets mapped to int in UsdShadeShaderDef appropriately.
        actualBoolInput = node.GetShaderInput('actualBool')
        attr = shaderDef.GetPrim(). \
                GetAttribute('inputs:actualBool')
        self.assertEqual(attr.GetTypeName(), Sdf.ValueTypeNames.Bool)
        self.assertEqual(actualBoolInput.GetTypeAsSdfType().GetSdfType(),
                Sdf.ValueTypeNames.Bool) 
        self.assertEqual(actualBoolInput.GetType(), Sdf.ValueTypeNames.Int)

        utils.TestShaderPropertiesNode(node)

    def test_GetShaderNodeFromAsset(self):
        """
        Test that invoking the parser to identify shader definitions from an
        asset file works when specifying a subIdentifier and sourceType
        """
        reg = Sdr.Registry()

        node = reg.GetShaderNodeFromAsset(
            "shaderDefs.usda",              # shaderAsset
            {},                             # metadata
            "TestShaderPropertiesNodeUSD",  # subIdentifier
            "glslfx"                        # sourceType
        )

        assert node is not None
        assert node.GetIdentifier().endswith(
            "<TestShaderPropertiesNodeUSD><glslfx>")

if __name__ == '__main__':
    unittest.main()
