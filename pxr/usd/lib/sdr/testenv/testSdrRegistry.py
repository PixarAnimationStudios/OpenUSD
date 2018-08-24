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

from pxr import Plug
from pxr import Sdr
from pxr import Tf

import os
import unittest

# The test discovery plugin is installed relative to this script
testRoot = os.path.join(os.path.dirname(__file__), 'SdrPlugins')
testPluginsDsoSearch = testRoot + '/lib/*/Resources/'

class TestShaderNode(unittest.TestCase):
    def test_Registry(self):
        """
        Test basic registry operations. Also ensures that the discovery process
        works correctly.
        """

        # Register test plugins and verify they have been found
        pr = Plug.Registry()
        plugins = pr.RegisterPlugins(testPluginsDsoSearch)

        # Verify the test plugins have been found.  When building monolithic
        # we should find at least these derived types.
        self.assertEqual(len(plugins), 1)
        fsdpType = Tf.Type.FindByName('_NdrFilesystemDiscoveryPlugin')
        tdpType  = Tf.Type.FindByName('_NdrTestDiscoveryPlugin')
        self.assertEqual(set([fsdpType, tdpType]) -
                         set(pr.GetAllDerivedTypes('NdrDiscoveryPlugin')),
                         set())
        self.assertEqual(set([Tf.Type.FindByName('_NdrArgsTestParserPlugin'),
                              Tf.Type.FindByName('_NdrOslTestParserPlugin')]) -
                         set(pr.GetAllDerivedTypes('NdrParserPlugin')),
                         set())

        # Instantiating the registry will kick off the discovery process.
        # Don't do the usual discovery plugin search in case this is a
        # monolithic build.  That will find any built-in plugins and
        # this test isn't expecting that.  Instead we'll list the plugins
        # we want explicitly.
        os.environ["PXR_NDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY"] = ""
        reg = Sdr.Registry()
        reg.SetExtraDiscoveryPlugins([fsdpType, tdpType])

        nodes = reg.GetShaderNodesByFamily()
        assert len(nodes) == 4
        assert reg.GetSearchURIs() == ["/TestSearchPath"]
        assert set(reg.GetNodeNames()) == {
            "TestNodeARGS", "TestNodeOSL", "TestNodeSameName"
        }
        assert id(reg.GetShaderNodeByURI(nodes[0].GetSourceURI())) == id(nodes[0])
        assert id(reg.GetShaderNodeByName(nodes[0].GetName())) == id(nodes[0])

        argsType = "RmanCpp"
        oslType = "OSL"

        # Ensure that the registry can retrieve two nodes of the same name but
        # different source types
        assert {argsType, oslType}.issubset(set(reg.GetAllNodeSourceTypes()))
        assert len(reg.GetShaderNodesByName("TestNodeSameName")) == 2
        assert reg.GetShaderNodeByNameAndType("TestNodeSameName", oslType) is not None
        assert reg.GetShaderNodeByNameAndType("TestNodeSameName", argsType) is not None
        assert reg.GetShaderNodeByName("TestNodeSameName", [oslType, argsType])\
                  .GetSourceType() == oslType
        assert reg.GetShaderNodeByName("TestNodeSameName", [argsType, oslType])\
                  .GetSourceType() == argsType

if __name__ == '__main__':
    unittest.main()
