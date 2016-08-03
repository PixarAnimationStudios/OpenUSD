#!/pxrpythonsubst

from pxr import Sdf, Pcp, Tf

import Mentor.Runtime
from Mentor.Runtime import *

def TestInvalidSublayerRemoval():
    invalidSublayerId = "/tmp/testPcpChanges_invalidSublayer.usda"

    layer = Sdf.Layer.CreateAnonymous()
    layer.subLayerPaths.append(invalidSublayerId)

    layerStackId = Pcp.LayerStackIdentifier(layer)
    pcp = Pcp.Cache(layerStackId)

    AssertEqual(len(pcp.ComputeLayerStack(layerStackId)[1]), 1)
    AssertTrue(pcp.IsInvalidSublayerIdentifier(invalidSublayerId))

    with Pcp._TestChangeProcessor(pcp):
        layer.subLayerPaths.remove(invalidSublayerId)

    AssertEqual(len(pcp.ComputeLayerStack(layerStackId)[1]), 0)
    AssertFalse(pcp.IsInvalidSublayerIdentifier(invalidSublayerId))

TestInvalidSublayerRemoval()

def TestUnusedVariantChanges():
    layer = Sdf.Layer.CreateAnonymous()
    parent = Sdf.PrimSpec(layer, 'Root', Sdf.SpecifierDef, 'Scope')
    vA = Sdf.CreateVariantInLayer(layer, parent.path, 'var', 'A')
    vB = Sdf.CreateVariantInLayer(layer, parent.path, 'var', 'B')
    parent.variantSelections['var'] = 'A'

    layerStackId = Pcp.LayerStackIdentifier(layer)
    pcp = Pcp.Cache(layerStackId)

    (pi, err) = pcp.ComputePrimIndex('/Root')
    AssertTrue(pi)
    AssertEqual(len(err), 0)

    # Add a new prim spec inside the unused variant and verify that this
    # does not cause the cached prim index to be blown.
    with Pcp._TestChangeProcessor(pcp):
        newPrim = Sdf.PrimSpec(vB.primSpec, 'Child', Sdf.SpecifierDef, 'Scope')

    AssertTrue(pcp.FindPrimIndex('/Root'))

TestUnusedVariantChanges()

def TestSublayerOffsetChanges():
    rootLayerPath = FindDataFile(
        'testPcpChanges.testenv/TestSublayerOffsetChanges/root.sdf')
    rootSublayerPath = FindDataFile(
        'testPcpChanges.testenv/TestSublayerOffsetChanges/root-sublayer.sdf')
    refLayerPath = FindDataFile(
        'testPcpChanges.testenv/TestSublayerOffsetChanges/ref.sdf')
    refSublayerPath = FindDataFile(
        'testPcpChanges.testenv/TestSublayerOffsetChanges/ref-sublayer.sdf')
    ref2LayerPath = FindDataFile(
        'testPcpChanges.testenv/TestSublayerOffsetChanges/ref2.sdf')
    
    rootLayer = Sdf.Layer.FindOrOpen(rootLayerPath)
    pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))

    (pi, err) = pcp.ComputePrimIndex('/A')
    AssertTrue(pi)
    AssertEqual(len(err), 0)

    # Verify the expected structure of the test asset. It should simply be
    # a chain of two references, with layer offsets of 100.0 and 50.0
    # respectively.
    refNode = pi.rootNode.children[0]
    AssertEqual(refNode.layerStack.layers, 
                [Sdf.Layer.Find(refLayerPath), Sdf.Layer.Find(refSublayerPath)])
    AssertEqual(refNode.arcType, Pcp.ArcTypeReference)
    AssertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(100.0))

    ref2Node = refNode.children[0]
    AssertEqual(ref2Node.layerStack.layers, [Sdf.Layer.Find(ref2LayerPath)])
    AssertEqual(ref2Node.arcType, Pcp.ArcTypeReference)
    AssertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(150.0))

    # Change the layer offset in the local layer stack and verify that
    # invalidates the prim index and that the updated layer offset is
    # taken into account after recomputing the index.
    with Pcp._TestChangeProcessor(pcp):
        rootLayer.subLayerOffsets[0] = Sdf.LayerOffset(200.0)
    
    AssertFalse(pcp.FindPrimIndex('/A'))
    (pi, err) = pcp.ComputePrimIndex('/A')
    refNode = pi.rootNode.children[0]
    ref2Node = refNode.children[0]

    AssertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(200.0))
    AssertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(250.0))

    # Change the layer offset in a referenced layer stack and again verify
    # that the prim index is invalidated and the updated layer offset is
    # taken into account.
    #
    # XXX: This currently does not work! Commented out for now, see bug 96529.
    #
    # refLayer = refNode.layerStack.layers[0]
    # with Pcp._TestChangeProcessor(pcp):
    #     refLayer.subLayerOffsets[0] = Sdf.LayerOffset(200.0)
    #
    # AssertFalse(pcp.FindPrimIndex('/A'))
    # (pi, err) = pcp.ComputePrimIndex('/A')
    # refNode = pi.rootNode.children[0]
    # ref2Node = refNode.children[0]

    # AssertEqual(refNode.mapToRoot.timeOffset, Sdf.LayerOffset(200.0))
    # AssertEqual(ref2Node.mapToRoot.timeOffset, Sdf.LayerOffset(400.0))

TestSublayerOffsetChanges()

def TestDefaultReferenceTargetChanges():
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
        AssertEqual(pi.rootNode.children[0].path, '/target1')

        # Now clear defaultPrim.  This should issue an error and
        # fail to pick up the referenced prim.
        with Pcp._TestChangeProcessor(pcp):
            targLyr.ClearDefaultPrim()

        (pi, err) = pcp.ComputePrimIndex('/source')
        AssertTrue(isinstance(err[0], Pcp.ErrorUnresolvedPrimPath))
        # If the reference to the defaultPrim is an external reference, 
        # the one child node should be the pseudoroot dependency placeholder.
        # If the reference is an internal reference, that dependency
        # placeholder is unneeded.
        if referencingLayer != targLyr:
            AssertEqual(len(pi.rootNode.children), 1)
            AssertEqual(pi.rootNode.children[0].path, '/')

        # Change defaultPrim to other target.  This should pick
        # up the reference again, but to the new prim target2.
        with Pcp._TestChangeProcessor(pcp):
            targLyr.defaultPrim = 'target2'

        (pi, err) = pcp.ComputePrimIndex('/source')
        AssertEqual(len(err), 0)
        AssertEqual(pi.rootNode.children[0].path, '/target2')

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

TestDefaultReferenceTargetChanges()

def TestInternalReferenceChanges():
    rootLayer = Sdf.Layer.CreateAnonymous()

    Sdf.PrimSpec(rootLayer, 'target1', Sdf.SpecifierDef)
    Sdf.PrimSpec(rootLayer, 'target2', Sdf.SpecifierDef)
    
    srcPrimSpec = Sdf.PrimSpec(rootLayer, 'source', Sdf.SpecifierDef)
    srcPrimSpec.referenceList.Add(Sdf.Reference(primPath = '/target1'))
    
    # Initially, the prim index for /source should contain a single
    # reference node to /target1 in rootLayer.
    pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer))
    (pi, err) = pcp.ComputePrimIndex('/source')
    
    AssertEqual(len(err), 0)
    AssertEqual(pi.rootNode.children[0].layerStack.identifier.rootLayer,
                rootLayer)
    AssertEqual(pi.rootNode.children[0].path, '/target1')

    # Modify the internal reference to point to /target2 and verify the
    # reference node is updated.
    with Pcp._TestChangeProcessor(pcp):
        srcPrimSpec.referenceList.addedItems[0] = \
            Sdf.Reference(primPath = '/target2')

    (pi, err) = pcp.ComputePrimIndex('/source')
    
    AssertEqual(len(err), 0)
    AssertEqual(pi.rootNode.children[0].layerStack.identifier.rootLayer,
                rootLayer)
    AssertEqual(pi.rootNode.children[0].path, '/target2')

    # Clear out all references and verify that the prim index contains no
    # reference nodes.
    with Pcp._TestChangeProcessor(pcp):
        srcPrimSpec.referenceList.ClearEdits()

    (pi, err) = pcp.ComputePrimIndex('/source')
    
    AssertEqual(len(err), 0)
    AssertEqual(len(pi.rootNode.children), 0)

TestInternalReferenceChanges()

def TestVariantChanges(usd):
    rootLayer = Sdf.Layer.CreateAnonymous()
    modelSpec = Sdf.PrimSpec(rootLayer, 'Variant', Sdf.SpecifierDef)

    pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd = usd)

    # Test changes that are emitted as a variant set and variant
    # are created.

    pcp.ComputePrimIndex('/Variant')    
    with Pcp._TestChangeProcessor(pcp) as cp:
        varSetSpec = Sdf.VariantSetSpec(modelSpec, 'test')
        AssertEqual(cp.GetSignificantChanges(), ['/Variant'])
        AssertEqual(cp.GetSpecChanges(), [])
        AssertEqual(cp.GetPrimChanges(), [])

    pcp.ComputePrimIndex('/Variant')    
    with Pcp._TestChangeProcessor(pcp) as cp:
        modelSpec.variantSelections['test'] = 'A'
        AssertEqual(cp.GetSignificantChanges(), ['/Variant'])
        AssertEqual(cp.GetSpecChanges(), [])
        AssertEqual(cp.GetPrimChanges(), [])

    pcp.ComputePrimIndex('/Variant')    
    with Pcp._TestChangeProcessor(pcp) as cp:
        modelSpec.variantSetNameList.Add('test')
        AssertEqual(cp.GetSignificantChanges(), ['/Variant'])
        AssertEqual(cp.GetSpecChanges(), [])
        AssertEqual(cp.GetPrimChanges(), [])

    pcp.ComputePrimIndex('/Variant')    
    with Pcp._TestChangeProcessor(pcp) as cp:
        varSpec = Sdf.VariantSpec(varSetSpec, 'A')
        AssertEqual(cp.GetSignificantChanges(), [])
        AssertEqual(cp.GetSpecChanges(), [])
        # Creating the variant spec adds an inert spec to /Variant's prim
        # stack but also requires rebuilding only /Variant's prim index to
        # account for the new variant node.
        AssertEqual(cp.GetPrimChanges(), ['/Variant'])

    pcp.ComputePrimIndex('/Variant')
    with Pcp._TestChangeProcessor(pcp) as cp:
        varSpec.primSpec.referenceList.Add(
            Sdf.Reference('./dummy.usda', '/Dummy'))
        AssertEqual(cp.GetSignificantChanges(), ['/Variant'])
        AssertEqual(cp.GetSpecChanges(), [])
        AssertEqual(cp.GetPrimChanges(), [])

TestVariantChanges(usd = True)
TestVariantChanges(usd = False)

def TestInstancingChanges(usd):
    refLayer = Sdf.Layer.CreateAnonymous()
    refParentSpec = Sdf.PrimSpec(refLayer, 'Parent', Sdf.SpecifierDef)
    refChildSpec = Sdf.PrimSpec(refParentSpec, 'RefChild', Sdf.SpecifierDef)

    rootLayer = Sdf.Layer.CreateAnonymous()
    parentSpec = Sdf.PrimSpec(rootLayer, 'Parent', Sdf.SpecifierOver)
    parentSpec.referenceList.Add(
        Sdf.Reference(refLayer.identifier, '/Parent'))
    childSpec = Sdf.PrimSpec(parentSpec, 'DirectChild', Sdf.SpecifierDef)

    pcp = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd = usd)

    # /Parent is initially not tagged as an instance, so we should
    # see both RefChild and DirectChild name children.
    (pi, err) = pcp.ComputePrimIndex('/Parent')
    AssertEqual(len(err), 0)
    AssertFalse(pi.IsInstanceable())
    AssertEqual(pi.ComputePrimChildNames(), (['RefChild', 'DirectChild'], []))

    with Pcp._TestChangeProcessor(pcp) as cp:
        parentSpec.instanceable = True
        AssertEqual(cp.GetSignificantChanges(), ['/Parent'])
        AssertEqual(cp.GetSpecChanges(), [])
        AssertEqual(cp.GetPrimChanges(), [])

    # After being made an instance, DirectChild should no longer
    # be reported as a name child since instances may not introduce
    # new prims locally.
    (pi, err) = pcp.ComputePrimIndex('/Parent')
    AssertEqual(len(err), 0)
    AssertTrue(pi.IsInstanceable())
    AssertEqual(pi.ComputePrimChildNames(), (['RefChild'], []))

    with Pcp._TestChangeProcessor(pcp) as cp:
        parentSpec.instanceable = False
        AssertEqual(cp.GetSignificantChanges(), ['/Parent'])
        AssertEqual(cp.GetSpecChanges(), [])
        AssertEqual(cp.GetPrimChanges(), [])

    # Flipping the instance flag back should restore us to the
    # original state.
    (pi, err) = pcp.ComputePrimIndex('/Parent')
    AssertEqual(len(err), 0)
    AssertFalse(pi.IsInstanceable())
    AssertEqual(pi.ComputePrimChildNames(), (['RefChild', 'DirectChild'], []))

TestInstancingChanges(usd = True)
# Instancing is disabled in Usd mode, so we don't test
# there.
# TestInstancingChanges(usd = False)

# All done!
Mentor.Runtime.ExitTest()
