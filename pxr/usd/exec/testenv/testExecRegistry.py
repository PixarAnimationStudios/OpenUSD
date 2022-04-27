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
from __future__ import print_function

from pxr import Plug
from pxr import Sdf
from pxr import Exec
from pxr import Tf

import os
import unittest

# The test discovery plugin is installed relative to this script
testRoot = os.path.join(os.path.dirname(__file__), 'ExecPlugins')
testPluginsDsoSearch = testRoot + '/lib/*/Resources/'

class TestExecNode(unittest.TestCase):
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
        cls.reg = Exec.Registry()

        # Set up the test parser plugins.
        cls.reg.SetExtraParserPlugins([cls.tppType, cls.tpp2Type])

        # We will register the discovery plugins one by one so that we can check
        # source types are not duplicated in the registry if we have plugins
        # that discover nodes of the same source type

        # The _NdrTestDiscoveryPlugin should find discovery results that have
        # source types of RmanCpp and OSL
        cls.reg.SetExtraDiscoveryPlugins([cls.tdpType])
        assert cls.reg.GetAllNodeSourceTypes() == [cls.oslType, cls.argsType]

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
        assert self.reg.GetAllNodeSourceTypes() == \
            [self.oslType, self.argsType, self.glslfxType]

        # Calling ExecRegistry::GetExecNodesByFamily() will actually parse the
        # discovery results.
        # Notice that in the five node names we find, we get 'TestNodeSameName'
        # twice because there are two nodes with different source types that
        # have the same name.
        # Notice that we do not see 'TestNodeGLSLFX' because we don't have a
        # parser plugin to support it
        nodes = self.reg.GetExecNodesByFamily()
        execNodeNames = [node.GetName() for node in nodes]
        assert set(execNodeNames) == {
            "TestNodeARGS",
            "TestNodeARGS2",
            "TestNodeOSL",
            "TestNodeSameName",
            "TestNodeSameName"
        }

        assert self.reg.GetSearchURIs() == ["/TestSearchPath", "/TestSearchPath2"]

        # Calling ExecRegistry::GetNodeNames only looks at discovery results
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

        assert id(self.reg.GetExecNodeByName(nodes[0].GetName())) == id(nodes[0])

        nodeName = "TestNodeSameName"
        nodeIdentifier = "TestNodeSameName"
        nodeAlias = "Alias_TestNodeSameName"

        # Ensure that the registry can retrieve two nodes of the same name but
        # different source types
        assert len(self.reg.GetExecNodesByName(nodeName)) == 2
        node = self.reg.GetExecNodeByNameAndType(nodeName, self.oslType)
        assert node is not None
        node = self.reg.GetExecNodeByNameAndType(nodeName, self.argsType)
        assert node is not None
        node = self.reg.GetExecNodeByName(nodeName, [self.oslType, self.argsType])
        assert node.GetSourceType() == self.oslType
        node = self.reg.GetExecNodeByName(nodeName, [self.argsType, self.oslType])
        assert node.GetSourceType() == self.argsType

        # Ensure that the registry can retrieve these same nodes via identifier,
        # which, in these cases, are the same as the node names.
        assert len(self.reg.GetExecNodesByIdentifier(nodeIdentifier)) == 2
        node = self.reg.GetExecNodeByIdentifierAndType(nodeIdentifier, self.oslType)
        assert node is not None
        node = self.reg.GetExecNodeByIdentifierAndType(nodeIdentifier, self.argsType)
        assert node is not None
        node = self.reg.GetExecNodeByIdentifier(nodeIdentifier, [self.oslType, self.argsType])
        assert node.GetSourceType() == self.oslType
        node = self.reg.GetExecNodeByIdentifier(nodeIdentifier, [self.argsType, self.oslType])
        assert node.GetSourceType() == self.argsType

        # Test aliases. The discovery result for the args type 
        # "TestNodeSameName" has been given an alias by its discovery plugin
        # (see TestExecPlugin_discoveryPlugin.cpp).
        # The args type node can be found by its alias through the 
        # GetExecNodeByIdentifier APIs. The osl type node is not found 
        # by this alias
        assert len(self.reg.GetExecNodesByIdentifier(nodeAlias)) == 1
        node = self.reg.GetExecNodeByIdentifierAndType(nodeAlias, self.oslType)
        assert node is None
        node = self.reg.GetExecNodeByIdentifierAndType(nodeAlias, self.argsType)
        assert node is not None
        assert node.GetIdentifier() == nodeIdentifier
        assert node.GetSourceType() == self.argsType
        node = self.reg.GetExecNodeByIdentifier(nodeAlias, [self.oslType, self.argsType])
        assert node.GetIdentifier() == nodeIdentifier
        assert node.GetSourceType() == self.argsType
        node = self.reg.GetExecNodeByIdentifier(nodeAlias, [self.argsType, self.oslType])
        assert node.GetIdentifier() == nodeIdentifier
        assert node.GetSourceType() == self.argsType

        # Ensure that GetExecNodeByName APIs are NOT able to find nodes using 
        # aliases.
        assert len(self.reg.GetExecNodesByName(nodeAlias)) == 0
        node = self.reg.GetExecNodeByNameAndType(nodeAlias, self.oslType)
        assert node is None
        node = self.reg.GetExecNodeByNameAndType(nodeAlias, self.argsType)
        assert node is None
        node = self.reg.GetExecNodeByName(nodeAlias, [self.oslType, self.argsType])
        assert node is None
        node = self.reg.GetExecNodeByName(nodeAlias, [self.argsType, self.oslType])
        assert node is None

        # Test GetExecNodeFromAsset to check that a subidentifier is part of
        # the node's identifier if one is specified
        node = self.reg.GetExecNodeFromAsset(
            Sdf.AssetPath('TestNodeSourceAsset.oso'),   # execAsset
            {},                                         # metadata
            "mySubIdentifier")                          # subIdentifier
        assert node.GetIdentifier().endswith("<mySubIdentifier><>")
        assert node.GetName() == "TestNodeSourceAsset.oso"

        # Test GetExecNodeFromAsset to check that a sourceType is part of
        # the node's identifier if one is specified
        node = self.reg.GetExecNodeFromAsset(
            Sdf.AssetPath('TestNodeSourceAsset.oso'),   # execAsset
            {},                                         # metadata
            "mySubIdentifier",                          # subIdentifier
            "OSL")                                      # sourceType
        assert node.GetIdentifier().endswith("<mySubIdentifier><OSL>")

    def test_UsdEncoding(self):
        # The two nodes have the same set of properties, as defined in the mock
        # OSL parser plugin of this test. But `TestNodeSameName` has been tagged
        # with execUsdEncodingVersion=0 in the mock discovery plugin and hence
        # should produce a different mapping from Exec to Sdf types
        nodeNew = self.reg.GetExecNodeByNameAndType("TestNodeOSL", self.oslType)
        assert nodeNew is not None
        nodeOld = self.reg.GetExecNodeByNameAndType("TestNodeSameName", self.oslType)
        assert nodeOld is not None
        assert nodeOld.GetMetadata()['execUsdEncodingVersion'] == "0"

        def _CheckTypes(node, expectedTypes):
            for inputName in node.GetInputNames():
                prop = node.GetInput(inputName)
                execType = prop.GetType()
                sdfType, sdfHint = prop.GetTypeAsSdfType()
                expectedExecType, expectedSdfType = expectedTypes[prop.GetName()]
                print("  ", prop.GetName(), execType, str(sdfType), 'vs expected', \
                      expectedExecType, str(expectedSdfType))
                if not (execType == expectedExecType and sdfType == expectedSdfType):
                    print("     MISMATCH")
                assert execType == expectedExecType and sdfType == expectedSdfType

        expectedTypes = {
           'IntProperty': (Exec.PropertyTypes.Int, Sdf.ValueTypeNames.Int),
           'StringProperty': (Exec.PropertyTypes.String, Sdf.ValueTypeNames.String),
           'FloatProperty': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.Float),
           'ColorProperty': (Exec.PropertyTypes.Color, Sdf.ValueTypeNames.Color3f),
           'PointProperty': (Exec.PropertyTypes.Point, Sdf.ValueTypeNames.Point3f),
           'NormalProperty': (Exec.PropertyTypes.Normal, Sdf.ValueTypeNames.Normal3f),
           'VectorProperty': (Exec.PropertyTypes.Vector, Sdf.ValueTypeNames.Vector3f),
           'MatrixProperty': (Exec.PropertyTypes.Matrix, Sdf.ValueTypeNames.Matrix4d),
           'StructProperty': (Exec.PropertyTypes.Struct, Sdf.ValueTypeNames.Token),
           'TerminalProperty': (Exec.PropertyTypes.Terminal, Sdf.ValueTypeNames.Token),
           'Float_Vec2Property': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.Float2),
           'Float_Vec3Property': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.Float3),
           'Float_Vec4Property': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.Float4),
           'String_AssetProperty': (Exec.PropertyTypes.String, Sdf.ValueTypeNames.Asset),
        }
        _CheckTypes(nodeNew, expectedTypes)

        expectedTypes.update({
           'StructProperty': (Exec.PropertyTypes.Struct, Sdf.ValueTypeNames.String),
           'Float_Vec2Property': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.FloatArray),
           'Float_Vec3Property': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.FloatArray),
           'Float_Vec4Property': (Exec.PropertyTypes.Float, Sdf.ValueTypeNames.FloatArray),
           'String_AssetProperty': (Exec.PropertyTypes.String, Sdf.ValueTypeNames.String),
        })
        _CheckTypes(nodeOld, expectedTypes)

if __name__ == '__main__':
    unittest.main()
