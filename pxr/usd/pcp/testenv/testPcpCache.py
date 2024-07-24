#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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

        ls = pcpCache.layerStack
        self.assertTrue(pcpCache.HasRootLayerStack(ls))

        # Create a PcpCache with a file format target, ensuring that layers
        # without the correct target will be marked invalid during composition.
        pcpCache = Pcp.Cache(lsi, fileFormatTarget='sdf')
        (pi, _) = pcpCache.ComputePrimIndex('/PrimWithReferences')
        self.assertTrue(pi.IsValid())
        self.assertEqual(len(pi.localErrors), 0)

        # Should be two local errors corresponding to invalid asset paths,
        # since this prim has two references to layers with a different target.
        pcpCache = Pcp.Cache(lsi, fileFormatTarget='Presto')
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
