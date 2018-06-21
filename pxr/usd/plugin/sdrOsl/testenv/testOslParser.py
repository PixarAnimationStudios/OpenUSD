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

import unittest
from pxr import Ndr
from pxr import SdrOsl
from pxr.Sdr import shaderParserTestUtils as utils


class TestShaderNode(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.URI = "TestNodeOSL.oso"

        discoveryResult = Ndr.NodeDiscoveryResult(
            "TestNodeOSL",   # Identifier
            Ndr.Version(),   # Version
            "TestNodeOSL",   # Name
            "",              # Family
            "oso",           # Discovery type (extension)
            "OSL",           # Source type
            cls.URI,         # URI
            cls.URI          # Resolved URI
        )

        cls.node = SdrOsl.OslParser().Parse(discoveryResult)

        assert cls.node is not None

    def test_Basic(self):
        """
        Tests all node and property methods that originate from Ndr and are not
        shading-specific, but still need to be tested to ensure the parser did
        its job correctly.
        """

        utils.TestBasicNode(self.node,
                            SdrOsl.OslParser.SourceType,
                            self.URI)

    def test_ShaderSpecific(self):
        """
        Tests all shading-specific methods on the node and property.
        """

        utils.TestShaderSpecificNode(self.node)


if __name__ == '__main__':
    unittest.main()
