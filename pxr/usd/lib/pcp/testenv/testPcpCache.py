#!/pxrpythonsubst
#
# Copyright 2017 Pixar
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

from pxr import Sdf, Pcp, Tf
import unittest, os

class TestPcpCache(unittest.TestCase):
    def test_Basic(self):
        # Create a PcpCache for a reference chain, but do not perform any actual
        # composition before querying its used layers. Verify that this does not crash.
        file = 'BasicReference/root.sdf'
        self.assertTrue(os.path.isfile(file))
        layer = Sdf.Layer.FindOrOpen(file)
        self.assertTrue(layer)
        lsi = Pcp.LayerStackIdentifier(layer)
        self.assertTrue(lsi)
        pcpCache = Pcp.Cache(lsi)
        self.assertTrue(pcpCache)
        pcpCache.GetUsedLayers()

        # Create a PcpCache with a target schema, ensuring that layers
        # without the correct target will be marked invalid during composition.
        pcpCache = Pcp.Cache(lsi, targetSchema='sdf')
        (pi, _) = pcpCache.ComputePrimIndex('/PrimWithReferences')
        self.assertTrue(pi.IsValid())
        self.assertEqual(len(pi.localErrors), 0)

        # XXX: Sdf currently emits coding errors when it cannot find a file format
        #      for the given target. I'm not sure this should be the case; it
        #      probably should treat it as though the file didn't exist, which
        #      does not emit errors.
        pcpCache = Pcp.Cache(lsi, targetSchema='Presto')

        with self.assertRaises(Tf.ErrorException):
            pcpCache.ComputePrimIndex('/PrimWithReferences')

        # Should be two local errors corresponding to invalid asset paths,
        # since this prim has two references to layers with a different target
        # schema.
        (pi, _) = pcpCache.ComputePrimIndex('/PrimWithReferences')
        self.assertTrue(pi.IsValid())
        self.assertEqual(len(pi.localErrors), 2)
        self.assertTrue(all([(isinstance(e, Pcp.ErrorInvalidAssetPath) 
                         for e in pi.localErrors)]))


    def test_PcpCacheReloadSessionLayers(self):
        rootLayer = Sdf.Layer.CreateAnonymous()

        sessionRootLayer = Sdf.Layer.CreateAnonymous()
        self.assertTrue(sessionRootLayer)
        sessionSubLayer = Sdf.Layer.CreateAnonymous()
        self.assertTrue(sessionSubLayer)

        sessionRootLayer.subLayerPaths.append(sessionSubLayer.identifier)

        # Author something to the sublayer
        primSpec = Sdf.PrimSpec(sessionSubLayer, "Root", Sdf.SpecifierDef)
        self.assertTrue(sessionSubLayer.GetPrimAtPath("/Root"))

        # Create the Pcp structures
        lsi = Pcp.LayerStackIdentifier(rootLayer, sessionRootLayer)
        self.assertTrue(lsi)
        pcpCache = Pcp.Cache(lsi)
        self.assertTrue(pcpCache)
        pcpCache.ComputeLayerStack(lsi)

        # Now reload and make sure that the spec on the sublayer stays intact
        pcpCache.Reload()
        self.assertTrue(sessionSubLayer.GetPrimAtPath("/Root"))

if __name__ == "__main__":
    unittest.main()
