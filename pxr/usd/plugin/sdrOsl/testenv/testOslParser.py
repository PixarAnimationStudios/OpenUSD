#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import os
import unittest
from pxr import Ndr
from pxr import SdrOsl
from pxr.Sdr import shaderParserTestUtils as utils

class TestShaderNode(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.uri = "TestNodeOSL.oso"
        cls.resolvedUri = os.path.abspath(cls.uri)

        cls.sourceCode="TestNode source code"
        cls.metadata = {"extra": "extraMetadata", 
                      "primvars":"a|b|c"}
        cls.blindData = "unused blind data"

        discoveryResult = Ndr.NodeDiscoveryResult(
            "TestNodeOSL",   # Identifier
            Ndr.Version(),   # Version
            "TestNodeOSL",   # Name
            "",              # Family
            "oso",           # Discovery type (extension)
            "OSL",           # Source type
            cls.uri,         # URI
            cls.resolvedUri, # Resolved URI
            sourceCode=cls.sourceCode,
            metadata=cls.metadata,
            blindData=cls.blindData,
            subIdentifier=""
        )

        cls.node = SdrOsl.OslParser().Parse(discoveryResult)
        assert cls.node is not None

    def test_Basic(self):
        """
        Tests all node and property methods that originate from Ndr and are not
        shading-specific, but still need to be tested to ensure the parser did
        its job correctly.
        """
        nodeMetadata = self.node.GetMetadata()
        assert nodeMetadata["extra"] == self.metadata["extra"]

        # The primvars value will be overridden by the parser plugin.
        assert nodeMetadata["primvars"] != self.metadata["primvars"]

        # Ensure that the source code gets copied.
        assert self.node.GetSourceCode() == self.sourceCode

        utils.TestBasicNode(self.node,
                            "OSL",
                            self.resolvedUri,
                            self.resolvedUri)

    def test_ShaderSpecific(self):
        """
        Tests all shading-specific methods on the node and property.
        """

        utils.TestShaderSpecificNode(self.node)

    def test_ShaderProperties(self):
        """
        Test property correctness on the "TestShaderPropertiesNodeOSL" node.

        See shaderParserTestUtils TestShaderPropertiesNode method for detailed
        description of the test.
        """
        uri = "TestShaderPropertiesNodeOSL.oso"
        resolvedUri = os.path.abspath(uri)
        sourceCode = ""
        metadata = {}
        blindData = ""
        subIdentifier = ""

        discoveryResult = Ndr.NodeDiscoveryResult(
            "TestShaderPropertiesNodeOSL",  # Identifier
            Ndr.Version(),                  # Version
            "TestShaderPropertiesNodeOSL",  # Name
            "",                             # Family
            "oso",                          # Discovery type (extension)
            "OSL",                          # Source type
            uri,                            # URI
            resolvedUri,                    # Resolved URI
            sourceCode,                     # sourceCode
            metadata,                       # metadata
            blindData,                      # blindData
            subIdentifier                   # subIdentifier
        )
        node = SdrOsl.OslParser().Parse(discoveryResult)
        assert node is not None

        utils.TestShaderPropertiesNode(node)

if __name__ == '__main__':
    unittest.main()
