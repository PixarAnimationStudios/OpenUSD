#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# Regression test for bug 101300. This bug was due to PcpLayerStack not
# recomputing its relocation tables when a prim that supplies 
# relocations was removed.

from pxr import Pcp, Sdf
import unittest

class TestPcpRegressionBugs_bug101300(unittest.TestCase):
    def test_Basic(self):
        rootLayer = Sdf.Layer.FindOrOpen('bug101300/root.sdf')
        pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        # Compute the prim index for /Root/A. The relocates authored on /Root/A
        # should be applied, causing /Root/A/B to be renamed /Root/A/B_2.
        (pi, err) = pcpCache.ComputePrimIndex('/Root/A')
        self.assertEqual(len(err), 0)
        self.assertEqual(pi.ComputePrimChildNames(), (['B_2'],['B']))
        self.assertEqual(pcpCache.layerStack.relocatesSourceToTarget,
                    {Sdf.Path('/Root/A/B'): Sdf.Path('/Root/A/B_2')})
        self.assertEqual(pcpCache.layerStack.pathsToPrimsWithRelocates,
                    [Sdf.Path('/Root/A')])

        # Remove the over on /Root/A that provides the relocates. 
        with Pcp._TestChangeProcessor(pcpCache):
            del rootLayer.GetPrimAtPath('/Root').nameChildren['A']

        # The prim at /Root/A should still exist, but there should no longer be
        # any relocates to apply.
        (pi, err) = pcpCache.ComputePrimIndex('/Root/A')
        self.assertEqual(len(err), 0)
        self.assertEqual(pi.ComputePrimChildNames(), (['B'],[]))
        self.assertEqual(pcpCache.layerStack.relocatesSourceToTarget, {})
        self.assertEqual(pcpCache.layerStack.pathsToPrimsWithRelocates, [])

if __name__ == "__main__":
    unittest.main()
