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

# Regression test for bug 82180. The ultimate cause of that bug was
# PcpLayerStack erroneously blowing its relocations when an insignificant
# change to the layer stack was being processed.

from pxr import Sdf, Pcp
import unittest

class TestPcpRegressionBugs_bug82180(unittest.TestCase):
    def test_Basic(self):
        rootLayer = Sdf.Layer.FindOrOpen('bug82180/root.sdf')
        sessionLayer = Sdf.Layer.CreateAnonymous()
        
        pcpCache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer, sessionLayer))
        
        # Compute the cache's root layer stack and verify its contents.
        (layerStack, errors) = \
            pcpCache.ComputeLayerStack(pcpCache.GetLayerStackIdentifier())
        
        self.assertTrue(layerStack)
        self.assertEqual(layerStack.layers, [sessionLayer, rootLayer])
        self.assertEqual(layerStack.relocatesSourceToTarget, 
                    { Sdf.Path('/ModelGroup/Model') : Sdf.Path('/ModelGroup/Model_1') })
        self.assertEqual(layerStack.relocatesTargetToSource, 
                    { Sdf.Path('/ModelGroup/Model_1') : Sdf.Path('/ModelGroup/Model') })
        
        self.assertEqual(len(errors), 0)
        
        # Now add an empty sublayer to the session layer, which should be regarded
        # as an insignificant layer stack change.
        #
        # Note that this bug isn't specific to the session layer; it occurred with 
        # any insignificant layer stack change. The test just uses the session layer
        # since that's the repro steps from the bug report.
        emptyLayer = Sdf.Layer.FindOrOpen('bug82180/empty.sdf')
        
        with Pcp._TestChangeProcessor(pcpCache):
            sessionLayer.subLayerPaths.insert(0, emptyLayer.identifier)
        
        # Verify that the layer stack's contents are still as expect; in particular,
        # the relocations should be unaffected.
        (layerStack, errors) = \
            pcpCache.ComputeLayerStack(pcpCache.GetLayerStackIdentifier())
        
        self.assertTrue(layerStack)
        self.assertEqual(layerStack.layers, [sessionLayer, emptyLayer, rootLayer])
        self.assertEqual(layerStack.relocatesSourceToTarget, 
                    { Sdf.Path('/ModelGroup/Model') : Sdf.Path('/ModelGroup/Model_1') })
        self.assertEqual(layerStack.relocatesTargetToSource, 
                    { Sdf.Path('/ModelGroup/Model_1') : Sdf.Path('/ModelGroup/Model') })
        
        self.assertEqual(len(errors), 0)

if __name__ == "__main__":
    unittest.main()
