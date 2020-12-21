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

from pxr import Ndr, Sdf, Sdr, Usd, UsdShade
from pxr.Sdr import shaderParserTestUtils as utils

import unittest
import os

class TestUsdShadeShaderDef(unittest.TestCase):
    def test_testSplitShaderIdentifier(self):
        SSI = UsdShade.ShaderDefUtils.SplitShaderIdentifier
        self.assertEqual(SSI('Primvar'), 
                ('Primvar', 'Primvar', Ndr.Version()))
        self.assertEqual(SSI('Primvar_float2'), 
                ('Primvar', 'Primvar_float2', Ndr.Version()))
        self.assertEqual(SSI('Primvar_float2_3'), 
                ('Primvar', 'Primvar_float2', Ndr.Version(3, 0)))
        self.assertEqual(SSI('Primvar_float_3_4'), 
                ('Primvar', 'Primvar_float', Ndr.Version(3, 4)))

        self.assertIsNone(SSI('Primvar_float2_3_nonNumber'))
        self.assertIsNone(SSI('Primvar_4_nonNumber'))

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

        discoveryResults = UsdShade.ShaderDefUtils.GetNodeDiscoveryResults(
                shaderPrim, stage.GetRootLayer().realPath)
        self.assertEqual(len(discoveryResults), 2)

        parserPlugin = UsdShade.ShaderDefParserPlugin()

        nodes = [parserPlugin.Parse(discResult) for discResult in 
                 discoveryResults]
        self.assertEqual(len(nodes), 2)

        for n in nodes:
            self.assertEqual(n.GetVersion(), Ndr.Version(2, 0))
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
            self.assertEqual(n.GetInputNames(), 
                ['fallback', 'float2Val', 'float3Val', 
                 'float4Val', 'normalVector', 'primvarFile', 'primvarName', 
                 'someColor', 'someVector'])
            self.assertEqual(n.GetOutputNames(), ['result', 'result2'])
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

        discoveryResults = UsdShade.ShaderDefUtils.GetNodeDiscoveryResults(
                shaderDef, stage.GetRootLayer().realPath)
        self.assertEqual(len(discoveryResults), 1)

        discoveryResult = discoveryResults[0]
        node = UsdShade.ShaderDefParserPlugin().Parse(discoveryResult)
        assert node is not None

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
