#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
