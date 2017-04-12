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

from pxr import Sdf, Pcp, Tf
import os, unittest

class TestPcpLayerMuting(unittest.TestCase):
    def _LoadLayer(self, layerPath):
        return Sdf.Layer.FindOrOpen(layerPath)

    def _LoadPcpCache(self, layerPath, sessionLayerPath=None):
        layer = self._LoadLayer(layerPath)
        sessionLayer = None if not sessionLayerPath else self._LoadLayer(sessionLayerPath)
        return Pcp.Cache(Pcp.LayerStackIdentifier(layer, sessionLayer))

    def test_MutingSublayers(self):
        """Tests muting sublayers"""
        layer = self._LoadLayer(os.path.join(os.getcwd(), 'sublayers/root.sdf'))
        sublayer = self._LoadLayer(os.path.join(os.getcwd(), 'sublayers/sublayer.sdf'))
        anonymousSublayer = Sdf.Layer.CreateAnonymous('.sdf')

        # Add a prim in an anonymous sublayer to the root layer for 
        # testing purposes.
        Sdf.CreatePrimInLayer(anonymousSublayer, '/Root')
        layer.subLayerPaths.append(anonymousSublayer.identifier)

        # Create two Pcp.Caches for the same layer stack. This is to verify
        # that muting/unmuting a layer in one cache does not affect any
        # other that shares the same layers.
        pcp = self._LoadPcpCache(layer.identifier)
        pcp2 = self._LoadPcpCache(layer.identifier)

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        # Muting the cache's root layer is explicitly disallowed.
        with self.assertRaises(Tf.ErrorException):
            pcp.RequestLayerMuting([layer.identifier], [])

        # Mute sublayer and verify that change processing has occurred
        # and that it no longer appears in /Root's prim stack.
        pcp.RequestLayerMuting([sublayer.identifier], [])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        # Unmute sublayer and verify that it comes back into /Root's
        # prim stack.
        pcp.RequestLayerMuting([], [sublayer.identifier])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        # Mute sublayer and verify that change processing has occurred
        # and that it no longer appears in /Root's prim stack.
        pcp.RequestLayerMuting([anonymousSublayer.identifier], [])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root')])
        self.assertTrue(anonymousSublayer)

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        # Unmute sublayer and verify that it comes back into /Root's
        # prim stack.
        pcp.RequestLayerMuting([], [anonymousSublayer.identifier])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        
    def test_MutingSessionLayer(self):
        """Tests ability to mute a cache's session layer."""
        
        layer = self._LoadLayer(os.path.join(os.getcwd(), 'session/root.sdf'))
        sessionLayer = self._LoadLayer(os.path.join(os.getcwd(), 'session/session.sdf'))

        pcp = self._LoadPcpCache(layer.identifier, sessionLayer.identifier)

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack,
                    [sessionLayer.GetPrimAtPath('/Root'),
                     layer.GetPrimAtPath('/Root')])

        pcp.RequestLayerMuting([sessionLayer.identifier], [])

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack,
                    [layer.GetPrimAtPath('/Root')])

        pcp.RequestLayerMuting([], [sessionLayer.identifier])

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack,
                    [sessionLayer.GetPrimAtPath('/Root'),
                     layer.GetPrimAtPath('/Root')])

    def test_MutingReferencedLayers(self):
        """Tests behavior when muting and unmuting the root layer of 
        a referenced layer stack."""

        rootLayer = self._LoadLayer(os.path.join(os.getcwd(), 'refs/root.sdf'))
        refLayer = self._LoadLayer(os.path.join(os.getcwd(), 'refs/ref.sdf'))

        pcp = self._LoadPcpCache(rootLayer.identifier)
        pcp2 = self._LoadPcpCache(rootLayer.identifier)

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi.rootNode.children[0].layerStack.layers[0], refLayer)

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

        # Mute the root layer of the referenced layer stack. This should
        # result in change processing, a composition error when recomposing
        # /Root, and no reference node on the prim index.
        pcp.RequestLayerMuting([refLayer.identifier], [])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(err)
        self.assertEqual(len(pi.rootNode.children), 0)

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

        # Unmute the layer and verify that the composition error is resolved
        # and the reference is restored to the prim index.
        pcp.RequestLayerMuting([], [refLayer.identifier])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi.rootNode.children[0].layerStack.layers[0], refLayer)

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

if __name__ == "__main__":
    unittest.main()
