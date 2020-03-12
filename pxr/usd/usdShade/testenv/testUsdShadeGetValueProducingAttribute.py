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

class TestUsdShadeGetValueProducingAttribute(unittest.TestCase):

    def _check(self, attr, expectedPath, expectedValue=None):
        print("CHECK:", attr, "vs", expectedPath)
        self.assertTrue(attr)
        if 'inputs:' in expectedPath:
            self.assertTrue(UsdShade.Input.IsInput(attr))
        else:
            self.assertTrue(UsdShade.Output.IsOutput(attr))
        self.assertEqual(attr.GetPath(), Sdf.Path(expectedPath))
        self.assertEqual(attr.Get(), expectedValue)

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
        attr, attrType = \
            material.GetInput('topLevelValue').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material.inputs:topLevelValue', 'TopLevelValue')

        # ----------------------------------------------------------------------
        # resolve inputs on terminal node
        # ----------------------------------------------------------------------
        attr, attrType = \
            terminal.GetInput('terminalInput1').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Output)
        self._check(attr, '/Material/NodeGraph/NestedNode1.outputs:nestedOut1')

        attr, attrType = \
            terminal.GetInput('terminalInput2').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        attr, attrType = \
            terminal.GetInput('terminalInput3').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Output)
        self._check(attr, '/Material/RegularNode.outputs:nodeOutput')

        attr, attrType = \
            terminal.GetInput('terminalInput4').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material.inputs:topLevelValue', 'TopLevelValue')

        attr, attrType = \
            terminal.GetInput('terminalInput5').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Output)
        self._check(attr, '/Material/NodeGraph/NestedNodeGraph/NestedNodeGraph2'
                          '/SuperNestedNode.outputs:superNestedOut')

        attr, attrType = \
            terminal.GetInput('terminalInput6').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material.inputs:topLevelValue', 'TopLevelValue')

        # ----------------------------------------------------------------------
        # resolve inputs on node graph node
        # ----------------------------------------------------------------------
        attr, attrType = \
            nodeGraph.GetInput('nodeGraphVal').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        attr, attrType = \
            nodeGraph.GetInput('nodeGraphIn1').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Output)
        self._check(attr, '/Material/RegularNode.outputs:nodeOutput')

        attr, attrType = \
            nodeGraph.GetInput('nodeGraphIn2').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material.inputs:topLevelValue', 'TopLevelValue')

        # ----------------------------------------------------------------------
        # resolve inputs on nested nodes
        # ----------------------------------------------------------------------
        attr, attrType = \
            nested1.GetInput('nestedIn1').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        attr, attrType = \
            nested2.GetInput('nestedIn2').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Output)
        self._check(attr, '/Material/RegularNode.outputs:nodeOutput')

        attr, attrType = \
            superNested.GetInput('superNestedIn').GetValueProducingAttribute()
        self.assertEqual(attrType, UsdShade.AttributeType.Input)
        self._check(attr, '/Material/NodeGraph.inputs:nodeGraphVal', 'NodeGraphValue')

        # ----------------------------------------------------------------------
        # resolve invalid inputs
        # ----------------------------------------------------------------------
        attr, attrType = \
            terminal.GetInput('terminalInput7').GetValueProducingAttribute()
        self.assertFalse(attr)
        self.assertEqual(attrType, UsdShade.AttributeType.Invalid)
        attr, attrType = \
            terminal.GetInput('terminalInput8').GetValueProducingAttribute()
        self.assertFalse(attr)
        self.assertEqual(attrType, UsdShade.AttributeType.Invalid)

        # ----------------------------------------------------------------------
        # resolve inputs with a cycle
        # ----------------------------------------------------------------------
        # This will issue a warning (TF_WARN):
        # GetValueProducingAttribute:
        #    Found cycle with attribute /Material/NodeGraph.inputs:cycleA
        attr, attrType = \
            nodeGraph.GetInput('cycleA').GetValueProducingAttribute()
        self.assertFalse(attr)
        self.assertEqual(attrType, UsdShade.AttributeType.Invalid)
        # This will issue a warning (TF_WARN):
        # GetValueProducingAttribute:
        #    Found cycle with attribute /Material/NodeGraph.inputs:deepCycleA
        attr, attrType = \
            nodeGraph.GetInput('deepCycleA').GetValueProducingAttribute()
        self.assertFalse(attr)
        self.assertEqual(attrType, UsdShade.AttributeType.Invalid)
        # This will issue a warning (TF_WARN):
        # GetValueProducingAttribute:
        #    Found cycle with attribute /Material/NodeGraph.inputs:selfLoop
        attr, attrType = \
            nodeGraph.GetInput('selfLoop').GetValueProducingAttribute()
        self.assertFalse(attr)
        self.assertEqual(attrType, UsdShade.AttributeType.Invalid)


if __name__ == '__main__':
    unittest.main()
