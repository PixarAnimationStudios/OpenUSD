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
