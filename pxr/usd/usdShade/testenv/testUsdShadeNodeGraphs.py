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

NODEGRAPH_PATH = Sdf.Path('/MyNodeGraph')
NESTED_NODEGRAPH_PATH = Sdf.Path('/MyNodeGraph/NestedNodeGraph')
NESTED_NODEGRAPH_SHADER_PATH = Sdf.Path('/MyNodeGraph/NestedNodeGraph/NestedShader')

# Names of shading prims and properties that we will be creating.
SHADERS = ['ShaderOne', 'ShaderTwo', 'ShaderThree']
OUTPUTS = ['OutputOne', 'OutputTwo', 'OutputThree']
PARAMS = ['ParamOne', 'ParamTwo', 'ParamThree']
INPUTS = ['InputOne', 'InputTwo', 'InputThree']

class TestUsdShadeNodeGraphs(unittest.TestCase):
    def _SetupStage(self):
        usdStage = Usd.Stage.CreateInMemory()
        self.assertTrue(usdStage)
    
        nodeGraph = UsdShade.NodeGraph.Define(usdStage, NODEGRAPH_PATH)
        self.assertTrue(nodeGraph)
    
        for i in range(len(SHADERS)):
            outputName = OUTPUTS[i]
            shaderName = SHADERS[i]
    
            paramName = PARAMS[i]
            inputName = INPUTS[i]
    
            shaderPath = '%s/%s' % (NODEGRAPH_PATH, shaderName)
            shader = UsdShade.Shader.Define(usdStage, shaderPath)
            self.assertTrue(shader)
    
            shaderInput = shader.CreateInput(paramName, 
                                             Sdf.ValueTypeNames.Float)
            self.assertTrue(shaderInput)
    
            nodeGraphInput = nodeGraph.CreateInput(inputName, 
                                                   Sdf.ValueTypeNames.Float)
            self.assertTrue(nodeGraphInput)
    
            shaderOutput = shader.CreateOutput(outputName, 
                                               Sdf.ValueTypeNames.Int)
            self.assertTrue(shaderOutput)
    
            nodeGraphOutput = nodeGraph.CreateOutput(outputName, 
                                                     Sdf.ValueTypeNames.Int)
            self.assertTrue(nodeGraphOutput)
    
            self.assertTrue(nodeGraphOutput.ConnectToSource(shaderOutput))
    
            self.assertTrue(shaderInput.ConnectToSource(nodeGraphInput))

        nestedNodeGraph = UsdShade.NodeGraph.Define(usdStage, 
            NESTED_NODEGRAPH_PATH)
        self.assertTrue(nestedNodeGraph)
    
        nestedNodeGraphShader = UsdShade.Shader.Define(usdStage, 
            NESTED_NODEGRAPH_SHADER_PATH)
        self.assertTrue(nestedNodeGraphShader)

        nestedNodeGraphInput = nestedNodeGraph.CreateInput("NestedInput", 
            Sdf.ValueTypeNames.Float)
        self.assertTrue(nestedNodeGraphInput)

        nestedNodeGraphInput.ConnectToSource(
            NODEGRAPH_PATH.AppendProperty("inputs:InputTwo"))
    
        nestedNodeGraphOutput = nestedNodeGraph.CreateOutput("NestedOutput", 
            Sdf.ValueTypeNames.Int)
        self.assertTrue(nestedNodeGraphOutput)
    
        nestedNodeGraphShaderOutput = nestedNodeGraphShader.CreateOutput("NestedShaderOutput", 
            Sdf.ValueTypeNames.Int)
        self.assertTrue(nestedNodeGraphShaderOutput)

        nodeGraphOutputThree = nodeGraph.GetOutput("OutputThree")
        nodeGraphOutputThree.ConnectToSource(nestedNodeGraphOutput)
        nestedNodeGraphOutput.ConnectToSource(nestedNodeGraphShaderOutput)

        return usdStage
    
    def _TestOutputs(self, usdStage):
        """
        Tests getting all the outputs of a nodeGraph.
        """
        nodeGraph = UsdShade.NodeGraph.Get(usdStage, NODEGRAPH_PATH)
        nestedNodeGraph = UsdShade.NodeGraph.Get(usdStage, 
                NESTED_NODEGRAPH_PATH)
        nestedNodeGraphShader = UsdShade.Shader.Get(usdStage, 
                NESTED_NODEGRAPH_SHADER_PATH)

        self.assertTrue(nodeGraph)
        self.assertTrue(nestedNodeGraph)
        self.assertTrue(nestedNodeGraphShader)

        allOutputs = nodeGraph.GetOutputs()
        self.assertEqual(len(allOutputs), 3)
        
        outputNames = {x.GetBaseName() for x in allOutputs}
        self.assertEqual(outputNames, set(OUTPUTS))
    
        # Getting all of the outputs individually by name should be equivalent
        # to getting them all at once with GetOutputs().
        outputs = {nodeGraph.GetOutput(outputName).GetBaseName()
                    for outputName in OUTPUTS}
        self.assertEqual(outputs, {output.GetBaseName() for output in allOutputs})
    
        # Test outputs on nested nodeGraphs and their connectability to other 
        # outputs.
        nestedNodeGraphOutputs = nestedNodeGraph.GetOutputs()
        self.assertEqual(len(nestedNodeGraphOutputs), 1)
        nestedNodeGraphOutput = nestedNodeGraphOutputs[0]

        nodeGraphOutput = nodeGraph.GetOutput("OutputThree")
        outputSource = nodeGraphOutput.GetConnectedSource()
        self.assertEqual(outputSource[0].GetPath(), nestedNodeGraph.GetPath())
        self.assertEqual(outputSource[1], "NestedOutput")
        self.assertEqual(outputSource[2], UsdShade.AttributeType.Output)

        nestedOutputSource = nestedNodeGraphOutputs[0].GetConnectedSource()
        self.assertEqual(nestedOutputSource[0].GetPath(), nestedNodeGraphShader.GetPath())
        self.assertEqual(nestedOutputSource[1], "NestedShaderOutput")
        self.assertEqual(nestedOutputSource[2], UsdShade.AttributeType.Output)

        computedNodeGraphOutputSource = nodeGraph.ComputeOutputSource(
            "OutputThree")
        self.assertEqual(computedNodeGraphOutputSource[0].GetPath(), 
                         nestedOutputSource[0].GetPath())
        self.assertEqual(computedNodeGraphOutputSource[1:2], 
                         nestedOutputSource[1:2])

    def _TestInputs(self, usdStage):
        nodeGraph = UsdShade.NodeGraph.Get(usdStage, NODEGRAPH_PATH)
        nestedNodeGraph = UsdShade.NodeGraph.Get(usdStage, NESTED_NODEGRAPH_PATH)
    
        allInputs = nodeGraph.GetInputs()
        self.assertEqual(len(allInputs), 3)
        
        inputNames = {x.GetBaseName() for x in allInputs}
        self.assertEqual(inputNames , set(INPUTS))
    
        # Getting all of the inputs indivCreateParameteridually by name should be equivalent to 
        # getting them all at once with GetInputs().
        inputs = {nodeGraph.GetInput(inputName).GetBaseName()
                  for inputName in INPUTS}
        self.assertEqual(inputs, {i.GetBaseName() for i in allInputs})
    
        # Test input to input connections.
        nestedInputs = nestedNodeGraph.GetInputs()
        self.assertEqual(len(nestedInputs), 1)
        nestedInputSource = nestedInputs[0].GetConnectedSource()
        self.assertEqual(nestedInputSource[0].GetPath(), nodeGraph.GetPath())
        self.assertEqual(nestedInputSource[1], 'InputTwo')
        self.assertEqual(nestedInputSource[2], UsdShade.AttributeType.Input)
    
        # Test ComputeInterfaceInputConsumersMap.
        inputConsumersMap = nodeGraph.ComputeInterfaceInputConsumersMap()
        for shadeInput, consumers in inputConsumersMap.iteritems():
            if shadeInput.GetBaseName() == "InputOne":
                self.assertEqual(len(consumers), 1)
                self.assertEqual(consumers[0].GetFullName(), "inputs:ParamOne")
            elif shadeInput.GetBaseName() == "InputTwo":
                self.assertEqual(len(consumers), 2)
            elif shadeInput.GetBaseName() == "InputThree":
                self.assertEqual(len(consumers), 1)
                self.assertEqual(consumers[0].GetFullName(), "inputs:ParamThree")
        
        nestedInputConsumersMap = nestedNodeGraph.ComputeInterfaceInputConsumersMap()
        self.assertEqual(len(nestedInputConsumersMap), 1)

    def test_Basic(self):
        stage = self._SetupStage()
        self._TestOutputs(stage)
        self._TestInputs(stage)

    def test_StaticMethods(self):
        self.assertFalse(UsdShade.Input.IsInterfaceInputName('interface:bla'))
        self.assertTrue(UsdShade.Input.IsInterfaceInputName('inputs:bla'))
        self.assertTrue(UsdShade.Input.IsInterfaceInputName('inputs:other:bla'))
        self.assertFalse(UsdShade.Input.IsInterfaceInputName('notinput:bla'))
        self.assertFalse(UsdShade.Input.IsInterfaceInputName('paramName'))
        self.assertFalse(UsdShade.Input.IsInterfaceInputName(''))

        stage = self._SetupStage()
        self.assertTrue(UsdShade.Input.IsInput(
            stage.GetPrimAtPath(NODEGRAPH_PATH).GetAttribute(
                'inputs:InputOne')))
        self.assertFalse(UsdShade.Input.IsInput(
            stage.GetPrimAtPath(NODEGRAPH_PATH).GetAttribute(
                'outputs:OutputOne')))

        stage = self._SetupStage()
        self.assertTrue(UsdShade.Input.IsInput(
            stage.GetPrimAtPath(NODEGRAPH_PATH).GetAttribute(
                'inputs:InputOne')))
        self.assertFalse(UsdShade.Input.IsInput(
            stage.GetPrimAtPath(NODEGRAPH_PATH).GetAttribute(
                'outputs:OutputOne')))

if __name__ == '__main__':
    unittest.main()
