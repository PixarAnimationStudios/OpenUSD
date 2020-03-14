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
