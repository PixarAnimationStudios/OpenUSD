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

MaterialPath = Sdf.Path("/Material")

Shader1Path = Sdf.Path("/Material/Shader1")
Shader2Path = Sdf.Path("/Material/Shader2")

NodeGraph1Path = Sdf.Path("/Material/NodeGraph1")
NestedShader1Path = Sdf.Path("/Material/NodeGraph1/NestedShader1")
NestedShader2Path = Sdf.Path("/Material/NodeGraph1/NestedShader2")
NestedNodeGraphPath = Sdf.Path("/Material/NodeGraph1/NestedNodeGraph")

NodeGraph2Path = Sdf.Path("/Material/NodeGraph2")
NestedShader3Path = Sdf.Path("/Material/NodeGraph2/NestedShader3")

class TestUsdShadeInterfaceInputConsumer(unittest.TestCase):
    def _setupStage(self, usdStage):
        material = UsdShade.Material.Define(usdStage, MaterialPath)
        self.assertTrue(material)
        
        nodeGraph1 = UsdShade.NodeGraph.Define(usdStage, NodeGraph1Path)
        self.assertTrue(nodeGraph1)

        nodeGraph2 = UsdShade.NodeGraph.Define(usdStage, NodeGraph2Path)
        self.assertTrue(nodeGraph2)
        
        shader1 = UsdShade.Shader.Define(usdStage, Shader1Path)
        self.assertTrue(shader1)

        shader2 = UsdShade.Shader.Define(usdStage, Shader2Path)
        self.assertTrue(shader2)
        
        nestedShader1 = UsdShade.Shader.Define(usdStage, NestedShader1Path)
        self.assertTrue(nestedShader1)

        nestedShader2 = UsdShade.Shader.Define(usdStage, NestedShader2Path)
        self.assertTrue(nestedShader2)

        nestedShader3 = UsdShade.Shader.Define(usdStage, NestedShader3Path)
        self.assertTrue(nestedShader3)
        
        nestedNodeGraph = UsdShade.NodeGraph.Define(usdStage, NestedNodeGraphPath)
        self.assertTrue(nestedNodeGraph)

        # Create all inputs.
        floatInput = material.CreateInput("floatInput", Sdf.ValueTypeNames.Float)
        colorInput = material.CreateInput("colorInput", Sdf.ValueTypeNames.Color3f)
        
        shader1Input1 = shader1.CreateInput("shader1Input1", Sdf.ValueTypeNames.Float)
        shader1Input2 = shader1.CreateInput("shader1Input2", Sdf.ValueTypeNames.Color3f)

        shader2Input1 = shader2.CreateInput("shader2Input1", Sdf.ValueTypeNames.Color3f)
        shader2Input2 = shader2.CreateInput("shader2Input2", Sdf.ValueTypeNames.Float)

        nodeGraph1FloatInput = nodeGraph1.CreateInput("nodeGraph1FloatInput",
                                                  Sdf.ValueTypeNames.Float)
        nodeGraph1ColorInput = nodeGraph1.CreateInput("nodeGraph1ColorInput", 
                                                  Sdf.ValueTypeNames.Color3f)

        nodeGraph2FloatInput = nodeGraph2.CreateInput("nodeGraph2FloatInput",
                                                  Sdf.ValueTypeNames.Float)
        nodeGraph2ColorInput = nodeGraph2.CreateInput("nodeGraph2ColorInput", 
                                                  Sdf.ValueTypeNames.Color3f)

        nestedShader1Input1 = nestedShader1.CreateInput("nestedShader1Input1", 
                                                       Sdf.ValueTypeNames.Color3f)
        nestedShader1Input2 = nestedShader1.CreateInput("nestedShader1Input2", 
                                                       Sdf.ValueTypeNames.Float)

        nestedShader2Input1 = nestedShader2.CreateInput("nestedShader2Input1", 
                                                       Sdf.ValueTypeNames.Float)
        nestedShader2Input2 = nestedShader2.CreateInput("nestedShader2Input2", 
                                                       Sdf.ValueTypeNames.Color3f)

        nestedShader3Input1 = nestedShader3.CreateInput("nestedShader3Input1", 
                                                       Sdf.ValueTypeNames.Color3f)
        nestedShader3Input2 = nestedShader3.CreateInput("nestedShader3Input2", 
                                                       Sdf.ValueTypeNames.Float)
        
        nestedNodeGraphInput1 = nestedNodeGraph.CreateInput("nestedNodeGraphInput1", 
                                                          Sdf.ValueTypeNames.Float)
        nestedNodeGraphInput2 = nestedNodeGraph.CreateInput("nestedNodeGraphInput2", 
                                                          Sdf.ValueTypeNames.Color3f)

        # Wire up the connections.
        shader1Input1.ConnectToSource(floatInput)
        shader1Input2.ConnectToSource(colorInput)

        shader2Input1.ConnectToSource(colorInput)
        shader2Input2.ConnectToSource(floatInput)

        nodeGraph1ColorInput.ConnectToSource(colorInput)
        nodeGraph1FloatInput.ConnectToSource(floatInput)

        nodeGraph2ColorInput.ConnectToSource(colorInput)
        nodeGraph2FloatInput.ConnectToSource(floatInput)

        nestedShader1Input1.ConnectToSource(nodeGraph1ColorInput)
        nestedShader1Input2.ConnectToSource(nodeGraph1FloatInput)

        nestedShader2Input1.ConnectToSource(nodeGraph1FloatInput)
        nestedShader2Input2.ConnectToSource(nodeGraph1ColorInput)

        nestedShader3Input1.ConnectToSource(nodeGraph2ColorInput)
        nestedShader3Input2.ConnectToSource(nodeGraph2FloatInput)

        nestedNodeGraphInput1.ConnectToSource(nodeGraph1FloatInput)
        nestedNodeGraphInput2.ConnectToSource(nodeGraph1ColorInput)

    def test_InterfaceConsumers(self):
        usdStage = Usd.Stage.CreateInMemory()
        self.assertTrue(usdStage)
        self._setupStage(usdStage)

        material = UsdShade.Material.Get(usdStage, MaterialPath)
        self.assertTrue(material)

        interfaceConsumersMapping = material.ComputeInterfaceInputConsumersMap()
        self.assertEqual(len(interfaceConsumersMapping), 2)

        for interfaceInput, consumers in interfaceConsumersMapping.iteritems():
            if interfaceInput.GetAttr().GetBaseName() == "floatInput":
                self.assertEqual(set([i.GetAttr().GetBaseName() for i in consumers]), 
                                 set(["nodeGraph1FloatInput", "nodeGraph2FloatInput",
                                      "shader1Input1", "shader2Input2"]))
            elif interfaceInput.GetAttr().GetBaseName() == "colorInput":
                self.assertEqual(set([i.GetAttr().GetBaseName() for i in consumers]), 
                                 set(["nodeGraph1ColorInput", "nodeGraph2ColorInput", 
                                      "shader2Input1", "shader1Input2"]))
            else:
                Tf.RaiseRuntimeError("Unexpected input: %s" % 
                                     interfaceInput.GetFullName())

        transitiveInterfaceMapping = material.ComputeInterfaceInputConsumersMap(True)
        self.assertEqual(len(transitiveInterfaceMapping), 2)

        for interfaceInput, consumers in transitiveInterfaceMapping.iteritems():
            if interfaceInput.GetAttr().GetBaseName() == "floatInput":
                self.assertEqual(set([i.GetAttr().GetBaseName() for i in consumers]), 
                                 set(["nestedShader1Input2", "nestedShader2Input1",
                                      "shader1Input1", "shader2Input2", 
                                      "nestedShader3Input2", "nestedNodeGraphInput1"]))
            elif interfaceInput.GetAttr().GetBaseName() == "colorInput":
                self.assertEqual(set([i.GetAttr().GetBaseName() for i in consumers]), 
                                 set(["nestedShader1Input1", "nestedShader2Input2",
                                      "shader1Input2", "shader2Input1",
                                      "nestedShader3Input1", "nestedNodeGraphInput2"]))
            else:
                Tf.RaiseRuntimeError("Unexpected input: %s" % 
                                     interfaceInput.GetFullName())

if __name__ == '__main__':
    unittest.main()
