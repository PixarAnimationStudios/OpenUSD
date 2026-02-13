#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# Disable automatic parser plugin discovery. We'll install our own parser
# plugin later to ensure its the only one used during the test.
import os
os.environ['PXR_SDR_SKIP_PARSER_PLUGIN_DISCOVERY'] = "1"

import unittest
from pxr import Plug, Sdr
from pxr.Sdr import shaderParserTestUtils as utils

class TestShaderNode(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parser = Plug.Registry().FindTypeByName("SdrOslParserPlugin")
        assert parser
        Sdr.Registry().SetExtraParserPlugins([parser])

        cls.uri = "TestNodeOSL.oso"
        cls.resolvedUri = os.path.abspath(cls.uri)

        cls.sourceCode="TestNode source code"
        cls.metadata = {"extra": "extraMetadata", 
                      "primvars":"a|b|c"}
        cls.blindData = "unused blind data"

        discoveryResult = Sdr.NodeDiscoveryResult(
            "TestNodeOSL",   # Identifier
            Sdr.Version(),   # Version
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

        Sdr.Registry().AddDiscoveryResult(discoveryResult)
        cls.node = Sdr.Registry().GetShaderNodeByIdentifier('TestNodeOSL')
        assert cls.node is not None

    def test_Basic(self):
        """
        Tests all node and property methods that originate from Sdr and are not
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

        discoveryResult = Sdr.NodeDiscoveryResult(
            "TestShaderPropertiesNodeOSL",  # Identifier
            Sdr.Version(),                  # Version
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

        Sdr.Registry().AddDiscoveryResult(discoveryResult)
        node = Sdr.Registry().GetShaderNodeByIdentifier(
            'TestShaderPropertiesNodeOSL')
        assert node is not None

        utils.TestShaderPropertiesNode(node)

if __name__ == '__main__':
    unittest.main()
