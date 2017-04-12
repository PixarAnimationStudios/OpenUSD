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

# Regression test for bug 90706. Adding a new prim spec
# within a variant that's being used by a prim index should
# cause that prim index to be updated.

from pxr import Sdf, Pcp
import unittest

class testPcpRegressionBugs_bug90706(unittest.TestCase):
    def test_Basic(self):
        rootLayer = Sdf.Layer.FindOrOpen('bug90706/root.sdf')
        pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))
        
        (index, err) = pcpCache.ComputePrimIndex('/Sarah/EmptyPrim')
        self.assertTrue(index)
        self.assertEqual(len(err), 0)
        
        # This prim index should only contain the root node; the variant
        # arc to /Sarah{displayColor=red}EmptyPrim will have been culled
        # because there's no prim spec there.
        self.assertEqual(len(index.rootNode.children), 0)
        
        with Pcp._TestChangeProcessor(pcpCache):
            p = Sdf.CreatePrimInLayer(
                rootLayer, '/Sarah{displayColor=red}EmptyPrim')
        
        # The addition of the inert prim should have caused the prim index
        # to be blown, because it needs to be re-computed to accommodate the
        # no-longer-inert variant node.
        self.assertFalse(pcpCache.FindPrimIndex('/Sarah/EmptyPrim'))
        (index, err) = pcpCache.ComputePrimIndex('/Sarah/EmptyPrim')
        self.assertTrue(index)
        self.assertEqual(len(err), 0)
        
        # There should now be a variant arc for /Sarah{displayColor=red}EmptyPrim
        # in the local layer stack.
        self.assertEqual(len(index.rootNode.children), 1)
        self.assertEqual(index.rootNode.children[0].arcType, 
                    Pcp.ArcTypeVariant)
        self.assertEqual(index.rootNode.children[0].layerStack,
                    index.rootNode.layerStack)
        self.assertEqual(index.rootNode.children[0].path, 
                    '/Sarah{displayColor=red}EmptyPrim')

if __name__ == "__main__":
    unittest.main()
