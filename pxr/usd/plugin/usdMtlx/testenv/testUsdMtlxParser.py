#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

import os
os.environ['PXR_USDMTLX_PLUGIN_SEARCH_PATHS'] = os.getcwd()

from pxr import Tf, Sdr
import unittest

class TestParser(unittest.TestCase):
    def test_NodeParser(self):
        """
        Test MaterialX node parser.
        """
        # Find our nodes.
        nodes = Sdr.Registry().GetShaderNodesByFamily('UsdMtlxTestNode')
        self.assertEqual(sorted([node.GetName() for node in nodes]), [
            'nd_boolean',
            'nd_customtype',
            'nd_float',
            'nd_integer',
            'nd_string',
            'nd_surface',
            'nd_vector',
        ])

        # Verify common info.
        for node in nodes:
            self.assertEqual(node.GetSourceType(), "OSL")
            self.assertEqual(node.GetFamily(), "UsdMtlxTestNode")
            self.assertEqual(sorted(node.GetInputNames()), ["in", "note"])
            if node.GetName() == 'nd_surface':
                # XXX -- Should shader semantic nodes have a named output?
                self.assertEqual(node.GetOutputNames(), [])
            else:
                self.assertEqual(node.GetOutputNames(), ["result"])

        # Verify converted types.
        typeNameMap = {
            'boolean': 'bool',
            'customtype': 'customtype',
            'float': 'float',
            'integer': 'int',
            'string': 'string',
            'surface': 'string',
            'vector': 'vector',
        }
        for node in nodes:
            # Strip leading nd_ from name.
            name = node.GetName()[3:]

            # Get the input.
            prop = node.GetInput('in')

            # Verify type.
            self.assertEqual(prop.GetType(), typeNameMap.get(name))

if __name__ == '__main__':
    unittest.main()
