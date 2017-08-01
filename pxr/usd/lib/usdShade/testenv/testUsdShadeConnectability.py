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

from pxr import Sdf, Usd, UsdShade
import unittest

class TestUsdShadeConnectability(unittest.TestCase):
    def _CanConnect(self, shadingProp, source):
        canConnect = UsdShade.ConnectableAPI.CanConnect(shadingProp, 
            source)
        self.assertTrue(canConnect)

    def _CannotConnect(self, shadingProp, source):
        canConnect = UsdShade.ConnectableAPI.CanConnect(shadingProp, 
            source)
        self.assertFalse(canConnect)

    def test_Basic(self):
        MaterialPath = Sdf.Path("/Material")
        ShaderPath = Sdf.Path("/Material/Shader")
        NodeGraphPath = Sdf.Path("/Material/NodeGraph")
        NestedShaderPath = Sdf.Path("/Material/NodeGraph/NestedShader")

        usdStage = Usd.Stage.CreateInMemory()
        self.assertTrue(usdStage)

        material = UsdShade.Material.Define(usdStage, MaterialPath)
        self.assertTrue(material)

        nodeGraph = UsdShade.NodeGraph.Define(usdStage, NodeGraphPath)
        self.assertTrue(nodeGraph)

        shader = UsdShade.Shader.Define(usdStage, ShaderPath)
        self.assertTrue(shader)

        nestedShader = UsdShade.Shader.Define(usdStage, NestedShaderPath)
        self.assertTrue(nestedShader)

        # Create all inputs and connections.

        # Create a float interface input on the material.
        matConnectable = material.ConnectableAPI()
        floatInterfaceInput = matConnectable.CreateInput("floatInput",
                                                   Sdf.ValueTypeNames.Float)
        # default connectability of an interface-input is 'full'
        self.assertEqual(floatInterfaceInput.GetConnectability(), 
                    UsdShade.Tokens.full)

        self.assertTrue(floatInterfaceInput.SetConnectability(
                UsdShade.Tokens.interfaceOnly))
        self.assertEqual(floatInterfaceInput.GetConnectability(),
                         UsdShade.Tokens.interfaceOnly)

        self.assertTrue(floatInterfaceInput.ClearConnectability())
        self.assertEqual(floatInterfaceInput.GetConnectability(), 
                         UsdShade.Tokens.full)


        # Create a color valued interface input on the material.
        colorInterfaceInput = material.CreateInput("colorInput",
                                                   Sdf.ValueTypeNames.Color3f)
        self.assertEqual(colorInterfaceInput.GetConnectability(),
                         UsdShade.Tokens.full)
        self.assertTrue(colorInterfaceInput.SetConnectability(
                UsdShade.Tokens.interfaceOnly))

        colorMaterialOutput = material.CreateOutput("colorOutput",
                                                    Sdf.ValueTypeNames.Color3f)

        # Create shader inputs.
        shaderConnectable = shader.ConnectableAPI()
        shaderInputFloat = shaderConnectable.CreateInput("shaderFloat", 
                Sdf.ValueTypeNames.Float)
        shaderInputColor = shader.CreateInput("shaderColor",
                                              Sdf.ValueTypeNames.Color3f)
        self.assertEqual(shaderInputFloat.GetConnectability(), 
                         UsdShade.Tokens.full)
        self.assertEqual(shaderInputColor.GetConnectability(), 
                         UsdShade.Tokens.full)

        # The shader inputs have full connectability by default and can be 
        # connected to any input or output.
        self._CanConnect(shaderInputColor, colorInterfaceInput)
        # Make the connection.
        self.assertTrue(shaderInputColor.ConnectToSource(colorInterfaceInput))

        if UsdShade.Utils.WriteNewEncoding():
            self.assertEqual(
                shaderInputColor.GetRawConnectedSourcePaths(),
                [colorInterfaceInput.GetAttr().GetPath()])
    
        self._CanConnect(shaderInputFloat, floatInterfaceInput)
        self.assertTrue(shaderInputFloat.ConnectToSource(floatInterfaceInput))

        if UsdShade.Utils.WriteNewEncoding():
            self.assertEqual(shaderInputFloat.GetRawConnectedSourcePaths(),
                             [floatInterfaceInput.GetAttr().GetPath()])

        shaderOutputColor = shader.CreateOutput("color", Sdf.ValueTypeNames.Color3f)
        shaderOutputFloat = shader.CreateOutput("fOut", Sdf.ValueTypeNames.Float)

        nodeGraphInputFloat = nodeGraph.CreateInput("nodeGraphFlIn",
                                                    Sdf.ValueTypeNames.Float)
        self.assertEqual(nodeGraphInputFloat.GetConnectability(),
                         UsdShade.Tokens.full)

        # NodeGraph Input with "interfaceOnly" connectability cannot be 
        # connected to an output.
        self.assertTrue(nodeGraphInputFloat.SetConnectability(
                UsdShade.Tokens.interfaceOnly))
        self._CannotConnect(nodeGraphInputFloat, shaderOutputFloat)

        # NodeGraph Input with full connectability can be connected to an output.
        self.assertTrue(nodeGraphInputFloat.SetConnectability(UsdShade.Tokens.full))
        self._CanConnect(nodeGraphInputFloat, shaderOutputFloat)
        self.assertTrue(nodeGraphInputFloat.ConnectToSource(shaderOutputFloat))

        nodeGraphInputColor = nodeGraph.CreateInput("nodeGraphColor", 
                                                    Sdf.ValueTypeNames.Color3f)
        self.assertTrue(nodeGraphInputColor.SetConnectability(
                                UsdShade.Tokens.interfaceOnly))
        # Can't connect an "interfaceOnly" input to an output on a shader or node-graph.
        self._CannotConnect(nodeGraphInputColor, shaderOutputColor)
        self._CannotConnect(nodeGraphInputColor, colorMaterialOutput)

        # Can't connect an "interfaceOnly" input to a "full" input on a shader.
        self._CannotConnect(nodeGraphInputColor, shaderInputColor)

        # Change connectability of input on shader to "interfaceOnly" to allow the 
        # previously attempted connection.
        self.assertTrue(shaderInputColor.SetConnectability(UsdShade.Tokens.interfaceOnly))
        self._CanConnect(nodeGraphInputColor, shaderInputColor)
        self.assertTrue(nodeGraphInputColor.ConnectToSource(shaderInputColor))

        # Change connectability of an interface input to full, to test connection 
        # from an "interfaceOnly" to a "full" input on a nodegraph. 
        self.assertTrue(floatInterfaceInput.SetConnectability(UsdShade.Tokens.full))
        self.assertEqual(floatInterfaceInput.GetConnectability(),
                         UsdShade.Tokens.full)

        nestedShaderInputFloat = nestedShader.CreateInput("nestedShaderFloat", 
                                                          Sdf.ValueTypeNames.Float)
        self.assertTrue(nestedShaderInputFloat.SetConnectability(
                            UsdShade.Tokens.interfaceOnly))
        self._CannotConnect(nestedShaderInputFloat, floatInterfaceInput)
    
        # Change connectability of interface input to "interfaceOnly". This will 
        # allow the previously attempted connection.
        self.assertTrue(floatInterfaceInput.SetConnectability(
                            UsdShade.Tokens.interfaceOnly))
        self._CanConnect(nestedShaderInputFloat, floatInterfaceInput)
        self.assertTrue(nestedShaderInputFloat.ConnectToSource(floatInterfaceInput))
   
if __name__ == "__main__":
    unittest.main()
