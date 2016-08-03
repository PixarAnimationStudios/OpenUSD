#!/pxrpythonsubst

from Mentor.Runtime import Assert, AssertEqual, FindDataFile, RequiredException
from pxr import Sdf, Pcp, Tf

def LoadLayer(layerPath):
    return Sdf.Layer.FindOrOpen(FindDataFile(layerPath))

def LoadPcpCache(layerPath, sessionLayerPath=None):
    layer = LoadLayer(layerPath)
    sessionLayer = None if not sessionLayerPath else LoadLayer(sessionLayerPath)
    return Pcp.Cache(Pcp.LayerStackIdentifier(layer, sessionLayer))

def TestMutingSublayers():
    """Tests muting sublayers"""
    print "TestMutingSublayers..."

    layer = LoadLayer('testPcpLayerMuting.testenv/sublayers/root.sdf')
    sublayer = LoadLayer('testPcpLayerMuting.testenv/sublayers/sublayer.sdf')
    anonymousSublayer = Sdf.Layer.CreateAnonymous('.sdf')

    # Add a prim in an anonymous sublayer to the root layer for 
    # testing purposes.
    Sdf.CreatePrimInLayer(anonymousSublayer, '/Root')
    layer.subLayerPaths.append(anonymousSublayer.identifier)

    # Create two Pcp.Caches for the same layer stack. This is to verify
    # that muting/unmuting a layer in one cache does not affect any
    # other that shares the same layers.
    pcp = LoadPcpCache(layer.identifier)
    pcp2 = LoadPcpCache(layer.identifier)

    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    # Muting the cache's root layer is explicitly disallowed.
    with RequiredException(Tf.ErrorException):
        pcp.RequestLayerMuting([layer.identifier], [])

    # Mute sublayer and verify that change processing has occurred
    # and that it no longer appears in /Root's prim stack.
    pcp.RequestLayerMuting([sublayer.identifier], [])
    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    # Unmute sublayer and verify that it comes back into /Root's
    # prim stack.
    pcp.RequestLayerMuting([], [sublayer.identifier])
    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    # Mute sublayer and verify that change processing has occurred
    # and that it no longer appears in /Root's prim stack.
    pcp.RequestLayerMuting([anonymousSublayer.identifier], [])
    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root')])
    Assert(anonymousSublayer)

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    # Unmute sublayer and verify that it comes back into /Root's
    # prim stack.
    pcp.RequestLayerMuting([], [anonymousSublayer.identifier])
    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.primStack, 
                [layer.GetPrimAtPath('/Root'),
                 sublayer.GetPrimAtPath('/Root'),
                 anonymousSublayer.GetPrimAtPath('/Root')])
    
TestMutingSublayers()

def TestMutingSessionLayer():
    """Tests ability to mute a cache's session layer."""
    print "TestMutingSessionLayer..."
    
    layer = LoadLayer('testPcpLayerMuting.testenv/session/root.sdf')
    sessionLayer = LoadLayer('testPcpLayerMuting.testenv/session/session.sdf')

    pcp = LoadPcpCache(layer.identifier, sessionLayer.identifier)

    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack,
                [sessionLayer.GetPrimAtPath('/Root'),
                 layer.GetPrimAtPath('/Root')])

    pcp.RequestLayerMuting([sessionLayer.identifier], [])

    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack,
                [layer.GetPrimAtPath('/Root')])

    pcp.RequestLayerMuting([], [sessionLayer.identifier])

    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.primStack,
                [sessionLayer.GetPrimAtPath('/Root'),
                 layer.GetPrimAtPath('/Root')])

TestMutingSessionLayer()

def TestMutingReferencedLayers():
    """Tests behavior when muting and unmuting the root layer of 
    a referenced layer stack."""
    print "TestMutingReferencedLayers..."

    rootLayer = LoadLayer('testPcpLayerMuting.testenv/refs/root.sdf')
    refLayer = LoadLayer('testPcpLayerMuting.testenv/refs/ref.sdf')

    pcp = LoadPcpCache(rootLayer.identifier)
    pcp2 = LoadPcpCache(rootLayer.identifier)

    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.rootNode.children[0].arcType, Pcp.ArcTypeReference)
    AssertEqual(pi.rootNode.children[0].layerStack.layers[0], refLayer)

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
    AssertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

    # Mute the root layer of the referenced layer stack. This should
    # result in change processing, a composition error when recomposing
    # /Root, and no reference node on the prim index.
    pcp.RequestLayerMuting([refLayer.identifier], [])
    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(err)
    AssertEqual(len(pi.rootNode.children), 0)

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
    AssertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

    # Unmute the layer and verify that the composition error is resolved
    # and the reference is restored to the prim index.
    pcp.RequestLayerMuting([], [refLayer.identifier])
    (pi, err) = pcp.ComputePrimIndex('/Root')
    Assert(not err)
    AssertEqual(pi.rootNode.children[0].arcType, Pcp.ArcTypeReference)
    AssertEqual(pi.rootNode.children[0].layerStack.layers[0], refLayer)

    (pi2, err2) = pcp2.ComputePrimIndex('/Root')
    Assert(not err2)
    AssertEqual(pi2.rootNode.children[0].arcType, Pcp.ArcTypeReference)
    AssertEqual(pi2.rootNode.children[0].layerStack.layers[0], refLayer)

TestMutingReferencedLayers()

print "OK"
