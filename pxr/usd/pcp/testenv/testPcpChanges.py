#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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

        # Adding an empty sublayer should not require recomputing any prim
        # indexes or change their prim stacks, but it should change the used
        # layers revision count.
        revCount = pcp.GetUsedLayersRevision()
        with Pcp._TestChangeProcessor(pcp):
            rootLayer.subLayerPaths.insert(0, subLayer2.identifier)

        pi = pcp.FindPrimIndex('/Test')
        self.assertTrue(pi)
        self.assertEqual(pi.primStack, [primSpec])
        self.assertNotEqual(revCount, pcp.GetUsedLayersRevision())

        # Same with deleting an empty sublayer.
        revCount = pcp.GetUsedLayersRevision()
        with Pcp._TestChangeProcessor(pcp):
            del rootLayer.subLayerPaths[0]

        pi = pcp.FindPrimIndex('/Test')
        self.assertTrue(pi)
        self.assertEqual(pi.primStack, [primSpec])
        self.assertNotEqual(revCount, pcp.GetUsedLayersRevision())

    def test_InvalidSublayerAdd(self):
        invalidSublayerId = "/tmp/testPcpChanges_invalidSublayer.sdf"

        layer = Sdf.Layer.CreateAnonymous()
        layerStackId = Pcp.LayerStackIdentifier(layer)
        pcp = Pcp.Cache(layerStackId)

        (layerStack, errs) = pcp.ComputeLayerStack(layerStackId)
        self.assertEqual(len(errs), 0)
        self.assertEqual(len(layerStack.localErrors), 0)
        self.assertTrue(pcp.UsesLayerStack(layerStack))

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
        self.assertTrue(pcp.UsesLayerStack(layerStack))

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
        self.assertTrue(pcp.UsesLayerStack(layerStack))

    def test_AddAndRemoveSublayers(self):
        sub1Layer = Sdf.Layer.CreateAnonymous('sub1')
        sub1Layer.ImportFromString('''
        #sdf 1.4.32
        
        def "A"
        {
        }
        ''')

        sub2Layer = Sdf.Layer.CreateAnonymous('sub2')
        sub2Layer.ImportFromString('''
        #sdf 1.4.32
        
        def "B"
        {
        }
        ''')

        defLayer = Sdf.Layer.CreateAnonymous('def')
        defLayer.ImportFromString('''
        #sdf 1.4.32

        def "A"
        {
        }

        def "B"
        {
        }
        ''')

        rootLayer = Sdf.Layer.CreateAnonymous('root')
        rootLayer.ImportFromString(f'''\
        #sdf 1.4.32
        (
            subLayers = [
                @{sub1Layer.identifier}@,
                @{defLayer.identifier}@
            ]
        )
        ''')

        layerStackId = Pcp.LayerStackIdentifier(rootLayer)
        pcp = Pcp.Cache(layerStackId)

        (pi, err) = pcp.ComputePrimIndex('/A')
        (pi, err) = pcp.ComputePrimIndex('/B')

        with Pcp._TestChangeProcessor(pcp) as cp:
            with Sdf.ChangeBlock():
                rootLayer.subLayerPaths.insert(0, sub2Layer.identifier)
                del rootLayer.subLayerPaths[1]

            # With incremental changes these changes should only cause a resync
            # of /A and /B.
            self.assertEqual(cp.GetSignificantChanges(), 
                             [Sdf.Path('/A'), Sdf.Path('/B')])
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(), [])

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

        revCount = pcp.GetUsedLayersRevision()
        with Pcp._TestChangeProcessor(pcp) as cp:
            del rootLayer.pseudoRoot.nameChildren['Parent']
            self.assertFalse(rootParentSpec)
            self.assertFalse(rootChildSpec)
            self.assertFalse(rootAttrSpec)

            self.assertEqual(cp.GetSignificantChanges(), [])
            self.assertEqual(cp.GetSpecChanges(), 
                             ['/Parent', '/Parent/Child', '/Parent/Child.attr'])
            self.assertEqual(cp.GetPrimChanges(), [])

        self.assertEqual(revCount, pcp.GetUsedLayersRevision())

        (pi, err) = pcp.ComputePrimIndex('/Parent')
        self.assertEqual(err, [])
        self.assertEqual(pi.primStack, [subParentSpec])

        (pi, err) = pcp.ComputePrimIndex('/Parent/Child')
        self.assertEqual(err, [])
        self.assertEqual(pi.primStack, [subChildSpec])
        
        (pi, err) = pcp.ComputePropertyIndex('/Parent/Child.attr')
        self.assertEqual(err, [])
        self.assertEqual(pi.propertyStack, [subAttrSpec])

    def test_ChangesToCulledAncestralNodes(self):
        layer = Sdf.Layer.FindOrOpen('TestCulledAncestralNodes/root.sdf')
        pcp = Pcp.Cache(Pcp.LayerStackIdentifier(layer))

        (pi, err) = pcp.ComputePrimIndex(
            '/FSToyCarA/Looks/PaintedWood_PaintedYellow')
        self.assertEqual(err, [])
 
        with Pcp._TestChangeProcessor(pcp) as cp:
            Sdf.CreatePrimInLayer(
                layer, '/FSToyCarA_defaultShadingVariant/Looks/PaintedWood')

            # These significant changes are surprising but expected; Pcp
            # detects that the addition of these specs would affect the prim
            # indexes at these paths in the root layer stack, but since we
            # haven't computed these prim indexes Pcp assumes these require
            # a significant resync. We could probably improve this.
            self.assertEqual(cp.GetSignificantChanges(),
                             ['/FSToyCarA/Looks/PaintedWood',
                              '/FSToyCarA_defaultShadingVariant/Looks/PaintedWood'])
            
            self.assertEqual(cp.GetSpecChanges(), [])
            self.assertEqual(cp.GetPrimChanges(),
                             ['/FSToyCarA/Looks/PaintedWood_PaintedYellow'])

    def test_AddMuteRemoveSublayerWithRelocates(self):
        """Tests that adding/muting/removing sublayers that only define layer 
           relocates will invalidate affected cached prim indexes."""

        # Start with simple setup. Root layer has one prim that references 
        # a simple hierarchy in ref layer
        refLayer = Sdf.Layer.CreateAnonymous()
        refLayer.ImportFromString(
            '''#sdf 1.4.32
                def "Ref" {
                    def "A" {
                        def "B" {
                            def "C" {
                            }                         
                        }
                    }
                }''')

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString(
            '''#sdf 1.4.32
                def "Root" (
                    references = @''' + refLayer.identifier + '''@</Ref>
                ) {}''')

        # Create the PcpCache
        layerStackId = Pcp.LayerStackIdentifier(rootLayer)
        cache = Pcp.Cache(layerStackId, usd=True)

        from contextlib import contextmanager
        @contextmanager
        def _VerifyChanges(primIndexPaths):
            # Verify that we have a computed prim index for all expected paths
            # beforehand
            for path in primIndexPaths:
                self.assertTrue(cache.FindPrimIndex(path))
            try:
                yield
            finally:
                for path in primIndexPaths:
                    self.assertFalse(cache.FindPrimIndex(path))

        # Verify computed prim indexes for initial setup

        # /Root has child 'A'
        pi, err = cache.ComputePrimIndex('/Root')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['A'], []))

        # /Root/A has child 'B'
        pi, err = cache.ComputePrimIndex('/Root/A')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['B'], []))

        # /Root/A/B has child 'C'
        pi, err = cache.ComputePrimIndex('/Root/A/B')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['C'], []))

        # Create a layer whose only contents is a relocate from /Root/A to
        # /Root/Foo.
        subLayer1 = Sdf.Layer.CreateAnonymous()
        subLayer1.ImportFromString(
            '''#sdf 1.4.32
                (
                    relocates = {
                        </Root/A> : </Root/Foo>
                    }
                )''')
        self.assertFalse(subLayer1.empty)

        # Add this layer as sublayer of root and verify all our previously
        # cached prim indices have been invalidated.
        with _VerifyChanges(['/Root', '/Root/A', '/Root/A/B']):
            with Pcp._TestChangeProcessor(cache) as cp:
                rootLayer.subLayerPaths = [subLayer1.identifier]
                self.assertEqual(cp.GetSignificantChanges(), ['/'])

        # Verify computed prim indexes with new relocates

        # /Root has child 'Foo' and prohibits child 'A'
        pi, err = cache.ComputePrimIndex('/Root')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['Foo'], ['A']))

        # '/Root/Foo' has child 'B'
        pi, err = cache.ComputePrimIndex('/Root/Foo')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['B'], []))

        # '/Root/Foo/B' has child 'C'
        pi, err = cache.ComputePrimIndex('/Root/Foo/B')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['C'], []))

        # Create another new layer whose only contents is a relocate from 
        # /Root/Foo/B to /Root/Foo/Bar.
        subLayer2 = Sdf.Layer.CreateAnonymous()
        subLayer2.ImportFromString(
            '''#sdf 1.4.32
                (
                    relocates = {
                        </Root/Foo/B> : </Root/Foo/Bar>
                    }
                )''')
        self.assertFalse(subLayer2.empty)

        # Add this layer as an additional sublayer of root and verify all our 
        # previously cached prim indices have been invalidated.
        with _VerifyChanges(['/Root', '/Root/Foo', '/Root/Foo/B']):
            with Pcp._TestChangeProcessor(cache) as cp:
                rootLayer.subLayerPaths = [
                    subLayer1.identifier, subLayer2.identifier]
                self.assertEqual(cp.GetSignificantChanges(), ['/'])

        # Verify computed prim indexes with new relocates

        # /Root has child 'Foo' and prohibits child 'A'
        pi, err = cache.ComputePrimIndex('/Root')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['Foo'], ['A']))

        # '/Root/Foo' has child 'Bar' and prohibits child 'B'
        pi, err = cache.ComputePrimIndex('/Root/Foo')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['Bar'], ['B']))

        # '/Root/Foo/Bar' has child 'C'
        pi, err = cache.ComputePrimIndex('/Root/Foo/Bar')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['C'], []))

        # Request muting of sublayer 2 and verify all our 
        # previously cached prim indices have been invalidated.
        with _VerifyChanges(['/Root', '/Root/Foo', '/Root/Foo/Bar']):
            cache.RequestLayerMuting([subLayer2.identifier], [])

        # Computed prim indexes and children will be back to the same as before
        # we added sublayer 2
        pi, err = cache.ComputePrimIndex('/Root')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['Foo'], ['A']))

        pi, err = cache.ComputePrimIndex('/Root/Foo')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['B'], []))

        pi, err = cache.ComputePrimIndex('/Root/Foo/B')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['C'], []))

        # Request unmuting of sublayer 2 and verify all our 
        # previously cached prim indices have been invalidated.
        with _VerifyChanges(['/Root', '/Root/Foo', '/Root/Foo/B']):
            cache.RequestLayerMuting([],[subLayer2.identifier])

        # Computed prim indexes and children will be restored to the same as 
        # before we muted sublayer 2
        pi, err = cache.ComputePrimIndex('/Root')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['Foo'], ['A']))

        pi, err = cache.ComputePrimIndex('/Root/Foo')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['Bar'], ['B']))

        pi, err = cache.ComputePrimIndex('/Root/Foo/Bar')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['C'], []))

        # Delete both sublayers from the root prim
        with _VerifyChanges(['/Root', '/Root/Foo', '/Root/Foo/Bar']):
            with Pcp._TestChangeProcessor(cache) as cp:
                rootLayer.subLayerPaths = []
                self.assertEqual(cp.GetSignificantChanges(), ['/'])

        # Verify computed prim indexes are back to the initial setup
        pi, err = cache.ComputePrimIndex('/Root')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['A'], []))

        pi, err = cache.ComputePrimIndex('/Root/A')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['B'], []))

        pi, err = cache.ComputePrimIndex('/Root/A/B')
        self.assertFalse(len(err))
        self.assertEqual(pi.ComputePrimChildNames(), (['C'], []))

    def test_RelocatesMapFunctionChanges(self):
        """Tests that adding/removing relocates correctly updates map functions
        in reference nodes that are needed for target path mapping. Also tests
        the behavior related to what prim indexes get invalidate when these
        relocates are edited."""

        # Test layer setup: /Root prim in root layer references /Ref prim in
        # ref layer which references /Model prim in ref layer 2. /Model has 
        # some descendants we can relocate.
        # Also /OtherRoot prim in root layer references /OtherRef in ref
        # ref layer which references the same /Model in ref layer 2. Nothing
        # in /OtherRoot will be relocated but is present to demonstrate
        # the specific cache invalidation effects when relocates are added
        # for the first time.
        modelLayer = Sdf.Layer.CreateAnonymous()
        modelLayer.ImportFromString('''#sdf 1.4.32
            def "Model" {
                def "PreRelo" {
                    def "PreReloChild" {
                    }
                }
            }''')

        refLayer = Sdf.Layer.CreateAnonymous()
        refLayer.ImportFromString('''#sdf 1.4.32
            def "Ref" (
                references = @''' + modelLayer.identifier + '''@</Model>
            ) {}
            
            def "OtherRef" (
                references = @''' + modelLayer.identifier + '''@</Model>
            ) {}
            ''')

        rootLayer = Sdf.Layer.CreateAnonymous()
        rootLayer.ImportFromString('''#sdf 1.4.32
            def "Root" (
                references = @''' + refLayer.identifier + '''@</Ref>
            ) {}
            
            def "OtherRoot" (
                references = @''' + refLayer.identifier + '''@</OtherRef>
            ) {}
            ''')

        # Create the cache for the root layer.
        layerStackId = Pcp.LayerStackIdentifier(rootLayer)
        cache = Pcp.Cache(layerStackId, usd=True)

        # Helper function that finds cached prim indexes or computes a new prim
        # index for every prim in the namespace hierarchy. Then it verifies that
        # found and computed prim index paths match the paths we expected to be
        # found and computed. The end result is every prim in the namespace 
        # hierarchy will have a cached prim index when this is complete.
        def _FindOrComputeAllPrimIndexes(
                expectedFoundPaths, expectedComputedPaths):
            foundPaths = []
            computedPaths = []

            def _FindOrComputeRecursive(path):
                pi = cache.FindPrimIndex(path)
                if pi:
                    foundPaths.append(path)
                else:
                    pi, err = cache.ComputePrimIndex(path)
                    self.assertFalse(err)
                    computedPaths.append(path)
                # Whether found or computed the prim index will be cache for this
                # path.
                self.assertTrue(cache.FindPrimIndex(path))

                # Compute children of this path and recurse
                childNames, prohibitedChildNames = pi.ComputePrimChildNames()
                for child in childNames:
                    childPath = path.AppendChild(child)
                    _FindOrComputeRecursive(childPath)

            _FindOrComputeRecursive(Sdf.Path.absoluteRootPath)

            self.assertEqual(foundPaths, expectedFoundPaths)
            self.assertEqual(computedPaths, expectedComputedPaths)

        # Helper for verifying the map to parent functions on the two reference
        # nodes in computed prim index for /Root evaluate to the expected 
        # mappings. The prim index graph for /Root in this test should always be:
        # 
        # @rootLayer@</Root> --ref--> @refLayer@</Ref> --ref--> @modelLayer@</Model>
        #      
        def _VerifyRootPrimIndexRefNodeMapToParentFunctions(refNode1Map, refNode2Map):
            pi = cache.FindPrimIndex('/Root')
            self.assertTrue(pi)
            refNode1 = pi.rootNode.children[0]
            refNode2 = refNode1.children[0]
            self.assertEqual(refNode1.mapToParent.Evaluate(),
                             Pcp.MapFunction(refNode1Map))
            self.assertEqual(refNode2.mapToParent.Evaluate(),
                             Pcp.MapFunction(refNode2Map))

        # Helper for setting a layer's relocates metadata to a new value with
        # change processing and verifying the expected paths are logged as 
        # significant changes, removing their prim indexes from the cache
        def _SetRelocatesAndVerify(layer, relocates, expectedSignificantChanges):
            with Pcp._TestChangeProcessor(cache) as cp:
                if relocates is None:
                    layer.ClearRelocates()
                else:
                    layer.relocates = relocates
                # Relocates changes will only produce significant changes and
                # no prim or spec changes.
                self.assertEqual(cp.GetSignificantChanges(),
                                 expectedSignificantChanges)
                self.assertFalse(cp.GetPrimChanges())
                self.assertFalse(cp.GetSpecChanges())

            for path in expectedSignificantChanges:
                self.assertFalse(cache.FindPrimIndex(path))

        # To start, compute all prim indexes with no relocates
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = [],
            expectedComputedPaths = [
                '/', '/Root', '/Root/PreRelo', '/Root/PreRelo/PreReloChild', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'])

        # Without relocates, both of /Root's refence nodes' map to parent 
        # functions are the single arc mapping from referencee to referencer
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { '/Ref': '/Root' },
            refNode2Map = { '/Model': '/Ref' })

        # First case: we'll add relocates to the root layer.

        # Relocate /Root/PreRelo to /Root/Relocated in the root layer.
        #
        # Because this is the first relocate added to this layer stack, 
        # this causes a significant change to the entire hierarchy starting from
        # the pseudoroot. This is because we don't store map expression 
        # variables when there are no relocates for a layer stack to avoid the
        # memory cost of tracking these in caches that never have relocates. The
        # trade off is we pay a resync cost, and subsequently the memory cost, 
        # on the first edit that introduces relocates.
        _SetRelocatesAndVerify(
            layer = rootLayer, 
            relocates = [('/Root/PreRelo', '/Root/Relocated')], 
            expectedSignificantChanges = ['/'])
        # All prim indexes have to be recomputed but now /Root/PreRelo is 
        # renamed to /Root/Relocated.
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = [],
            expectedComputedPaths = [
                '/', '/Root', '/Root/Relocated', '/Root/Relocated/PreReloChild', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'])
        # /Root's first reference node's map to parent function now includes the 
        # relocation mapping (for target and connection path mapping).
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { 
                '/Ref': '/Root', 
                '/Ref/PreRelo': '/Root/Relocated' },
            refNode2Map = { '/Model': '/Ref' })

        # Now add an additional relocate /Root/Relocated/PreReloChild to 
        # /Root/Relocated/RelocatedChild in the root layer.
        #
        # This time, because we already had relocates on the root layer, 
        # we only have significant changes on the relocated prim's prior
        # path and new path.
        _SetRelocatesAndVerify(
            layer = rootLayer, 
            relocates = [
                ('/Root/PreRelo', '/Root/Relocated'),
                ('/Root/Relocated/PreReloChild', '/Root/Relocated/RelocatedChild')], 
            expectedSignificantChanges = [
                '/Root/Relocated/PreReloChild', '/Root/Relocated/RelocatedChild'])
        # Likewise, all prim indexes except /Root/Relocated/PreReloChild should
        # be found in the cache and we need to compute the prim index at the 
        # new path /Root/Relocated/RelocatedChild
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = [
                '/', '/Root', '/Root/Relocated', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'],
            expectedComputedPaths = ['/Root/Relocated/RelocatedChild'])
        # /Root's first reference node's map to parent function now includes 
        # both relocation mappings.
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { 
                '/Ref': '/Root', 
                '/Ref/PreRelo': '/Root/Relocated', 
                '/Ref/PreRelo/PreReloChild': '/Root/Relocated/RelocatedChild' },
            refNode2Map = { '/Model': '/Ref' })
            
        # Now clear all relocates on the root layer
        #
        # Just like when we added the first relocate to this layer stack,
        # deleting all relocates from the layer stack also causes a 
        # significant change to the whole hierarchy starting at the pseudoroot.
        # This is the reciprocal of adding the first relocate and regains the 
        # memory no longer needed for tracking relocates. But the trade off 
        # again is a full resync.
        _SetRelocatesAndVerify(
            layer = rootLayer, 
            relocates = None, 
            expectedSignificantChanges = ['/'])
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = [],
            expectedComputedPaths = [
                '/', '/Root', '/Root/PreRelo', '/Root/PreRelo/PreReloChild', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'
            ])
        # The reference nodes' map functions return to their original functions
        # without relocation mappings.
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { '/Ref': '/Root' },
            refNode2Map = { '/Model': '/Ref' })

        # Second case: we'll add relocates to the first ref layer.

        # Relocate /Ref/PreRelo to /Ref/Relocated in the ref layer.
        #
        # This is the first relocate added to the reference's layer stack, 
        # so this causes a significant change to any prim index in our cache
        # that depends on the ref layer. In this cache, that is the prim at
        # /Root AND the prim at /OtherRoot even though only /Root is 
        # actually affected by the relocates.
        _SetRelocatesAndVerify(
            layer = refLayer, 
            relocates = [('/Ref/PreRelo', '/Ref/Relocated')], 
            expectedSignificantChanges = ['/OtherRoot', '/Root'])
        # The pseudoroot prim index does not have to be recomputed, however all
        # prim indexes have to be recomputed since both root level prims were 
        # invalidated. Also /Root/PreRelo is renamed to /Root/Relocated as it
        # is now relocated across the reference.
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = ['/'],
            expectedComputedPaths = [
                '/Root', '/Root/Relocated', '/Root/Relocated/PreReloChild', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'
            ])
        # /Root's second reference node's map to parent function now includes 
        # the relocation mapping from the ref layer.
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { '/Ref': '/Root' },
            refNode2Map = {
                '/Model': '/Ref',
                '/Model/PreRelo': '/Ref/Relocated' })

        # Now add an additional relocate /Ref/Relocated/PreReloChild to 
        # /Ref/Relocated/RelocatedChild in the ref layer.
        #
        # This time, because we already had relocates on the ref layer, 
        # we only have significant changes on the prim indexes with dependencies
        # on relocated prim's prior path and new path.
        _SetRelocatesAndVerify(
            layer = refLayer, 
            relocates = [
                ('/Ref/PreRelo', '/Ref/Relocated'),
                ('/Ref/Relocated/PreReloChild', '/Ref/Relocated/RelocatedChild')], 
            expectedSignificantChanges = [
                '/Root/Relocated/PreReloChild', '/Root/Relocated/RelocatedChild'])
        # Likewise, all prim indexes except /Root/Relocated/PreReloChild should
        # be found in the cache and we need to compute the prim index at the 
        # new path /Root/Relocated/RelocatedChild
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = [
                '/', '/Root', '/Root/Relocated', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'],
            expectedComputedPaths = ['/Root/Relocated/RelocatedChild'])
        # /Root's second reference node's map to parent function now includes 
        # both relocation mappings.
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { '/Ref': '/Root' },
            refNode2Map = { 
                '/Model': '/Ref',
                '/Model/PreRelo': '/Ref/Relocated',
                '/Model/PreRelo/PreReloChild': '/Ref/Relocated/RelocatedChild'})

        # Now set the relocates to explicitly empty on the ref layer (which is
        # different than clearing the relocates metadata).
        #
        # Just like clearing the relocates on the root layer, making relocates
        # explicitly empty has the same effect where we need to resync 
        # everything that depends on any paths in the ref layer which again 
        # is /Root and /OtherRoot.
        _SetRelocatesAndVerify(
            layer = refLayer, 
            relocates = [], 
            expectedSignificantChanges = ['/OtherRoot', '/Root'])
        _FindOrComputeAllPrimIndexes(
            expectedFoundPaths = ['/'],
            expectedComputedPaths = [
                '/Root', '/Root/PreRelo', '/Root/PreRelo/PreReloChild', 
                '/OtherRoot', '/OtherRoot/PreRelo', '/OtherRoot/PreRelo/PreReloChild'])
        # The reference nodes' map functions return to their original functions
        # without relocation mappings.
        _VerifyRootPrimIndexRefNodeMapToParentFunctions(
            refNode1Map = { '/Ref': '/Root' },
            refNode2Map = { '/Model': '/Ref' }
        )

    def test_NestedRelocatesChanges(self):
        """Regression test for a bug where the changes to relocate would not
        invalidate prim indexes that were affected by nested relocates of the
        changed relocate. In this particular test example, the /GrandChild
        prim index was previously not being invalidated for the relocates 
        changes made here. This case particularly makes sure that it is now
        correctly invalidated."""

        # First a layer with prims /World/Foo and /World/FooBar each with the 
        # same Child and GrandChild prims.
        layer1 = Sdf.Layer.CreateAnonymous("layer1.sdf")
        layer1.ImportFromString('''#sdf 1.4.32
        def "World" {
            def "Foo" {
                def "Child" {
                    def "GrandChild" {           
                    }
                }    
            }
        
            def "FooBar" {
                def "Child" {
                    def "GrandChild" {           
                    }
                }    
            }   
        }
        ''')
        
        # Second a layer that has a prim that references /World and then 
        # relocates that flatten the "Foo" hierarchy to individual root prims
        layer2 = Sdf.Layer.CreateAnonymous("layer2.sdf")
        layer2.ImportFromString('''#sdf 1.4.32
        (
            relocates = {
                </Prim/Foo> : </Foo>,
                </Foo/Child> : </Child>,
                </Child/GrandChild> : </GrandChild>
            }
        )
        
        
        def "Prim" (
            references = @''' + layer1.identifier + '''@</World>
        ) {
        }
        
        
        ''') 

        # Create a cache with layer2 root
        layerStackId = Pcp.LayerStackIdentifier(layer2)
        cache = Pcp.Cache(layerStackId, usd=True)

        # Compute the prim indexes for the flattened relocated prims.
        # The prim stacks for each contains the correct spec under /World/Foo
        # for each relocated reference prim.
        foo, _ = cache.ComputePrimIndex('/Foo')
        child, _ = cache.ComputePrimIndex('/Child')
        grandChild, _ = cache.ComputePrimIndex('/GrandChild')

        self.assertEqual(foo.primStack, 
                         [layer1.GetPrimAtPath('/World/Foo')])
        self.assertEqual(child.primStack, 
                         [layer1.GetPrimAtPath('/World/Foo/Child')])
        self.assertEqual(grandChild.primStack, 
                         [layer1.GetPrimAtPath('/World/Foo/Child/GrandChild')])
        
        # Update the relocates so that /Foo is relocated from /Prim/FooBar 
        # instead of /Prim/Foo.
        with Pcp._TestChangeProcessor(cache):
            layer2.relocates = [
                ('/Prim/FooBar', '/Foo'), 
                ('/Foo/Child', '/Child'), 
                ('/Child/GrandChild', '/GrandChild')]

        # Verify the prim index for each relocated prim has been invalidated.
        self.assertFalse(cache.FindPrimIndex('/Foo'))
        self.assertFalse(cache.FindPrimIndex('/Child'))
        self.assertFalse(cache.FindPrimIndex('/GrandChild'))

        # Compute the prim indexes for the flattened relocated prims.
        # The prim stacks for each now contains the new correct spec under 
        # /World/FooBar for each relocated reference prim.
        foo, _ = cache.ComputePrimIndex('/Foo')
        child, _ = cache.ComputePrimIndex('/Child')
        grandChild, _ = cache.ComputePrimIndex('/GrandChild')

        self.assertEqual(foo.primStack, 
                         [layer1.GetPrimAtPath('/World/FooBar')])
        self.assertEqual(child.primStack, 
                         [layer1.GetPrimAtPath('/World/FooBar/Child')])
        self.assertEqual(grandChild.primStack, 
                         [layer1.GetPrimAtPath('/World/FooBar/Child/GrandChild')])
        
        # Update the relocates so that /Foo is relocated from /Prim/Bogus 
        # which does not exist..
        with Pcp._TestChangeProcessor(cache):
            layer2.relocates = [
                ('/Prim/Bogus', '/Foo'), 
                ('/Foo/Child', '/Child'), 
                ('/Child/GrandChild', '/GrandChild')]

        # Verify the prim index for each relocated prim has been invalidated.
        self.assertFalse(cache.FindPrimIndex('/Foo'))
        self.assertFalse(cache.FindPrimIndex('/Child'))
        self.assertFalse(cache.FindPrimIndex('/GrandChild'))

        # Compute the prim indexes for the flattened relocated prims.
        # The prim stacks for each are now all empty because of the bogus
        # relocates.
        foo, _ = cache.ComputePrimIndex('/Foo')
        child, _ = cache.ComputePrimIndex('/Child')
        grandChild, _ = cache.ComputePrimIndex('/GrandChild')

        self.assertEqual(foo.primStack, [])
        self.assertEqual(child.primStack, [])
        self.assertEqual(grandChild.primStack, [])
        
        # Update the relocates so that /Foo is relocated from /Prim/Foo again 
        # like at the start
        with Pcp._TestChangeProcessor(cache):
            layer2.relocates = [
                ('/Prim/Foo', '/Foo'), 
                ('/Foo/Child', '/Child'), 
                ('/Child/GrandChild', '/GrandChild')]

        # Verify the prim index for each relocated prim has been invalidated.
        self.assertFalse(cache.FindPrimIndex('/Foo'))
        self.assertFalse(cache.FindPrimIndex('/Child'))
        self.assertFalse(cache.FindPrimIndex('/GrandChild'))

        # Compute the prim indexes for the flattened relocated prims.
        # The prim stacks for each contains the correct spec under /World/Foo
        # again for each relocated reference prim.
        foo, _ = cache.ComputePrimIndex('/Foo')
        child, _ = cache.ComputePrimIndex('/Child')
        grandChild, _ = cache.ComputePrimIndex('/GrandChild')

        self.assertEqual(foo.primStack, 
                         [layer1.GetPrimAtPath('/World/Foo')])
        self.assertEqual(child.primStack, 
                         [layer1.GetPrimAtPath('/World/Foo/Child')])
        self.assertEqual(grandChild.primStack, 
                         [layer1.GetPrimAtPath('/World/Foo/Child/GrandChild')])

if __name__ == "__main__":
    unittest.main()
