#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# Regression test for bug 90508. This bug was due to PcpCache incorrectly
# evicting prim indexes for all descendent prims when processing a change
# that affects only the parent prim. 

from pxr import Pcp, Sdf
import unittest

class testPcpRegressionBugs_bug90508(unittest.TestCase):
    def test_Basic(self):
        rootLayer = Sdf.Layer.FindOrOpen('bug90508/root.sdf')
        pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        for p in [Sdf.Path('/Model'),
                  Sdf.Path('/Model/Child'),
                  Sdf.Path('/Model/Child/Child2')]:
            self.assertTrue(pcpCache.ComputePrimIndex(p))
            self.assertTrue(pcpCache.FindPrimIndex(p))

        # The prim index at /Model/Child should have an an implied inherit arc to
        # the site @root.sdf@</_class_Model/Child>; however, this node is culled 
        # because there are no prim specs at that location. Adding a spec there
        # should cause the PcpCache to evict the prim index, since it needs to be
        # regenerated to accommodate the new node. However, this change doesn't
        # affect any other prim index, so those should remain in the cache.
        with Pcp._TestChangeProcessor(pcpCache):
            Sdf.CreatePrimInLayer(rootLayer, '/_class_Model/Child')

        self.assertTrue(pcpCache.FindPrimIndex('/Model'))
        self.assertFalse(pcpCache.FindPrimIndex('/Model/Child'))
        self.assertTrue(pcpCache.FindPrimIndex('/Model/Child/Child2'))

if __name__ == "__main__":
    unittest.main()
