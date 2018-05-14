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
        (pi, err) = pcp.ComputePrimIndex('/A')
        refNode = pi.rootNode.children[0]
        ref2Node = refNode.children[0]

        self.assertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(200.0))
        self.assertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(400.0))

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

    # Instancing is disabled in Usd mode, so we don't test there.
    # TestInstancingChanges(usd = False)
    def test_InstancingChanges(self):
        refLayer = Sdf.Layer.CreateAnonymous()
        refParentSpec = Sdf.PrimSpec(refLayer, 'Parent', Sdf.SpecifierDef)
        refChildSpec = Sdf.PrimSpec(refParentSpec, 'RefChild', Sdf.SpecifierDef)

        rootLayer = Sdf.Layer.CreateAnonymous()
        parentSpec = Sdf.PrimSpec(rootLayer, 'Parent', Sdf.SpecifierOver)
        parentSpec.referenceList.Add(
            Sdf.Reference(refLayer.identifier, '/Parent'))
        childSpec = Sdf.PrimSpec(parentSpec, 'DirectChild', Sdf.SpecifierDef)

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

    
if __name__ == "__main__":
    unittest.main()
