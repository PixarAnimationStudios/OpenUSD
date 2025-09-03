#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest

from pxr import UsdShade, Usd

class TestSdrShownIfConversion(unittest.TestCase):
    def test_SdrShownIfConversion(self):
        """
        Ensure that conditional visability metadata is converted to shownIf
        expressions.
        """

        expectedNodes = {
            # test converting from conditionalVis metadata
            'TestConversion': {
                # basic metadata
                'input1': 'numOps >= 1',
                # recursive metadata
                'input2': 'numOps >= 2 && input1 < 10',
            },

            # test preservation of explicit shownIf expressions.
            # this node definition has the same conditionalVis metadata
            # as 'TestConversion' but also has 'shownIf' expressions with
            # specific formatting (additional parenthesis for illustration)
            'TestPassThrough': {
                'input1': '(numOps >= 1)',
                'input2': '((numOps >= 2) && (input1 < 10))',
            },
        }

        for (nodeName, inputs) in expectedNodes.items():
            print(f'Testing {nodeName}')

            stage = Usd.Stage.Open(f'{nodeName}.usda')
            shaderDef = UsdShade.Shader.Get(stage, f'/{nodeName}')
            discoveredShaders = UsdShade.ShaderDefUtils.GetDiscoveryResults(shaderDef, stage.GetRootLayer().realPath)
            node = UsdShade.ShaderDefParserPlugin().ParseShaderNode(discoveredShaders[0])
            assert node is not None

            for (inputName, expected) in inputs.items():
                print(f'  {inputName}')
                input = node.GetShaderInput(inputName)
                assert input is not None
                assert input.GetShownIf() == expected

if __name__ == '__main__':
    unittest.main()
