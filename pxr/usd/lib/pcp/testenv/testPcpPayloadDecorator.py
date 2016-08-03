#!/pxrpythonsubst

from pxr import Sdf, Pcp
from Mentor.Runtime import FindDataFile

import os

class Decorator(Pcp.PayloadDecorator):
    def __init__(self):
        super(Decorator, self).__init__()
    def _GetArgs(self, context):
        class Composer(object):
            def __init__(self):
                super(Composer, self).__init__()
                self.value = None
            def Compose(self, value):
                self.value = value
                return True

        c = Composer()
        context.ComposeValue('documentation', c.Compose)
        doc = c.value if c.value else 'none'
            
        c = Composer()
        context.ComposeValue('kind', c.Compose)
        kind = c.value if c.value else 'none'
        return {'doc':doc, 'kind':kind}

    def _DecoratePayload(self, payload, context):
        return self._GetArgs(context)
    def _IsFieldRelevantForDecoration(self, layer, path, field):
        return (field == "documentation" or
                field == "kind")

def CreatePcpCache(rootLayer, decorator):
    layerStackId = Pcp.LayerStackIdentifier(rootLayer)
    return Pcp.Cache(layerStackId, payloadDecorator=decorator)

def TestBasic():
    '''Test that composing a prim with payloads causes
    the appropriate decorator functions to be called and that the
    arguments generated from those functions are taken into account
    when opening the payload layers.'''

    rootLayerFile = FindDataFile(
        'testPcpPayloadDecorator.testenv/basic/root.sdf')
    rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

    cache = CreatePcpCache(rootLayer, Decorator())
    cache.RequestPayloads(['/Instance'], [])
    (pi, err) = cache.ComputePrimIndex('/Instance')

    assert not err
    assert Sdf.Layer.Find(rootLayerFile)

    payloadLayerFile = FindDataFile(
        'testPcpPayloadDecorator.testenv/basic/payload.sdf')
    payloadLayerId = Sdf.Layer.CreateIdentifier(
        payloadLayerFile, {'doc':'instance','kind':'ref'})
    assert Sdf.Layer.Find(payloadLayerId), \
        "Failed to find expected payload layer '%s'" % payloadLayerId

    # Test that authoring a new value for the relevant fields
    # causes the prim to be significantly changed.
    with Pcp._TestChangeProcessor(cache) as cp:
        rootLayer.GetPrimAtPath('/Instance') \
            .SetInfo('documentation', 'updated_instance')
        assert cp.GetSignificantChanges() == ['/Instance'], \
            "Got significant changes %s" % cp.GetSignificantChanges()
        assert cp.GetSpecChanges() == [], \
            "Got spec changes %s" % cp.GetSpecChanges()
        assert cp.GetPrimChanges() == [], \
            "Got prim changes %s" % cp.GetPrimChanges()
        
    (pi, err) = cache.ComputePrimIndex('/Instance')

    assert Sdf.Layer.Find(payloadLayerId)

    with Pcp._TestChangeProcessor(cache) as cp:
        rootLayer.GetPrimAtPath('/Instance') \
            .SetInfo('kind', 'updated_instance')
        assert cp.GetSignificantChanges() == ['/Instance'], \
            "Got significant changes %s" % cp.GetSignificantChanges()
        assert cp.GetSpecChanges() == [], \
            "Got spec changes %s" % cp.GetSpecChanges()
        assert cp.GetPrimChanges() == [], \
            "Got prim changes %s" % cp.GetPrimChanges()

    (pi, err) = cache.ComputePrimIndex('/Instance')

    payloadLayerId = Sdf.Layer.CreateIdentifier(
        payloadLayerFile, {'doc':'updated_instance','kind':'updated_instance'})
    assert Sdf.Layer.Find(payloadLayerId)

    # Test that authoring a new value for an irrelevant field
    # does not cause any changes.
    with Pcp._TestChangeProcessor(cache) as cp:
        rootLayer.GetPrimAtPath('/Instance') \
            .SetInfo('comment', 'fooooo')
        assert cp.GetSignificantChanges() == [], \
            "Got significant changes %s" % cp.GetSignificantChanges()
        assert cp.GetSpecChanges() == [], \
            "Got spec changes %s" % cp.GetSpecChanges()
        assert cp.GetPrimChanges() == [], \
            "Got prim changes %s" % cp.GetPrimChanges()

def TestSiblingStrength():
    '''Test that Pcp.PayloadDecorator is invoked and that the
    Pcp.PayloadContext finds the correct strongest values in cases
    where multiple arcs of the same type, both directly and ancestrally,
    are present.'''
    testDir = FindDataFile('testPcpPayloadDecorator.testenv/sibling_strength')

    rootLayerFile = os.path.join(testDir, 'root.sdf')
    rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

    cache = CreatePcpCache(rootLayer, Decorator())
    cache.RequestPayloads(['/Instance', '/Instance/Child'], [])
    (pi, err) = cache.ComputePrimIndex('/Instance/Child')
    assert not err

    payloadLayerId = Sdf.Layer.CreateIdentifier(
        os.path.join(testDir, 'payload.sdf'), {'doc':'none','kind':'none'})
    assert Sdf.Layer.Find(payloadLayerId)

    # The payload layers for /Instance/Child should not include the
    # 'documentation' value from payload.sdf because the payload arc to that
    # layer is ancestral and thus weaker than the direct payload.
    childPayloadLayerId = Sdf.Layer.CreateIdentifier(
        os.path.join(testDir, 'child_payload.sdf'), {'doc':'none','kind':'none'})
    assert Sdf.Layer.Find(childPayloadLayerId)

def TestInheritFromPayload():
    '''Test that Pcp.PayloadContext does not pick up values from
    class overrides when the inherit that introduces the class is authored
    inside the payload.'''
    testDir = FindDataFile(
        'testPcpPayloadDecorator.testenv/inherit_from_payload')

    rootLayerFile = os.path.join(testDir, 'root.sdf')
    rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

    cache = CreatePcpCache(rootLayer, Decorator())
    cache.RequestPayloads(['/Instance'], [])
    (pi, err) = cache.ComputePrimIndex('/Instance')
    assert not err

    ref1LayerId = Sdf.Layer.CreateIdentifier(
        os.path.join(testDir, 'payload.sdf'), {'doc':'none','kind':'none'})
    assert Sdf.Layer.Find(ref1LayerId)
    

TestBasic()
TestSiblingStrength()
TestInheritFromPayload()
