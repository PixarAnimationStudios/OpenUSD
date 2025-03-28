#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

# Regression test for bug 92955.  Removing a class should not cause
# unrelated spooky sites to be flushed.

from pxr import Pcp, Sdf
import unittest

class TestPcpRegressionBugs_bug92955(unittest.TestCase):
    def test_Basic(self):
        rootLayer = Sdf.Layer.FindOrOpen('bug92955/root.sdf')
        pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        # Populate all prims in the scene
        paths = [Sdf.Path('/')]
        while paths:
            path, paths = paths[0], paths[1:]
            primIndex, _ = pcpCache.ComputePrimIndex(path)
            for name in primIndex.ComputePrimChildNames()[0]:
                paths.append(path.AppendChild(name))

        # Verify there are symmetric corresponding paths on SymRig.
        layerStack = pcpCache.layerStack
        x = pcpCache.FindSiteDependencies(layerStack, '/Model/Rig/SymRig',
                                        Pcp.DependencyTypeDirect |
                                        Pcp.DependencyTypeNonVirtual)
        self.assertNotEqual(len(x), 0)

        # Delete the unrelated class.
        with Pcp._TestChangeProcessor(pcpCache):
            del rootLayer.GetPrimAtPath('/Model/Rig').nameChildren['SymScope']

        # Verify the same symmetric corresponding paths are on SymRig.
        y = pcpCache.FindSiteDependencies(layerStack, '/Model/Rig/SymRig',
                                        Pcp.DependencyTypeDirect |
                                        Pcp.DependencyTypeNonVirtual)
        self.assertEqual(x, y)

if __name__ == "__main__":
    unittest.main()
