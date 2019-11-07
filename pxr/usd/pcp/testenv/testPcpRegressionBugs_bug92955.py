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
