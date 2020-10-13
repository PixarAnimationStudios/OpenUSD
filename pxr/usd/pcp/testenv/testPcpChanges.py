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
import unittest
from contextlib import contextmanager

class TestPcpChanges(unittest.TestCase):
    def test_EmptySublayerChanges(self):
        subLayer1 = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.CreatePrimInLayer(subLayer1, '/Test')
        
        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.subLayerPaths.append(subLayer1.identifier)

        layerStackId = Pcp.LayerStackIdentifier(rootLayer)
        pcp = Pcp.Cache(layerStackId)

        (pi, err) = pcp.ComputePrimIndex('/Test')
        self.assertFalse(err)
        self.assertEqual(pi.primStack, [primSpec])

        subLayer2 = Sdf.Layer.CreateAnonymous()
        self.assertTrue(subLayer2.empty)

        # Adding an empty sublayer should not require recomputing any
        # prim indexes or change their prim stacks.
        with Pcp._TestChangeProcessor(pcp):
            rootLayer.subLayerPaths.insert(0, subLayer2.identifier)

        pi = pcp.FindPrimIndex('/Test')
        self.assertTrue(pi)
        self.assertEqual(pi.primStack, [primSpec])

        # Same with deleting an empty sublayer.
        with Pcp._TestChangeProcessor(pcp):
            del rootLayer.subLayerPaths[0]

        pi = pcp.FindPrimIndex('/Test')
        self.assertTrue(pi)
        self.assertEqual(pi.primStack, [primSpec])

    def test_InvalidSublayerAdd(self):
        invalidSublayerId = "/tmp/testPcpChanges_invalidSublayer.sdf"

        layer = Sdf.Layer.CreateAnonymous()
        layerStackId = Pcp.LayerStackIdentifier(layer)
        pcp = Pcp.Cache(layerStackId)

        (layerStack, errs) = pcp.ComputeLayerStack(layerStackId)
        self.assertEqual(len(errs), 0)
        self.assertEqual(len(layerStack.localErrors), 0)

        with Pcp._TestChangeProcessor(pcp):
            layer.subLayerPaths.append(invalidSublayerId)

        (layerStack, errs) = pcp.ComputeLayerStack(layerStackId)
        # This is potentially surprising. Layer stacks are recomputed
        # immediately during change processing, so any composition
        # errors generated during that process won't be reported 
        # during the call to ComputeLayerStack. The errors will be
        # stored in the layer stack's localErrors field, however.
        self.assertEqual(len(errs), 0)
        self.assertEqual(len(layerStack.localErrors), 1)

    def test_InvalidSublayerRemoval(self):
        invalidSublayerId = "/tmp/testPcpChanges_invalidSublayer.sdf"

        layer = Sdf.Layer.CreateAnonymous()
        layer.subLayerPaths.append(invalidSublayerId)

        layerStackId = Pcp.LayerStackIdentifier(layer)
        pcp = Pcp.Cache(layerStackId)

        (layerStack, errs) = pcp.ComputeLayerStack(layerStackId)
        self.assertEqual(len(errs), 1)
        self.assertEqual(len(layerStack.localErrors), 1)
        self.assertTrue(pcp.IsInvalidSublayerIdentifier(invalidSublayerId))

        with Pcp._TestChangeProcessor(pcp):
            layer.subLayerPaths.remove(invalidSublayerId)

        (layerStack, errs) = pcp.ComputeLayerStack(layerStackId)
        self.assertEqual(len(errs), 0)
        self.assertEqual(len(layerStack.localErrors), 0)
        self.assertFalse(pcp.IsInvalidSublayerIdentifier(invalidSublayerId))

    def test_UnusedVariantChanges(self):
        layer = Sdf.Layer.CreateAnonymous()
        parent = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef, 'Scope')
        vA = Sdf.CreateVariantInLayer(layer, parent.path, 'var', 'A')
        vB = Sdf.CreateVariantInLayer(layer, parent.path, 'var', 'B')
        parent.variantSelections['var'] = 'A'

        layerStackId = Pcp.LayerStackIdentifier(layer)
        pcp = Pcp.Cache(layerStackId)

        (pi, err) = pcp.ComputePrimIndex('/Root')
        self.assertTrue(pi)
        self.assertEqual(len(err), 0)

        # Add a new prim spec inside the unused variant and verify that this
        # does not cause the cached prim index to be blown.
        with Pcp._TestChangeProcessor(pcp):
            newPrim = Sdf.PrimSpec(vB.primSpec, 'Child', Sdf.SpecifierDef, 'Scope')

        self.assertTrue(pcp.FindPrimIndex('/Root'))

    def test_SublayerOffsetChanges(self):
        rootLayerPath = 'TestSublayerOffsetChanges/root.sdf'
        rootSublayerPath = 'TestSublayerOffsetChanges/root-sublayer.sdf'
        refLayerPath = 'TestSublayerOffsetChanges/ref.sdf'
        refSublayerPath = 'TestSublayerOffsetChanges/ref-sublayer.sdf'
        ref2LayerPath = 'TestSublayerOffsetChanges/ref2.sdf'
        
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        (pi, err) = pcp.ComputePrimIndex('/A')
        self.assertTrue(pi)
        self.assertEqual(len(err), 0)

        # Verify the expected structure of the test asset. It should simply be
        # a chain of two references, with layer offsets of 100.0 and 50.0
        # respectively.
        refNode = pi.rootNode.children[0]
        self.assertEqual(refNode.layerStack.layers, 
                    [Sdf.Layer.Find(refLayerPath), Sdf.Layer.Find(refSublayerPath)])
        self.assertEqual(refNode.arcType, Pcp.ArcTypeReference)
        self.assertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(100.0))

        ref2Node = refNode.children[0]
        self.assertEqual(ref2Node.layerStack.layers, [Sdf.Layer.Find(ref2LayerPath)])
        self.assertEqual(ref2Node.arcType, Pcp.ArcTypeReference)
        self.assertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(150.0))

        # Change the layer offset in the local layer stack and verify that
        # invalidates the prim index and that the updated layer offset is
        # taken into account after recomputing the index.
        with Pcp._TestChangeProcessor(pcp):
            rootLayer.subLayerOffsets[0] = Sdf.LayerOffset(200.0)
        
        self.assertFalse(pcp.FindPrimIndex('/A'))
        (pi, err) = pcp.ComputePrimIndex('/A')
        refNode = pi.rootNode.children[0]
        ref2Node = refNode.children[0]

        self.assertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(200.0))
        self.assertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(250.0))

        # Change the layer offset in a referenced layer stack and again verify
        # that the prim index is invalidated and the updated layer offset is
        # taken into account.
        refLayer = refNode.layerStack.layers[0]
        with Pcp._TestChangeProcessor(pcp):
            refLayer.subLayerOffsets[0] = Sdf.LayerOffset(200.0)

            self.assertFalse(pcp.FindPrimIndex('/A'))
            # Compute the prim index in the change processing block as the 
            # changed refLayer is only being held onto by the changes' lifeboat
            # as its referencing prim index has been invalidated.
            (pi, err) = pcp.ComputePrimIndex('/A')
            refNode = pi.rootNode.children[0]
            ref2Node = refNode.children[0]

            self.assertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(200.0))
            self.assertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(400.0))

    def _RunTcpsChangesForLayer(self, pcpCache, layer, tcpsToExpecteOffsetsMap, 
                                affectedPaths):
        """ 
        Helper function for test_TcpsChanges to run a suite of various TCPS and
        FPS metadata changes on a particular layer and verifying that the 
        correct change processing is run for every change.
        """

        # Helper function for a making a TCPS and/or FPS change and verify the
        # expected change processing, prim index invalidation, and new computed
        # values.
        def _ChangeAndVerify(newValDict, expectSignificantChange, expectedTcps):

            # Just verify we have a tcps value and/or fps value to set
            self.assertTrue('tcps' in newValDict or 'fps' in newValDict)
            with Pcp._TestChangeProcessor(pcpCache) as cp:
                # Change block for when we're setting both fps and tcps
                with Sdf.ChangeBlock():
                    # Set the tcps value if present (None -> Clear)
                    if 'tcps' in newValDict:
                        val = newValDict['tcps']
                        if val is None:
                            layer.ClearTimeCodesPerSecond()
                            self.assertFalse(layer.HasTimeCodesPerSecond())
                        else:
                            layer.timeCodesPerSecond = val
                            self.assertTrue(layer.HasTimeCodesPerSecond())

                    # Set the fps value if present (None -> Clear)
                    if 'fps' in newValDict:
                        val = newValDict['fps']
                        if val is None:
                            layer.ClearFramesPerSecond()
                            self.assertFalse(layer.HasFramesPerSecond())
                        else:
                            layer.framesPerSecond = val
                            self.assertTrue(layer.HasFramesPerSecond())

                # Verify whether the change processor logged a significant
                # change for the expected affected paths or not based on 
                # whether we expect a significant change.
                if expectSignificantChange:
                    self.assertEqual(cp.GetSignificantChanges(), affectedPaths)
                    # A significant change will have invalidated our 
                    # prim's prim index.
                    self.assertFalse(pcpCache.FindPrimIndex('/A'))
                    # Recompute the new prim index
                    (pi, err) = pcpCache.ComputePrimIndex('/A')
                else:
                    # No significant should leave our prim's prim index 
                    # valid.
                    self.assertEqual(cp.GetSignificantChanges(), [])
                    pi = pcpCache.FindPrimIndex('/A')
                    self.assertTrue(pi)

                refNode = pi.rootNode.children[0]
                ref2Node = refNode.children[0]

                # Verify the layer has the expected computed TCPS
                self.assertEqual(layer.timeCodesPerSecond, expectedTcps)
                # Verify the ref node offesets match the expected offsets
                # for the layer's expected computed TCPS.
                expectedOffsets = tcpsToExpecteOffsetsMap[expectedTcps]
                self.assertEqual(refNode.mapToRoot.timeOffset, 
                                 expectedOffsets[0])
                self.assertEqual(ref2Node.mapToRoot.timeOffset, 
                                 expectedOffsets[1])

        # Expect the layer to start with no authored TCPS of FPS. Verify
        # various changes to authored timeCodesPerSecond
        self.assertFalse(layer.HasTimeCodesPerSecond())
        self.assertFalse(layer.HasFramesPerSecond())
        _ChangeAndVerify({'tcps' : 24.0}, False, 24.0)
        _ChangeAndVerify({'tcps' : None}, False, 24.0)
        _ChangeAndVerify({'tcps' : 48.0}, True, 48.0)
        _ChangeAndVerify({'tcps' : 12.0}, True, 12.0)
        _ChangeAndVerify({'tcps' : None}, True, 24.0)

        # Expect the layer to start with no authored TCPS of FPS again. 
        # Verify various changes to authored framesPerSecond
        self.assertFalse(layer.HasTimeCodesPerSecond())
        self.assertFalse(layer.HasFramesPerSecond())
        _ChangeAndVerify({'fps' : 24.0}, False, 24.0)
        _ChangeAndVerify({'fps' : None}, False, 24.0)
        _ChangeAndVerify({'fps' : 48.0}, True, 48.0)
        _ChangeAndVerify({'fps' : 12.0}, True, 12.0)
        _ChangeAndVerify({'fps' : None}, True, 24.0)

        # Change the layer to have an authored non-default framesPerSecond.
        # Verify various changes to timeCodesPerSecond.
        _ChangeAndVerify({'fps' : 48.0}, True, 48.0)
        self.assertFalse(layer.HasTimeCodesPerSecond())
        self.assertTrue(layer.HasFramesPerSecond())
        self.assertEqual(layer.timeCodesPerSecond, 48.0)
        _ChangeAndVerify({'tcps' : 48.0}, False, 48.0)
        _ChangeAndVerify({'tcps' : None}, False, 48.0)
        _ChangeAndVerify({'tcps' : 12.0}, True, 12.0)
        _ChangeAndVerify({'tcps' : 24.0}, True, 24.0)
        _ChangeAndVerify({'tcps' : None}, True, 48.0)

        # Change the layer to have an authored timeCodesPerSecond.
        # Verify that various changes to framesPerSecond have no effect.
        _ChangeAndVerify({'tcps' : 24.0, 'fps' : None}, True, 24.0)
        self.assertTrue(layer.HasTimeCodesPerSecond())
        self.assertFalse(layer.HasFramesPerSecond())
        self.assertEqual(layer.timeCodesPerSecond, 24.0)
        _ChangeAndVerify({'fps' : 24.0}, False, 24.0)
        _ChangeAndVerify({'fps' : None}, False, 24.0)
        _ChangeAndVerify({'fps' : 48.0}, False, 24.0)
        _ChangeAndVerify({'fps' : 12.0}, False, 24.0)
        _ChangeAndVerify({'fps' : None}, False, 24.0)

        # Change the layer to start with an unauthored timeCodesPerSecond 
        # and a non-default framesPerSecond
        # Verify various changes to timeCodesPerSecond and framesPerSecond
        # at the same time.
        _ChangeAndVerify({'tcps' : None, 'fps' : 48.0}, True, 48.0)
        self.assertFalse(layer.HasTimeCodesPerSecond())
        self.assertTrue(layer.HasFramesPerSecond())
        self.assertEqual(layer.timeCodesPerSecond, 48.0)
        _ChangeAndVerify({'tcps' : 48.0, 'fps' : None}, False, 48.0)
        _ChangeAndVerify({'tcps' : None, 'fps' : 48.0}, False, 48.0)
        _ChangeAndVerify({'tcps' : 24.0, 'fps' : None}, True, 24.0)
        _ChangeAndVerify({'tcps' : None, 'fps' : 48.0}, True, 48.0)
        _ChangeAndVerify({'tcps' : 12.0, 'fps' : None}, True, 12.0)
        _ChangeAndVerify({'tcps' : 48.0, 'fps' : 12.0}, True, 48.0)
        _ChangeAndVerify({'tcps' : 12.0, 'fps' : 48.0}, True, 12.0)
        _ChangeAndVerify({'tcps' : None, 'fps' : 12.0}, False, 12.0)
        _ChangeAndVerify({'tcps' : 24.0, 'fps' : 24.0}, True, 24.0)
        _ChangeAndVerify({'tcps' : None, 'fps' : None}, False, 24.0)

    @unittest.skipIf(
        Tf.GetEnvSetting('PCP_DISABLE_TIME_SCALING_BY_LAYER_TCPS'),
        "Test requires layer TCPS time scaling enabled")
    def test_TcpsChanges(self):
        """
        Tests change processing for changes that affect the time codes per
        second of all layers in the layer stacks of a PcpCache.
        """

        # Use the same layers as the sublayer offset test case.
        rootLayerPath = 'TestSublayerOffsetChanges/root.sdf'
        rootSublayerPath = 'TestSublayerOffsetChanges/root-sublayer.sdf'
        refLayerPath = 'TestSublayerOffsetChanges/ref.sdf'
        refSublayerPath = 'TestSublayerOffsetChanges/ref-sublayer.sdf'
        ref2LayerPath = 'TestSublayerOffsetChanges/ref2.sdf'

        rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)
        sessionLayer = Sdf.Layer.CreateAnonymous()
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer, sessionLayer))

        (pi, err) = pcp.ComputePrimIndex('/A')
        self.assertTrue(pi)
        self.assertEqual(len(err), 0)

        rootSublayer = Sdf.Layer.Find(rootSublayerPath)
        refLayer = Sdf.Layer.Find(refLayerPath)
        refSublayer = Sdf.Layer.Find(refSublayerPath)
        ref2Layer = Sdf.Layer.Find(ref2LayerPath)

        # Verify the expected structure of the test asset. It should simply be
        # a chain of two references, with layer offsets of 100.0 and 50.0
        # respectively.
        self.assertEqual(pi.rootNode.layerStack.layers, 
                         [sessionLayer, rootLayer, rootSublayer])

        refNode = pi.rootNode.children[0]
        self.assertEqual(refNode.layerStack.layers, [refLayer, refSublayer])
        self.assertEqual(refNode.arcType, Pcp.ArcTypeReference)
        self.assertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(100.0))
        for layer in refNode.layerStack.layers:
            self.assertFalse(layer.HasTimeCodesPerSecond())
            self.assertEqual(layer.timeCodesPerSecond, 24.0)

        ref2Node = refNode.children[0]
        self.assertEqual(ref2Node.layerStack.layers, [ref2Layer])
        self.assertEqual(ref2Node.arcType, Pcp.ArcTypeReference)
        self.assertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(150.0))
        for layer in ref2Node.layerStack.layers:
            self.assertFalse(layer.HasTimeCodesPerSecond())
            self.assertEqual(layer.timeCodesPerSecond, 24.0)

        # Run the TCPS change suite on the root layer. 
        tcpsToOffsets = {12.0: (Sdf.LayerOffset(100.0, 0.5),
                                Sdf.LayerOffset(125.0, 0.5)),
                         24.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0)),
                         48.0: (Sdf.LayerOffset(100.0, 2.0),
                                Sdf.LayerOffset(200.0, 2.0))
                         }
        self._RunTcpsChangesForLayer(pcp, rootLayer, tcpsToOffsets, ['/'])

        # Run the TCPS change suite on the first reference layer. 
        tcpsToOffsets = {12.0: (Sdf.LayerOffset(100.0, 2.0),
                                Sdf.LayerOffset(200.0)),
                         24.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0)),
                         48.0: (Sdf.LayerOffset(100.0, 0.5),
                                Sdf.LayerOffset(125.0))
                         }
        self._RunTcpsChangesForLayer(pcp, refLayer, tcpsToOffsets, ['/A'])

        # Run the TCPS change suite on the second reference layer. 
        tcpsToOffsets = {12.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0, 2.0)),
                         24.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0)),
                         48.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0, 0.5))
                         }
        self._RunTcpsChangesForLayer(pcp, ref2Layer, tcpsToOffsets, ['/A'])

        # Run the TCPS change suite on the sublayers of the root and reference 
        # layers. In the particular setup of these layer, TCPS of either 
        # sublayer doesn't change the layer offsets applied to the reference 
        # nodes, but will still cause change management to report significant
        # changes to prim indexes.
        tcpsToOffsets = {12.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0)),
                         24.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0)),
                         48.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0))
                         }
        self._RunTcpsChangesForLayer(pcp, rootSublayer, tcpsToOffsets, ['/'])
        self._RunTcpsChangesForLayer(pcp, refSublayer, tcpsToOffsets, ['/A'])

        # Run the TCPS change suite on the session layer.
        tcpsToOffsets = {12.0: (Sdf.LayerOffset(50.0, 0.5),
                                Sdf.LayerOffset(75.0, 0.5)),
                         24.0: (Sdf.LayerOffset(100.0),
                                Sdf.LayerOffset(150.0)),
                         48.0: (Sdf.LayerOffset(200.0, 2.0),
                                Sdf.LayerOffset(300.0, 2.0))
                         }
        self._RunTcpsChangesForLayer(pcp, sessionLayer, tcpsToOffsets, ['/'])

        # Special cases for the session layer when root layer has a tcps value
        rootLayer.timeCodesPerSecond = 24.0
        self.assertTrue(rootLayer.HasTimeCodesPerSecond())
        self.assertFalse(sessionLayer.HasTimeCodesPerSecond())
        with Pcp._TestChangeProcessor(pcp) as cp:
            # Set the session layer's FPS. This will change the session layer's
            # computed TCPS and is a significant change even though the overall
            # TCPS of the root layer stack is 24 as it is authored on the root 
            # layer.
            sessionLayer.framesPerSecond = 48.0
            self.assertEqual(sessionLayer.timeCodesPerSecond, 48.0)
            self.assertEqual(cp.GetSignificantChanges(), ['/'])
            self.assertFalse(pcp.FindPrimIndex('/A'))
            # Recompute the new prim index
            (pi, err) = pcp.ComputePrimIndex('/A')
            refNode = pi.rootNode.children[0]
            ref2Node = refNode.children[0]
            # The reference layer offsets are still the same as authored as
            # the root layer TCPS still matches its layer stack's overall TCPS
            self.assertEqual(refNode.mapToRoot.timeOffset, 
                             Sdf.LayerOffset(100.0))
            self.assertEqual(ref2Node.mapToRoot.timeOffset, 
                             Sdf.LayerOffset(150.0))

        # Continuing from the previous case, root layer has TCPS set to 24 and
        # session layer has FPS set to 48. Now we set the session TCPS to 48.
        # While this does not cause a TCPS change to session layer taken by 
        # itself, this does mean that the session now overrides the overall
        # TCPS of the layer stack which used to come the root. We verify here
        # that this is a significant change and that it scales the layer offsets
        # to the references.
        self.assertFalse(sessionLayer.HasTimeCodesPerSecond())
        with Pcp._TestChangeProcessor(pcp) as cp:
            sessionLayer.timeCodesPerSecond = 48.0
            self.assertEqual(sessionLayer.timeCodesPerSecond, 48.0)
            self.assertEqual(cp.GetSignificantChanges(), ['/'])
            self.assertFalse(pcp.FindPrimIndex('/A'))
            # Recompute the new prim index
            (pi, err) = pcp.ComputePrimIndex('/A')
            refNode = pi.rootNode.children[0]
            ref2Node = refNode.children[0]
            self.assertEqual(refNode.mapToRoot.timeOffset,
                             Sdf.LayerOffset(200.0, 2.0))
            self.assertEqual(ref2Node.mapToRoot.timeOffset,
                             Sdf.LayerOffset(300.0, 2.0))

        # And as a parallel to the previous case, we now clear the session TCPS
        # again. This is still no effective change to the session layer TCPS
        # itself, but root layer's TCPS is once again the overall layer stack
        # TCPS. This is significant changes and the layer offsets return to
        # their original values.
        with Pcp._TestChangeProcessor(pcp) as cp:
            sessionLayer.ClearTimeCodesPerSecond()
            self.assertEqual(sessionLayer.timeCodesPerSecond, 48.0)
            self.assertEqual(cp.GetSignificantChanges(), ['/'])
            self.assertFalse(pcp.FindPrimIndex('/A'))
            # Recompute the new prim index
            (pi, err) = pcp.ComputePrimIndex('/A')
            refNode = pi.rootNode.children[0]
            ref2Node = refNode.children[0]
            # The reference layer offsets are still the same as authored as
            # the root layer TCPS still matches its layer stack's overall TCPS
            self.assertEqual(refNode.mapToRoot.timeOffset, 
                             Sdf.LayerOffset(100.0))
            self.assertEqual(ref2Node.mapToRoot.timeOffset, 
                             Sdf.LayerOffset(150.0))

        # One more special case. Neither session or root layers have TCPS set.
        # Root layer has FPS set to 48, session layer has FPS set to 24. Overall
        # computed TCPS of the layer stack will be 24, matching session FPS.
        # We then author a TCPS value of 48 to the root layer, matching its FPS
        # value. There is no effective change to the root layer's TCPS itself
        # but we do end up with a significant change as the overall layer stack
        # TCPS will now compute to 48.
        sessionLayer.ClearTimeCodesPerSecond()
        rootLayer.ClearTimeCodesPerSecond()
        sessionLayer.framesPerSecond = 24.0
        rootLayer.framesPerSecond = 48.0
        with Pcp._TestChangeProcessor(pcp) as cp:
            rootLayer.timeCodesPerSecond = 48.0
            self.assertEqual(rootLayer.timeCodesPerSecond, 48.0)
            self.assertEqual(cp.GetSignificantChanges(), ['/'])
            self.assertFalse(pcp.FindPrimIndex('/A'))
            # Recompute the new prim index
            (pi, err) = pcp.ComputePrimIndex('/A')
            refNode = pi.rootNode.children[0]
            ref2Node = refNode.children[0]
            self.assertEqual(refNode.mapToRoot.timeOffset,
                             Sdf.LayerOffset(100.0, 2.0))
            self.assertEqual(ref2Node.mapToRoot.timeOffset,
                             Sdf.LayerOffset(200.0, 2.0))


    def test_DefaultReferenceTargetChanges(self):
        # create a layer, set DefaultPrim, then reference it.
        targLyr = Sdf.Layer.CreateAnonymous()
        
        def makePrim(name):
            primSpec = Sdf.CreatePrimInLayer(targLyr, name)
            primSpec.specifier = Sdf.SpecifierDef

        makePrim('target1')
        makePrim('target2')

        targLyr.defaultPrim = 'target1'

        def _Test(referencingLayer):
            # Make a PcpCache.
            pcp = Pcp.Cache(Pcp.LayerStackIdentifier(referencingLayer))

            (pi, err) = pcp.ComputePrimIndex('/source')

            # First child node should be the referenced target1.
            self.assertEqual(pi.rootNode.children[0].path, '/target1')

            # Now clear defaultPrim.  This should issue an error and
            # fail to pick up the referenced prim.
            with Pcp._TestChangeProcessor(pcp):
                targLyr.ClearDefaultPrim()

            (pi, err) = pcp.ComputePrimIndex('/source')
            self.assertTrue(isinstance(err[0], Pcp.ErrorUnresolvedPrimPath))
            # If the reference to the defaultPrim is an external reference, 
            # the one child node should be the pseudoroot dependency placeholder.
            # If the reference is an internal reference, that dependency
            # placeholder is unneeded.
            if referencingLayer != targLyr:
                self.assertEqual(len(pi.rootNode.children), 1)
                self.assertEqual(pi.rootNode.children[0].path, '/')

            # Change defaultPrim to other target.  This should pick
            # up the reference again, but to the new prim target2.
            with Pcp._TestChangeProcessor(pcp):
                targLyr.defaultPrim = 'target2'

            (pi, err) = pcp.ComputePrimIndex('/source')
            self.assertEqual(len(err), 0)
            self.assertEqual(pi.rootNode.children[0].path, '/target2')

            # Reset defaultPrim to original target
            targLyr.defaultPrim = 'target1'

        # Test external reference case where some other layer references
        # a layer with defaultPrim specified.
        srcLyr = Sdf.Layer.CreateAnonymous()
        srcPrimSpec = Sdf.CreatePrimInLayer(srcLyr, '/source')
        srcPrimSpec.referenceList.Add(Sdf.Reference(targLyr.identifier))
        _Test(srcLyr)

        # Test internal reference case where a prim in the layer with defaultPrim
        # specified references the same layer.
        srcPrimSpec = Sdf.CreatePrimInLayer(targLyr, '/source')
        srcPrimSpec.referenceList.Add(Sdf.Reference())
        _Test(targLyr)

    def test_InternalReferenceChanges(self):
        rootLayer = Sdf.Layer.CreateAnonymous()

        Sdf.PrimSpec(rootLayer, 'target1', Sdf.SpecifierDef)
        Sdf.PrimSpec(rootLayer, 'target2', Sdf.SpecifierDef)
        
        srcPrimSpec = Sdf.PrimSpec(rootLayer, 'source', Sdf.SpecifierDef)
        srcPrimSpec.referenceList.Add(Sdf.Reference(primPath = '/target1'))
        
        # Initially, the prim index for /source should contain a single
        # reference node to /target1 in rootLayer.
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))
        (pi, err) = pcp.ComputePrimIndex('/source')
        
        self.assertEqual(len(err), 0)
        self.assertEqual(pi.rootNode.children[0].layerStack.identifier.rootLayer,
                    rootLayer)
        self.assertEqual(pi.rootNode.children[0].path, '/target1')

        # Modify the internal reference to point to /target2 and verify the
        # reference node is updated.
        with Pcp._TestChangeProcessor(pcp):
            srcPrimSpec.referenceList.addedItems[0] = \
                Sdf.Reference(primPath = '/target2')

        (pi, err) = pcp.ComputePrimIndex('/source')
        
        self.assertEqual(len(err), 0)
        self.assertEqual(pi.rootNode.children[0].layerStack.identifier.rootLayer,
                    rootLayer)
        self.assertEqual(pi.rootNode.children[0].path, '/target2')

        # Clear out all references and verify that the prim index contains no
        # reference nodes.
        with Pcp._TestChangeProcessor(pcp):
            srcPrimSpec.referenceList.ClearEdits()

        (pi, err) = pcp.ComputePrimIndex('/source')
        
        self.assertEqual(len(err), 0)
        self.assertEqual(len(pi.rootNode.children), 0)

    def test_VariantChanges(self):
        rootLayer = Sdf.Layer.CreateAnonymous()
        modelSpec = Sdf.PrimSpec(rootLayer, 'Variant', Sdf.SpecifierDef)

        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd=True)

        # Test changes that are emitted as a variant set and variant
        # are created.

        pcp.ComputePrimIndex('/Variant')    
        with Pcp._TestChangeProcessor(pcp) as cp:
            varSetSpec = Sdf.VariantSetSpec(modelSpec, 'test')
            self.assertEqual(cp.GetSignificantChanges(), ['/Variant'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

        pcp.ComputePrimIndex('/Variant')    
        with Pcp._TestChangeProcessor(pcp) as cp:
            modelSpec.variantSelections['test'] = 'A'
            self.assertEqual(cp.GetSignificantChanges(), ['/Variant'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

        pcp.ComputePrimIndex('/Variant')    
        with Pcp._TestChangeProcessor(pcp) as cp:
            modelSpec.variantSetNameList.Add('test')
            self.assertEqual(cp.GetSignificantChanges(), ['/Variant'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

        pcp.ComputePrimIndex('/Variant')    
        with Pcp._TestChangeProcessor(pcp) as cp:
            varSpec = Sdf.VariantSpec(varSetSpec, 'A')
            self.assertEqual(cp.GetSignificantChanges(), [])
            self.assertEqual(cp.GetSpecChanges(), ['/Variant'])
            # Creating the variant spec adds an inert spec to /Variant's prim
            # stack but does not require rebuilding /Variant's prim index to
            # account for the new variant node.
            self.assertEqual(cp.GetPrimChanges(), [])

        pcp.ComputePrimIndex('/Variant')
        with Pcp._TestChangeProcessor(pcp) as cp:
            varSpec.primSpec.referenceList.Add(
                Sdf.Reference('./dummy.sdf', '/Dummy'))
            self.assertEqual(cp.GetSignificantChanges(), ['/Variant'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

    def test_InstancingChanges(self):
        refLayer = Sdf.Layer.CreateAnonymous()
        refParentSpec = Sdf.PrimSpec(refLayer, 'Parent', Sdf.SpecifierDef)
        refChildSpec = Sdf.PrimSpec(refParentSpec, 'RefChild', Sdf.SpecifierDef)

        rootLayer = Sdf.Layer.CreateAnonymous()
        parentSpec = Sdf.PrimSpec(rootLayer, 'Parent', Sdf.SpecifierOver)
        parentSpec.referenceList.Add(
            Sdf.Reference(refLayer.identifier, '/Parent'))
        childSpec = Sdf.PrimSpec(parentSpec, 'DirectChild', Sdf.SpecifierDef)

        # Instancing is only enabled in Usd mode.
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd=True)

        # /Parent is initially not tagged as an instance, so we should
        # see both RefChild and DirectChild name children.
        (pi, err) = pcp.ComputePrimIndex('/Parent')
        self.assertEqual(len(err), 0)
        self.assertFalse(pi.IsInstanceable())
        self.assertEqual(pi.ComputePrimChildNames(), (['RefChild', 'DirectChild'], []))

        with Pcp._TestChangeProcessor(pcp) as cp:
            parentSpec.instanceable = True
            self.assertEqual(cp.GetSignificantChanges(), ['/Parent'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

        # After being made an instance, DirectChild should no longer
        # be reported as a name child since instances may not introduce
        # new prims locally.
        (pi, err) = pcp.ComputePrimIndex('/Parent')
        self.assertEqual(len(err), 0)
        self.assertTrue(pi.IsInstanceable())
        self.assertEqual(pi.ComputePrimChildNames(), (['RefChild'], []))

        with Pcp._TestChangeProcessor(pcp) as cp:
            parentSpec.instanceable = False
            self.assertEqual(cp.GetSignificantChanges(), ['/Parent'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

        # Flipping the instance flag back should restore us to the
        # original state.
        (pi, err) = pcp.ComputePrimIndex('/Parent')
        self.assertEqual(len(err), 0)
        self.assertFalse(pi.IsInstanceable())
        self.assertEqual(pi.ComputePrimChildNames(), (['RefChild', 'DirectChild'], []))

    def test_InstancingChangesForSubrootArcs(self):
        # First layer: Build up the first reference structure 
        refLayer = Sdf.Layer.CreateAnonymous()
        # Base spec name space hierarchy: /Base/Child/BaseLeaf.
        baseSpec = Sdf.PrimSpec(refLayer, 'Base', Sdf.SpecifierDef)
        baseChildSpec = Sdf.PrimSpec(baseSpec, 'Child', Sdf.SpecifierDef)
        baseLeafSpec = Sdf.PrimSpec(baseChildSpec, 'BaseLeaf', Sdf.SpecifierDef)

        # /A references /Base
        refASpec = Sdf.PrimSpec(refLayer, 'A', Sdf.SpecifierDef)
        refASpec.referenceList.Add(Sdf.Reference('', '/Base'))

        # /B references /A
        refBSpec = Sdf.PrimSpec(refLayer, 'B', Sdf.SpecifierDef)
        refBSpec.referenceList.Add(Sdf.Reference('', '/A'))

        # Second layer: Same structure as layer with incmented names for clarity
        # when both layers have prims referenced
        refLayer2 = Sdf.Layer.CreateAnonymous()
        base2Spec = Sdf.PrimSpec(refLayer2, 'Base2', Sdf.SpecifierDef)
        base2ChildSpec = Sdf.PrimSpec(base2Spec, 'Child', Sdf.SpecifierDef)
        base2LeafSpec = Sdf.PrimSpec(base2ChildSpec, 'Base2Leaf', 
                                     Sdf.SpecifierDef)

        # /A2 references /Base2
        refA2Spec = Sdf.PrimSpec(refLayer2, 'A2', Sdf.SpecifierDef)
        refA2Spec.referenceList.Add(Sdf.Reference('', '/Base2'))

        # /B2 references /A2
        refB2Spec = Sdf.PrimSpec(refLayer2, 'B2', Sdf.SpecifierDef)
        refB2Spec.referenceList.Add(Sdf.Reference('', '/A2'))

        # Root layer: 
        # Instance spec namespace hierarchy: /Instance/Child/InstanceLeaf
        # /Instance references /B2. 
        # /Instance/Child subroot references /B/Child
        # 
        # What this gives us is the following prim index graph for
        # /Instance/Child:
        # 
        # /Instance/Child --> /B/Child -a-> /A/Child -a-> /Base/Child
        #         |
        #         +-a-> /B2/Child -a-> /A2/Child -a-> /Base2/Child
        #
        # All reference arcs are "ancestral" except the direct reference from 
        # /Instance/Child to /B/Child. Ancestral arcs under a direct arc are 
        # treated differently for instancing than normal ancestral arcs.
        layer = Sdf.Layer.CreateAnonymous()
        instanceSpec = Sdf.PrimSpec(layer, 'Instance', Sdf.SpecifierDef)
        instanceSpec.referenceList.Add(
            Sdf.Reference(refLayer2.identifier, '/B2'))
        instanceChildSpec = Sdf.PrimSpec(instanceSpec, 'Child', 
                                         Sdf.SpecifierDef)
        instanceChildSpec.referenceList.Add(
            Sdf.Reference(refLayer.identifier, '/B/Child'))
        instanceLeafSpec = Sdf.PrimSpec(instanceChildSpec, 'InstanceLeaf', 
                                        Sdf.SpecifierDef)

        # Instancing is only enabled in Usd mode.
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(layer), usd=True)

        # Compute our initial prim index. It is not yet instanceable and
        # all leaf nodes appear when computing prim child names.
        (pi, err) = pcp.ComputePrimIndex('/Instance/Child')
        self.assertEqual(len(err), 0)
        self.assertFalse(pi.IsInstanceable())
        self.assertEqual(pi.ComputePrimChildNames(), 
                         (['Base2Leaf', 'BaseLeaf', 'InstanceLeaf'], []))

        # Helper context for verifying the change processing of the changes
        # affecting /Instance/Child
        @contextmanager
        def _VerifyChanges(significant=False, spec=False, prim=False):
            primIndexPath = '/Instance/Child'
            # Verify that we have a computed prim index for /Instance/Child
            # before making the change .
            self.assertTrue(pcp.FindPrimIndex(primIndexPath))
            with Pcp._TestChangeProcessor(pcp) as cp:
                try: 
                    yield cp
                finally:
                    self.assertEqual(cp.GetSignificantChanges(), 
                                     [primIndexPath] if significant else [])
                    self.assertEqual(cp.GetSpecChanges(), 
                                     [primIndexPath] if spec else [])
                    self.assertEqual(cp.GetPrimChanges(), 
                                     [primIndexPath] if prim else [])
                    # Significant and prim changes do invalidate the prim index.
                    if significant or prim:
                        self.assertFalse(pcp.FindPrimIndex(primIndexPath))
                    else:
                        self.assertTrue(pcp.FindPrimIndex(primIndexPath))

        # Add Child spec to /A. This is just a spec change to /Instance/Child
        # as /Instance/Child is not instanceable yet.
        with _VerifyChanges(spec=True):
            Sdf.PrimSpec(refASpec, 'Child', Sdf.SpecifierOver)

        # Add Child spec to /A2. This is just a spec change to /Instance/Child.
        with _VerifyChanges(spec=True):
            Sdf.PrimSpec(refA2Spec, 'Child', Sdf.SpecifierOver)

        # Delete both Child specs we just added.
        with _VerifyChanges(spec=True):
            del refASpec.nameChildren['Child']
        with _VerifyChanges(spec=True):
            del refA2Spec.nameChildren['Child']

        # Now set /Base/Child to instanceable which is always a significant 
        # change.
        with _VerifyChanges(significant=True):
            baseChildSpec.instanceable = True

        # /Instance/Child is now instanceable. BaseLeaf is still returned by
        # ComputePrimChildren because the ancestral node that provides it comes 
        # from a subtree composed for a direct subroot reference arc so it is 
        # part of the instance. Base2Leaf comes from an actual ancestral arc so
        # it gets skipped when computing children as does the local child 
        # opinion for InstanceLeaf.
        (pi, err) = pcp.ComputePrimIndex('/Instance/Child')
        self.assertEqual(len(err), 0)
        self.assertTrue(pi.IsInstanceable())
        self.assertEqual(pi.ComputePrimChildNames(), (['BaseLeaf'], []))

        # Add Child spec to /A2 again now that /Instance/Child is instanceable. 
        # This is still just a spec change to /Instance/Child as true ancestral
        # nodes are ignored by the instance key.
        with _VerifyChanges(spec=True):
            Sdf.PrimSpec(refA2Spec, 'Child', Sdf.SpecifierOver)

        # Add Child spec to /A again. This is a significant change as /A/Child
        # is an ancestral node that's part of a direct arc's subtree and is
        # part of the instance key.
        with _VerifyChanges(significant=True):
            Sdf.PrimSpec(refASpec, 'Child', Sdf.SpecifierOver)

        (pi, err) = pcp.ComputePrimIndex('/Instance/Child')
        self.assertEqual(len(err), 0)
        self.assertTrue(pi.IsInstanceable())
        self.assertEqual(pi.ComputePrimChildNames(), (['BaseLeaf'], []))

    def test_InertPrimChanges(self):
        refLayer = Sdf.Layer.CreateAnonymous()
        refParentSpec = Sdf.PrimSpec(refLayer, 'Parent', Sdf.SpecifierDef)
        refChildSpec = Sdf.PrimSpec(refParentSpec, 'Child', Sdf.SpecifierDef)

        rootLayer = Sdf.Layer.CreateAnonymous()
        parentSpec = Sdf.PrimSpec(rootLayer, 'Parent', Sdf.SpecifierOver)
        parentSpec.referenceList.Add(
            Sdf.Reference(refLayer.identifier, '/Parent'))

        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        # Adding an empty over to a prim that already exists (has specs)
        # is an insignificant change.
        (pi, err) = pcp.ComputePrimIndex('/Parent/Child')
        self.assertEqual(err, [])
        with Pcp._TestChangeProcessor(pcp) as cp:
            Sdf.CreatePrimInLayer(rootLayer, '/Parent/Child')
            self.assertEqual(cp.GetSignificantChanges(), [])
            self.assertEqual(cp.GetSpecChanges(), ['/Parent/Child'])
            self.assertEqual(cp.GetPrimChanges(), [])

        # Adding an empty over as the first spec for a prim is a
        # a significant change, even if we haven't computed a prim index
        # for that path yet.
        with Pcp._TestChangeProcessor(pcp) as cp:
            Sdf.CreatePrimInLayer(rootLayer, '/Parent/NewChild')
            self.assertEqual(cp.GetSignificantChanges(), ['/Parent/NewChild'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

        (pi, err) = pcp.ComputePrimIndex('/Parent/NewChild2')
        self.assertEqual(err, [])
        with Pcp._TestChangeProcessor(pcp) as cp:
            Sdf.CreatePrimInLayer(rootLayer, '/Parent/NewChild2')
            self.assertEqual(cp.GetSignificantChanges(), ['/Parent/NewChild2'])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

    def test_InertPrimRemovalChanges(self):
        subLayer = Sdf.Layer.CreateAnonymous()
        subParentSpec = Sdf.PrimSpec(subLayer, 'Parent', Sdf.SpecifierDef)
        subChildSpec = Sdf.PrimSpec(subParentSpec, 'Child', Sdf.SpecifierDef)
        subAttrSpec = Sdf.AttributeSpec(subChildSpec, 'attr', Sdf.ValueTypeNames.Double)
        subAttrSpec.default = 1.0

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootParentSpec = Sdf.PrimSpec(rootLayer, 'Parent', Sdf.SpecifierOver)
        rootChildSpec = Sdf.PrimSpec(rootParentSpec, 'Child', Sdf.SpecifierOver)
        rootAttrSpec = Sdf.AttributeSpec(rootChildSpec, 'attr', Sdf.ValueTypeNames.Double)
        rootLayer.subLayerPaths.append(subLayer.identifier)

        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

        (pi, err) = pcp.ComputePrimIndex('/Parent')
        self.assertEqual(err, [])
        self.assertEqual(pi.primStack, [rootParentSpec, subParentSpec])

        (pi, err) = pcp.ComputePrimIndex('/Parent/Child')
        self.assertEqual(err, [])
        self.assertEqual(pi.primStack, [rootChildSpec, subChildSpec])

        (pi, err) = pcp.ComputePropertyIndex('/Parent/Child.attr')
        self.assertEqual(err, [])
        self.assertEqual(pi.propertyStack, [rootAttrSpec, subAttrSpec])

        with Pcp._TestChangeProcessor(pcp) as cp:
            del rootLayer.pseudoRoot.nameChildren['Parent']
            self.assertFalse(rootParentSpec)
            self.assertFalse(rootChildSpec)
            self.assertFalse(rootAttrSpec)

            self.assertEqual(cp.GetSignificantChanges(), [])
            self.assertEqual(cp.GetSpecChanges(), 
                             ['/Parent', '/Parent/Child', '/Parent/Child.attr'])
            self.assertEqual(cp.GetPrimChanges(), [])

        (pi, err) = pcp.ComputePrimIndex('/Parent')
        self.assertEqual(err, [])
        self.assertEqual(pi.primStack, [subParentSpec])

        (pi, err) = pcp.ComputePrimIndex('/Parent/Child')
        self.assertEqual(err, [])
        self.assertEqual(pi.primStack, [subChildSpec])
        
        (pi, err) = pcp.ComputePropertyIndex('/Parent/Child.attr')
        self.assertEqual(err, [])
        self.assertEqual(pi.propertyStack, [subAttrSpec])

if __name__ == "__main__":
    unittest.main()
