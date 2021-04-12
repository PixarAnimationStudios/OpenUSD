#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

from pxr import Sdf, Pcp, Plug, Vt
import os, unittest

class TestPcpStreamingLayerReload(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        testRoot = os.path.join(os.path.dirname(__file__), 'PcpPlugins')
        testPluginsDso = testRoot + '/lib'
        testPluginsDsoSearch = testPluginsDso + '/*/Resources/'

        # Register dso plugins.  Discard possible exception due to
        # TestPlugDsoEmpty.  The exception only shows up here if it happens in
        # the main thread so we can't rely on it.
        try:
            Plug.Registry().RegisterPlugins(testPluginsDsoSearch)
        except RuntimeError:
            pass

    def setUp(self):
        # We expect there to be no layers left loaded when we start each test
        # case so we can start fresh. By the tearDown completes this needs to 
        # be true.
        self.assertFalse(Sdf.Layer.GetLoadedLayers())

    def _CreatePcpCache(self, rootLayer):
        return Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

    def test_StreamingLayerReload(self):

        # Open a layer with our streaming format.
        l = Sdf.Layer.FindOrOpen('root.testpcpstreaminglayerreload')
        self.assertTrue(l)

        # Build a cache.
        cache = self._CreatePcpCache(l)

        # Attempt to compute an index for /torus1/mesh_0 (should not exist).
        primIndex, errors = cache.ComputePrimIndex('/torus1/mesh_0')
        self.assertEqual(primIndex.primStack, [])

        # Load up asset.sdf, and replace l's content with it.  This only changes
        # the sublayer list, which pcp should recognize and blow layer stacks.
        # Since l's underlying data implementation returns true for
        # "StreamsData()" this exercises a different code-path in Pcp's change
        # processing.
        assetLayer = Sdf.Layer.FindOrOpen('asset.sdf')
        self.assertTrue(assetLayer)
        with Pcp._TestChangeProcessor(cache):
            l.TransferContent(assetLayer)

        # Now when we compute the index for the mesh, it should have a spec, due
        # to the added sublayer.
        primIndex, errors = cache.ComputePrimIndex('/torus1/mesh_0')
        self.assertTrue(len(primIndex.primStack) > 0)

if __name__ == "__main__":
    unittest.main()
