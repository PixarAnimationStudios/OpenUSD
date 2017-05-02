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

# Regression test case for bug 70951. Adding inert prim specs was not
# properly updating the 'hasSpecs' field on the associated node in the
# prim index.

from pxr import Sdf, Pcp
import unittest

class TestPcpRegressionBugs_bug70951(unittest.TestCase):
    def test_Basic(self):
        rootLayer = Sdf.Layer.FindOrOpen('bug70951/root.sdf')
        refLayer = Sdf.Layer.FindOrOpen('bug70951/JoyGroup.sdf')

        pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        # Compute the prim index for the Sim scope under JoyGroup. This index
        # should consist of two nodes: the direct node and the reference node.
        # Since there is no spec for the Sim scope in the root layer, the root node
        # should be marked as such.
        (primIndex, errors) = \
            pcpCache.ComputePrimIndex('/World/anim/chars/JoyGroup/Sim')
        self.assertEqual(len(errors), 0)
        self.assertEqual(primIndex.primStack, [refLayer.GetPrimAtPath('/JoyGroup/Sim')])
        self.assertFalse(primIndex.rootNode.hasSpecs)

        # Now create an empty over for the Sim scope in the root layer and verify
        # that the prim index sees this new spec after change processing.
        with Pcp._TestChangeProcessor(pcpCache):
            emptyOver = \
                Sdf.CreatePrimInLayer(rootLayer, '/World/anim/chars/JoyGroup/Sim')
            self.assertTrue(emptyOver.IsInert())

        (primIndex, errors) = \
            pcpCache.ComputePrimIndex('/World/anim/chars/JoyGroup/Sim')
        self.assertEqual(len(errors), 0)
        self.assertEqual(primIndex.primStack, 
                    [rootLayer.GetPrimAtPath('/World/anim/chars/JoyGroup/Sim'),
                     refLayer.GetPrimAtPath('/JoyGroup/Sim')])
        self.assertTrue(primIndex.rootNode.hasSpecs)

if __name__ == "__main__":
    unittest.main()
