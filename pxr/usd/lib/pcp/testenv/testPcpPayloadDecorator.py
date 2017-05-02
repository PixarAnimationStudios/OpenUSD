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

from pxr import Sdf, Pcp
import os, unittest

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

    def _DecoratePayload(self, primIndexPath, payload, context):
        return self._GetArgs(context)
    def _IsFieldRelevantForDecoration(self, field):
        return (field == "documentation" or
                field == "kind")
    def _IsFieldChangeRelevantForDecoration(self, primIndexPath, 
                                            siteLayer, sitePath, 
                                            field, oldVal, newVal):
        assert (field == "documentation" or
                field == "kind")
        return True

class TestPcpPayloadDecorator(unittest.TestCase):
    def _CreatePcpCache(self, rootLayer, decorator):
        layerStackId = Pcp.LayerStackIdentifier(rootLayer)
        return Pcp.Cache(layerStackId, payloadDecorator=decorator)

    def test_Basic(self):
        '''Test that composing a prim with payloads causes
        the appropriate decorator functions to be called and that the
        arguments generated from those functions are taken into account
        when opening the payload layers.'''

        rootLayerFile = 'basic/root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

        cache = self._CreatePcpCache(rootLayer, Decorator())
        cache.RequestPayloads(['/Instance'], [])
        (pi, err) = cache.ComputePrimIndex('/Instance')

        assert not err
        assert Sdf.Layer.Find(rootLayerFile)

        payloadLayerFile = 'basic/payload.sdf'
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
        testDir = 'sibling_strength'

        rootLayerFile = os.path.join(testDir, 'root.sdf')
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

        cache = self._CreatePcpCache(rootLayer, Decorator())
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
        testDir = 'inherit_from_payload'

        rootLayerFile = os.path.join(testDir, 'root.sdf')
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

        cache = self._CreatePcpCache(rootLayer, Decorator())
        cache.RequestPayloads(['/Instance'], [])
        (pi, err) = cache.ComputePrimIndex('/Instance')
        assert not err

        ref1LayerId = Sdf.Layer.CreateIdentifier(
            os.path.join(testDir, 'payload.sdf'), {'doc':'none','kind':'none'})
        assert Sdf.Layer.Find(ref1LayerId)
        
    def test_Changes(self):
        '''Test that the appropriate prim indexes are recomposed based
        on whether Pcp.PayloadDecorator decides a change is relevant to
        payload decoration.'''
        class TestChangeDecorator(Pcp.PayloadDecorator):
            def __init__(self):
                super(TestChangeDecorator, self).__init__()

            def _DecoratePayload(self, primIndexPath, payload, context):
                # This test case doesn't care about the actual decoration.
                return {}
            def _IsFieldRelevantForDecoration(self, field):
                return field == 'documentation'
            def _IsFieldChangeRelevantForDecoration(self, primIndexPath, 
                                                    siteLayer, sitePath, 
                                                    field, oldVal, newVal):
                assert field == 'documentation'
                assert primIndexPath == '/Instance'
                return oldVal == 'instance' and newVal == 'foo'

        testLayerFile = 'basic/root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(testLayerFile)

        cache = self._CreatePcpCache(rootLayer, TestChangeDecorator())
        cache.RequestPayloads(['/Instance'], [])
        (pi1, err) = cache.ComputePrimIndex('/Instance')
        assert not err

        # The test decorator only marks changes to the 'documentation'
        # field from 'instance' to 'foo' as relevant and requiring a
        # significant change.
        with Pcp._TestChangeProcessor(cache) as cp:
            rootLayer.GetPrimAtPath('/Instance').SetInfo('documentation', 'foo')
            assert cp.GetSignificantChanges() == ['/Instance'], \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

        # Other changes aren't relevant, so no changes should be
        # reported.
        with Pcp._TestChangeProcessor(cache) as cp:
            rootLayer.GetPrimAtPath('/Instance').SetInfo('documentation', 'bar')
            assert cp.GetSignificantChanges() == [], \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

if __name__ == "__main__":
    unittest.main()
