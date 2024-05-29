#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
from __future__ import print_function

from pxr import Plug
from pxr import Sdf
from pxr import Sdr
from pxr import Tf

import os
import unittest

# The test discovery plugin is installed relative to this script
testRoot = os.path.join(os.path.dirname(__file__), 'SdrPlugins')
testPluginsDsoSearch = testRoot + '/lib/*/Resources/'

class TestShaderNode(unittest.TestCase):
    # The following source types are what we expect to discover from
    # _NdrTestDiscoveryPlugin and _NdrTestDiscoveryPlugin2.  Note that there
    # is no glslfx parser plugin provided in this test.
    argsType = "RmanCpp"
    oslType = "OSL"
    glslfxType = "glslfx"

    @classmethod
    def setUpClass(cls):
        """
        Load the test modules for discovery and parsing and check basic type
        registration
        """

        # Register test plugins and verify they have been found
        cls.pr = Plug.Registry()
        plugins = cls.pr.RegisterPlugins(testPluginsDsoSearch)

        # Verify the test plugins have been found.  When building monolithic
        # we should find at least these derived types.
        assert len(plugins) == 1
        cls.tdpType  = Tf.Type.FindByName('_NdrTestDiscoveryPlugin')
        cls.tdp2Type = Tf.Type.FindByName('_NdrTestDiscoveryPlugin2')

        cls.tppType = Tf.Type.FindByName('_NdrArgsTestParserPlugin')
        cls.tpp2Type = Tf.Type.FindByName('_NdrOslTestParserPlugin')

        # We don't check for all the derived types of NdrDiscoveryPlugin
        # because this test only uses the discovery and parser plugins 
        # that are defined in this testenv
        assert {cls.tdpType, cls.tdp2Type}.issubset(
            set(cls.pr.GetAllDerivedTypes('NdrDiscoveryPlugin')))
        assert {cls.tppType, cls.tpp2Type}.issubset(
            set(cls.pr.GetAllDerivedTypes('NdrParserPlugin')))

            # Instantiating the registry will kick off the discovery process.
        # This test assumes the PXR_NDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY 
        # and PXR_NDR_SKIP_PARSER_PLUGIN_DISCOVERY has been set prior to 
        # being run to ensure built-in plugins are not found. Instead 
        # we'll list the plugins we want explicitly.

        # Setting this from within the script does not work on Windows.
        # os.environ["PXR_NDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY"] = ""
        # os.environ["PXR_NDR_SKIP_PARSER_PLUGIN_DISCOVERY"] = ""
        cls.reg = Sdr.Registry()

        # Set up the test parser plugins.
        cls.reg.SetExtraParserPlugins([cls.tppType, cls.tpp2Type])

        # We will register the discovery plugins one by one so that we can check
        # source types are not duplicated in the registry if we have plugins
        # that discover nodes of the same source type

        # The _NdrTestDiscoveryPlugin should find discovery results that have
        # source types of RmanCpp and OSL
        cls.reg.SetExtraDiscoveryPlugins([cls.tdpType])
        assert sorted(cls.reg.GetAllNodeSourceTypes()) == \
            [cls.oslType, cls.argsType]

        # The _NdrTestDiscoveryPlugin2 should find discovery results that have
        # source types of RmanCpp and glslfx
        cls.reg.SetExtraDiscoveryPlugins([cls.tdp2Type])

    def test_Registry(self):
        """
        Test basic registry operations. Also ensures that the discovery process
        works correctly.
        """

        # Test that the registry does not see 'RmanCpp' twice as a source type,
        # and that it finds 'glslfx' as a source type
        assert sorted(self.reg.GetAllNodeSourceTypes()) == \
            [self.oslType, self.argsType, self.glslfxType]

        # Calling SdrRegistry::GetShaderNodesByFamily() will actually parse the
        # discovery results.
        # Notice that in the five node names we find, we get 'TestNodeSameName'
        # twice because there are two nodes with different source types that
        # have the same name.
        # Notice that we do not see 'TestNodeGLSLFX' because we don't have a
        # parser plugin to support it
        nodes = self.reg.GetShaderNodesByFamily()
        shaderNodeNames = [node.GetName() for node in nodes]
        assert set(shaderNodeNames) == {
            "TestNodeARGS",
            "TestNodeARGS2",
            "TestNodeOSL",
            "TestNodeSameName",
            "TestNodeSameName"
        }

        assert self.reg.GetSearchURIs() == ["/TestSearchPath", "/TestSearchPath2"]

        # Calling SdrRegistry::GetNodeNames only looks at discovery results
        # without parsing them.
        # Notice that we get 'TestNodeSameName' only once because we only show
        # unique names.
        # Notice that we see 'TestNodeGLSLFX' because it is in our discovery
        # results even though we do not have a parser plugin that supports its
        # source type.
        assert set(self.reg.GetNodeNames()) == {
            "TestNodeARGS",
            "TestNodeARGS2",
            "TestNodeOSL",
            "TestNodeSameName",
            "TestNodeGLSLFX"
        }
        # Verify that GetNodeIdentifiers follows the same rules as GetNodeNames.
        # Note that the names and identifiers do happen to be the same in this
        # test case which is common.
        assert set(self.reg.GetNodeIdentifiers()) == {
            "TestNodeARGS",
            "TestNodeARGS2",
            "TestNodeOSL",
            "TestNodeSameName",
            "TestNodeGLSLFX"
        }

        assert id(self.reg.GetShaderNodeByName(nodes[0].GetName())) == id(nodes[0])

        nodeName = "TestNodeSameName"
        nodeIdentifier = "TestNodeSameName"

        # Ensure that the registry can retrieve two nodes of the same name but
        # different source types
        assert len(self.reg.GetShaderNodesByName(nodeName)) == 2
        node = self.reg.GetShaderNodeByNameAndType(nodeName, self.oslType)
        assert node is not None
        node = self.reg.GetShaderNodeByNameAndType(nodeName, self.argsType)
        assert node is not None
        node = self.reg.GetShaderNodeByName(nodeName, [self.oslType, self.argsType])
        assert node.GetSourceType() == self.oslType
        node = self.reg.GetShaderNodeByName(nodeName, [self.argsType, self.oslType])
        assert node.GetSourceType() == self.argsType

        # Ensure that the registry can retrieve these same nodes via identifier,
        # which, in these cases, are the same as the node names.
        assert len(self.reg.GetShaderNodesByIdentifier(nodeIdentifier)) == 2
        node = self.reg.GetShaderNodeByIdentifierAndType(nodeIdentifier, self.oslType)
        assert node is not None
        node = self.reg.GetShaderNodeByIdentifierAndType(nodeIdentifier, self.argsType)
        assert node is not None
        node = self.reg.GetShaderNodeByIdentifier(nodeIdentifier, [self.oslType, self.argsType])
        assert node.GetSourceType() == self.oslType
        node = self.reg.GetShaderNodeByIdentifier(nodeIdentifier, [self.argsType, self.oslType])
        assert node.GetSourceType() == self.argsType

        # Test GetShaderNodeFromAsset to check that a subidentifier is part of
        # the node's identifier if one is specified
        node = self.reg.GetShaderNodeFromAsset(
            Sdf.AssetPath('TestNodeSourceAsset.oso'),   # shaderAsset
            {},                                         # metadata
            "mySubIdentifier")                          # subIdentifier
        assert node.GetIdentifier().endswith("<mySubIdentifier><>")
        assert node.GetName() == "TestNodeSourceAsset.oso"

        # Test GetShaderNodeFromAsset to check that a sourceType is part of
        # the node's identifier if one is specified
        node = self.reg.GetShaderNodeFromAsset(
            Sdf.AssetPath('TestNodeSourceAsset.oso'),   # shaderAsset
            {},                                         # metadata
            "mySubIdentifier",                          # subIdentifier
            "OSL")                                      # sourceType
        assert node.GetIdentifier().endswith("<mySubIdentifier><OSL>")

    def test_UsdEncoding(self):
        # The two nodes have the same set of properties, as defined in the mock
        # OSL parser plugin of this test. But `TestNodeSameName` has been tagged
        # with sdrUsdEncodingVersion=0 in the mock discovery plugin and hence
        # should produce a different mapping from Sdr to Sdf types
        nodeNew = self.reg.GetShaderNodeByNameAndType("TestNodeOSL", self.oslType)
        assert nodeNew is not None
        nodeOld = self.reg.GetShaderNodeByNameAndType("TestNodeSameName", self.oslType)
        assert nodeOld is not None
        assert nodeOld.GetMetadata()['sdrUsdEncodingVersion'] == "0"

        def _CheckTypes(node, expectedTypes):
            for inputName in node.GetInputNames():
                prop = node.GetInput(inputName)
                sdrType = prop.GetType()
                sdfType, sdfHint = prop.GetTypeAsSdfType()
                expectedSdrType, expectedSdfType = expectedTypes[prop.GetName()]
                print("  ", prop.GetName(), sdrType, str(sdfType), 'vs expected', \
                      expectedSdrType, str(expectedSdfType))
                if not (sdrType == expectedSdrType and sdfType == expectedSdfType):
                    print("     MISMATCH")
                assert sdrType == expectedSdrType and sdfType == expectedSdfType

        print("Current USD encoding:")
        expectedTypes = {
           'IntProperty': (Sdr.PropertyTypes.Int, Sdf.ValueTypeNames.Int),
           'StringProperty': (Sdr.PropertyTypes.String, Sdf.ValueTypeNames.String),
           'FloatProperty': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.Float),
           'ColorProperty': (Sdr.PropertyTypes.Color, Sdf.ValueTypeNames.Color3f),
           'PointProperty': (Sdr.PropertyTypes.Point, Sdf.ValueTypeNames.Point3f),
           'NormalProperty': (Sdr.PropertyTypes.Normal, Sdf.ValueTypeNames.Normal3f),
           'VectorProperty': (Sdr.PropertyTypes.Vector, Sdf.ValueTypeNames.Vector3f),
           'MatrixProperty': (Sdr.PropertyTypes.Matrix, Sdf.ValueTypeNames.Matrix4d),
           'StructProperty': (Sdr.PropertyTypes.Struct, Sdf.ValueTypeNames.Token),
           'TerminalProperty': (Sdr.PropertyTypes.Terminal, Sdf.ValueTypeNames.Token),
           'VstructProperty': (Sdr.PropertyTypes.Vstruct, Sdf.ValueTypeNames.Token),
           'Vstruct_ArrayProperty': (Sdr.PropertyTypes.Vstruct, Sdf.ValueTypeNames.TokenArray),
           'Float_VstructProperty': (Sdr.PropertyTypes.Vstruct, Sdf.ValueTypeNames.TokenArray),
           'Float_Vec2Property': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.Float2),
           'Float_Vec3Property': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.Float3),
           'Float_Vec4Property': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.Float4),
           'String_AssetProperty': (Sdr.PropertyTypes.String, Sdf.ValueTypeNames.Asset),
        }
        _CheckTypes(nodeNew, expectedTypes)

        print("Version 0 USD encoding:")
        # These are the differences relative to the public encoding
        expectedTypes.update({
           'StructProperty': (Sdr.PropertyTypes.Struct, Sdf.ValueTypeNames.String),
           'VstructProperty': (Sdr.PropertyTypes.Vstruct, Sdf.ValueTypeNames.Float),
           'Vstruct_ArrayProperty': (Sdr.PropertyTypes.Vstruct, Sdf.ValueTypeNames.FloatArray),
           'Float_Vec2Property': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.FloatArray),
           'Float_Vec3Property': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.FloatArray),
           'Float_Vec4Property': (Sdr.PropertyTypes.Float, Sdf.ValueTypeNames.FloatArray),
           'String_AssetProperty': (Sdr.PropertyTypes.String, Sdf.ValueTypeNames.String),
           'Float_VstructProperty': (Sdr.PropertyTypes.Vstruct, Sdf.ValueTypeNames.FloatArray),
        })
        _CheckTypes(nodeOld, expectedTypes)

if __name__ == '__main__':
    unittest.main()
