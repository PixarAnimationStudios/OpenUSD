#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from __future__ import print_function

import unittest
from pxr import Usd, UsdShade, Sdf

Input = UsdShade.AttributeType.Input
Output = UsdShade.AttributeType.Output
Invalid = UsdShade.AttributeType.Invalid

class TestUsdShadeGetValueProducingAttribute(unittest.TestCase):

    def _getType(self, attr):
        return UsdShade.Utils.GetType(attr.GetName())

    def _check(self, attr, expectedPath, expectedValue=None):
        print("CHECK:", attr, "vs", expectedPath)
        self.assertTrue(attr)
        if 'inputs:' in expectedPath:
            self.assertTrue(UsdShade.Input.IsInput(attr))
        else:
            self.assertTrue(UsdShade.Output.IsOutput(attr))
        self.assertEqual(attr.GetPath(), Sdf.Path(expectedPath))
        self.assertEqual(attr.Get(), expectedValue)

    def _test(self, prim, inputName,
              expectedType, expectedPath, expectedValue=None):
        attrs = prim.GetInput(inputName).GetValueProducingAttributes()
        self.assertEqual(len(attrs), 1)
        self.assertEqual(self._getType(attrs[0]), expectedType)
        self._check(attrs[0], expectedPath, expectedValue)

    def _testInvalid(self, prim, inputName):
        attrs = prim.GetInput(inputName).GetValueProducingAttributes()
        self.assertEqual(len(attrs), 0)
        self.assertFalse(attrs)

    def _testMulti(self, prim, inputName,
                   expectedTypes, expectedPaths, expectedValues=None):
        attrs = prim.GetInput(inputName).GetValueProducingAttributes()
        self.assertEqual(len(attrs), len(expectedPaths))
        for i in range(len(attrs)):
            self.assertEqual(self._getType(attrs[i]), expectedTypes[i])
            expectedValue = expectedValues[i] if expectedValues else None
            self._check(attrs[i], expectedPaths[i], expectedValue)

    def test_GetValueProducingAttribute(self):
        stage = Usd.Stage.Open('test.usda')

        material = UsdShade.ConnectableAPI(
                        stage.GetPrimAtPath('/Material'))
        terminal = UsdShade.ConnectableAPI(
                        stage.GetPrimAtPath('/Material/Terminal'))
        nodeGraph = UsdShade.ConnectableAPI(
                        stage.GetPrimAtPath('/Material/NodeGraph'))
        nested1 = UsdShade.ConnectableAPI(
                        stage.GetPrimAtPath('/Material/NodeGraph/NestedNode1'))
        nested2 = UsdShade.ConnectableAPI(
                        stage.GetPrimAtPath('/Material/NodeGraph/NestedNode2'))
        superNested = UsdShade.ConnectableAPI(
                        stage.GetPrimAtPath('/Material/NodeGraph/NestedNodeGraph'
                                            '/NestedNodeGraph2/SuperNestedNode'))

        # note, serval inputs resolve to the same attributes. Please see the
        # test.usda file to make sense of these connections

        # ----------------------------------------------------------------------
        # resolve top level inputs
        # ----------------------------------------------------------------------
        self._test(material, 'topLevelValue', Input,
                   '/Material.inputs:topLevelValue', 'TopLevelValue')

        # ----------------------------------------------------------------------
        # resolve inputs on terminal node
        # ----------------------------------------------------------------------
        self._test(terminal, 'terminalInput1', Output,
                   '/Material/NodeGraph/NestedNode1.outputs:nestedOut1')

        self._test(terminal, 'terminalInput2', Input,
                   '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        self._test(terminal, 'terminalInput3', Output,
                   '/Material/RegularNode.outputs:nodeOutput')

        self._test(terminal, 'terminalInput4', Input,
                   '/Material.inputs:topLevelValue', 'TopLevelValue')

        self._test(terminal, 'terminalInput5', Output,
                   '/Material/NodeGraph/NestedNodeGraph/NestedNodeGraph2'
                   '/SuperNestedNode.outputs:superNestedOut')

        self._test(terminal, 'terminalInput6', Input,
                   '/Material.inputs:topLevelValue', 'TopLevelValue')

        # ----------------------------------------------------------------------
        # resolve inputs on node graph node
        # ----------------------------------------------------------------------
        self._test(nodeGraph, 'nodeGraphVal', Input,
                   '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        self._test(nodeGraph, 'nodeGraphIn1', Output,
                   '/Material/RegularNode.outputs:nodeOutput')

        self._test(nodeGraph, 'nodeGraphIn2', Input,
                   '/Material.inputs:topLevelValue', 'TopLevelValue')

        # ----------------------------------------------------------------------
        # resolve inputs on nested nodes
        # ----------------------------------------------------------------------
        self._test(nested1, 'nestedIn1', Input,
                   '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        self._test(nested2, 'nestedIn2', Output,
                   '/Material/RegularNode.outputs:nodeOutput')

        self._test(superNested, 'superNestedIn', Input,
                   '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        # ----------------------------------------------------------------------
        # resolve invalid inputs
        # ----------------------------------------------------------------------
        self._testInvalid(terminal, 'terminalInput7')
        self._testInvalid(terminal, 'terminalInput8')

        # ----------------------------------------------------------------------
        # resolve inputs with a cycle
        # ----------------------------------------------------------------------
        # This will issue a warning (TF_WARN):
        # GetValueProducingAttribute:
        #    Found cycle with attribute /Material/NodeGraph.inputs:cycleA
        self._testInvalid(nodeGraph, 'cycleA')
        # This will issue a warning (TF_WARN):
        # GetValueProducingAttribute:
        #    Found cycle with attribute /Material/NodeGraph.inputs:deepCycleA
        self._testInvalid(nodeGraph, 'deepCycleA')
        # This will issue a warning (TF_WARN):
        # GetValueProducingAttribute:
        #    Found cycle with attribute /Material/NodeGraph.inputs:selfLoop
        self._testInvalid(nodeGraph, 'selfLoop')

        # ----------------------------------------------------------------------
        # resolve multi-connection inputs
        # ----------------------------------------------------------------------
        self._testMulti(terminal, 'multiInput1',
                        [Output, Output],
                        ['/Material/NodeGraph/NestedNode1.outputs:nestedOut1',
                         '/Material/RegularNode.outputs:nodeOutput'])
        self._testMulti(terminal, 'multiInput2',
                        [Output, Output],
                        ['/Material/NodeGraph/NestedNode1.outputs:nestedOut1',
                         '/Material/NodeGraph/NestedNode2.outputs:nestedOut2'])
        self._testMulti(terminal, 'multiInput3',
                        [Input, Output, Output, Output],
                        ['/Material/NodeGraph.inputs:nodeGraphVal',
                         '/Material/NodeGraph/NestedNode1.outputs:nestedOut1',
                         '/Material/NodeGraph/NestedNode2.outputs:nestedOut2',
                         '/Material/RegularNode.outputs:nodeOutput'],
                         ['NodeGraphValue', None, None, None])

        # ----------------------------------------------------------------------
        # test resolution with the shaderOutputsOnly=True mode
        # ----------------------------------------------------------------------
        # Test Outputs
        # Finding the upstream attribute that is an output on a shader
        surfaceGood = material.GetOutput('surfaceGood')
        attrs = UsdShade.Utils.GetValueProducingAttributes(surfaceGood,
                                                    shaderOutputsOnly=True)
        self.assertEqual(len(attrs), 1)
        self.assertEqual(attrs[0].GetPath(),
                         Sdf.Path('/Material/Terminal.outputs:bxdfOut'))

        # Make sure it does not find upstream attributes that are not shader
        # outputs and carry values
        surfaceBad = material.GetOutput('surfaceBad')
        attrs = UsdShade.Utils.GetValueProducingAttributes(surfaceBad,
                                                    shaderOutputsOnly=True)
        self.assertEqual(attrs, [])

        # Test Inputs
        # Finding the upstream attribute that is an output of a shader
        inputGood = terminal.GetInput('terminalInput1')
        attrs = UsdShade.Utils.GetValueProducingAttributes(inputGood,
                                                    shaderOutputsOnly=True)
        self.assertEqual(len(attrs), 1)
        self.assertEqual(attrs[0].GetPath(),
                Sdf.Path('/Material/NodeGraph/NestedNode1.outputs:nestedOut1'))

        # Make sure it does not find upstream attributes that are not shader
        # outputs and carry values
        inputBad = terminal.GetInput('terminalInput2')
        attrs = UsdShade.Utils.GetValueProducingAttributes(inputBad,
                                                    shaderOutputsOnly=True)
        self.assertEqual(attrs, [])


if __name__ == '__main__':
    unittest.main()
