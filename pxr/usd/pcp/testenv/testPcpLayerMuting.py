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

    def _LoadPcpCache(self, layerPath, sessionLayerPath=None,
                      fileFormatTarget=None):
        layer = self._LoadLayer(layerPath)
        sessionLayer = None if not sessionLayerPath else self._LoadLayer(sessionLayerPath)
        return Pcp.Cache(Pcp.LayerStackIdentifier(layer, sessionLayer),
                         fileFormatTarget=fileFormatTarget or '')

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
        self.assertEqual(pcp.GetMutedLayers(), [])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, [])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp2.GetMutedLayers(), [])
        self.assertEqual(pi2.rootNode.layerStack.mutedLayers, [])

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
        self.assertEqual(pcp.GetMutedLayers(), [sublayer.identifier])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, 
                         [sublayer.identifier])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp2.GetMutedLayers(), [])
        self.assertEqual(pi2.rootNode.layerStack.mutedLayers, [])

        # Unmute sublayer and verify that it comes back into /Root's
        # prim stack.
        pcp.RequestLayerMuting([], [sublayer.identifier])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp.GetMutedLayers(), [])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, [])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp2.GetMutedLayers(), [])
        self.assertEqual(pi2.rootNode.layerStack.mutedLayers, [])

        # Mute sublayer and verify that change processing has occurred
        # and that it no longer appears in /Root's prim stack.
        pcp.RequestLayerMuting([anonymousSublayer.identifier], [])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root')])
        self.assertTrue(anonymousSublayer)
        self.assertEqual(pcp.GetMutedLayers(), [anonymousSublayer.identifier])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, 
                         [anonymousSublayer.identifier])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp2.GetMutedLayers(), [])
        self.assertEqual(pi2.rootNode.layerStack.mutedLayers, [])

        # Unmute sublayer and verify that it comes back into /Root's
        # prim stack.
        pcp.RequestLayerMuting([], [anonymousSublayer.identifier])
        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp.GetMutedLayers(), [])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, [])

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.primStack, 
                    [layer.GetPrimAtPath('/Root'),
                     sublayer.GetPrimAtPath('/Root'),
                     anonymousSublayer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp2.GetMutedLayers(), [])
        self.assertEqual(pi2.rootNode.layerStack.mutedLayers, [])
        
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
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, [])

        pcp.RequestLayerMuting([sessionLayer.identifier], [])

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack,
                    [layer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp.GetMutedLayers(), [sessionLayer.identifier])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, 
                         [sessionLayer.identifier])

        pcp.RequestLayerMuting([], [sessionLayer.identifier])

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.primStack,
                    [sessionLayer.GetPrimAtPath('/Root'),
                     layer.GetPrimAtPath('/Root')])
        self.assertEqual(pcp.GetMutedLayers(), [])
        self.assertEqual(pi.rootNode.layerStack.mutedLayers, [])

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
        self.assertEqual(pcp.GetMutedLayers(), [refLayer.identifier])

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
        self.assertEqual(pcp.GetMutedLayers(), [])

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(not err)
        self.assertEqual(pi.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi.rootNode.children[0].layerStack.layers[0], refLayer)

        (pi2, err2) = pcp2.ComputePrimIndex('/Root')
        self.assertTrue(not err2)
        self.assertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
        self.assertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

    def test_MutingWithFileFormatTarget(self):
        """Tests layer muting for a Pcp.Cache with a file format target."""

        def _HasInvalidSublayerError(errors):
            return (Pcp.ErrorType_InvalidSublayerPath
                    in (e.errorType for e in errors))

        def _TestMuting(mutedId, 
                        expectMutedLayerId=None,
                        expectInvalidSublayerError=False):

            expectMutedLayerId = expectMutedLayerId or mutedId

            # Create a Pcp.Cache with a 'bogus' file format target. This
            # target will be used for loading layers during composition.
            pcp = self._LoadPcpCache(
                'sublayers/root.sdf', fileFormatTarget='bogus')

            # Normally, we'd be unable to load the sublayer specified in
            # root.sdf because there is no file format plugin that handles
            # the 'bogus' file format target. This would cause an invalid
            # sublayer error during composition.
            #
            # To avoid this, we're going to mute the sublayer before we
            # compose anything.
            pcp.RequestLayerMuting([mutedId], [])
            # NOTE: We use os.path.normcase() to ensure the paths can be 
            # accurately compared.  On Windows this will change forward slash 
            # directory separators to backslashes.
            mutedLayers = [os.path.normcase(mutedLayer) for mutedLayer in \
                    pcp.GetMutedLayers()]
            self.assertEqual(mutedLayers, [os.path.normcase(expectMutedLayerId)])

            # Compute prim index. We should not get an invalid sublayer error
            # because the sublayer was muted above.
            (pi, err) = pcp.ComputePrimIndex('/Root')

            if not expectInvalidSublayerError:
                self.assertFalse(_HasInvalidSublayerError(err))

        sublayerPath = os.path.join(os.getcwd(), 'sublayers/sublayer.sdf')

        # Test using an identifier with no target specified.
        _TestMuting(sublayerPath, expectMutedLayerId=sublayerPath)

        # Test using an identifier with a target that matches the target
        # used by the Pcp.Cache.
        _TestMuting(
            Sdf.Layer.CreateIdentifier(sublayerPath, {'target':'bogus'}),
            expectMutedLayerId=sublayerPath)

        # Test using an identifier with a target that does not match the
        # target used by the Pcp.Cache. In this case, the sublayer will
        # not be muted, so we expect an invalid sublayer composition error.
        _TestMuting(
            Sdf.Layer.CreateIdentifier(sublayerPath, {'target':'bogus2'}),
            expectInvalidSublayerError=True)

if __name__ == "__main__":
    unittest.main()
