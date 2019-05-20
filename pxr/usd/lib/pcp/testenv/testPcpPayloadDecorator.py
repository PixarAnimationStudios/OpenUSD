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

class TestPcpPayloadDecorator(unittest.TestCase):
    def _CreatePcpCache(self, rootLayer):
        return Pcp._TestPayloadDecorator.CreatePcpCacheWithTestDecorator(rootLayer)

    def test_Basic(self):
        '''Test that composing a prim with payloads causes
        the appropriate decorator functions to be called and that the
        arguments generated from those functions are taken into account
        when opening the payload layers.'''

        rootLayerFile = 'basic/root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

        cache = self._CreatePcpCache(rootLayer)
        cache.RequestPayloads(['/Instance'], [])
        (pi, err) = cache.ComputePrimIndex('/Instance')

        assert not err
        assert Sdf.Layer.Find(rootLayerFile)

        payloadLayerFile = 'basic/payload.sdf'
        payloadLayerId1 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile, {'doc':'instance','kind':'ref'})
        payloadLayerId2 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile, {'doc':'updated_instance','kind':'ref'})
        payloadLayerId3 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile, {'doc':'updated_instance','kind':'updated_instance'})
        assert Sdf.Layer.Find(payloadLayerId1), \
            "Failed to find expected payload layer '%s'" % payloadLayerId1
        assert not Sdf.Layer.Find(payloadLayerId2), \
            "Expected to not find payload layer '%s'" % payloadLayerId2
        assert not Sdf.Layer.Find(payloadLayerId3), \
            "Expected to not find payload layer '%s'" % payloadLayerId3

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

        assert Sdf.Layer.Find(payloadLayerId1), \
            "Failed to find expected payload layer '%s'" % payloadLayerId1
        assert Sdf.Layer.Find(payloadLayerId2), \
            "Failed to find expected payload layer '%s'" % payloadLayerId2
        assert not Sdf.Layer.Find(payloadLayerId3), \
            "Expected to not find payload layer '%s'" % payloadLayerId3

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

        assert Sdf.Layer.Find(payloadLayerId1), \
            "Failed to find expected payload layer '%s'" % payloadLayerId1
        assert Sdf.Layer.Find(payloadLayerId2), \
            "Failed to find expected payload layer '%s'" % payloadLayerId2
        assert Sdf.Layer.Find(payloadLayerId3), \
            "Failed to find expected payload layer '%s'" % payloadLayerId3

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

    def test_BasicPayloadList(self):
        '''Test that composing a prim with payloads causes
        the appropriate decorator functions to be called and that the
        arguments generated from those functions are taken into account
        when opening the payload layers when payloads are defined in a list op.'''

        rootLayerFile = 'basic_payload_list/root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

        cache = self._CreatePcpCache(rootLayer)
        cache.RequestPayloads(['/Instance'], [])
        (pi, err) = cache.ComputePrimIndex('/Instance')

        assert not err
        assert Sdf.Layer.Find(rootLayerFile)

        payloadLayerFile1 = 'basic_payload_list/payload1.sdf'
        payloadLayerFile2 = 'basic_payload_list/payload2.sdf'
        payload1LayerId1 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile1, {'doc':'instance','kind':'ref'})
        payload1LayerId2 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile1, {'doc':'updated_instance','kind':'ref'})
        payload2LayerId1 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile2, {'doc':'instance','kind':'ref'})
        payload2LayerId2 = Sdf.Layer.CreateIdentifier(
            payloadLayerFile2, {'doc':'updated_instance','kind':'ref'})
        assert Sdf.Layer.Find(payload1LayerId1), \
            "Failed to find expected payload layer '%s'" % payload1LayerId1
        assert not Sdf.Layer.Find(payload1LayerId2), \
            "Expected to not find payload layer '%s'" % payload1LayerId2
        assert Sdf.Layer.Find(payload2LayerId1), \
            "Failed to find expected payload layer '%s'" % payload2LayerId1
        assert not Sdf.Layer.Find(payload2LayerId2), \
            "Expected to not find payload layer '%s'" % payload2LayerId2

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

        assert Sdf.Layer.Find(payload1LayerId1), \
            "Failed to find expected payload layer '%s'" % payload1LayerId1
        assert Sdf.Layer.Find(payload1LayerId2), \
            "Failed to find expected payload layer '%s'" % payload1LayerId2
        assert Sdf.Layer.Find(payload2LayerId1), \
            "Failed to find expected payload layer '%s'" % payload2LayerId1
        assert Sdf.Layer.Find(payload2LayerId2), \
            "Failed to find expected payload layer '%s'" % payload2LayerId2

    def TestSiblingStrength():
        '''Test that Pcp.PayloadDecorator is invoked and that the
        Pcp.PayloadContext finds the correct strongest values in cases
        where multiple arcs of the same type, both directly and ancestrally,
        are present.'''
        testDir = 'sibling_strength'

        rootLayerFile = os.path.join(testDir, 'root.sdf')
        rootLayer = Sdf.Layer.FindOrOpen(rootLayerFile)

        cache = self._CreatePcpCache(rootLayer)
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

        cache = self._CreatePcpCache(rootLayer)
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

        testLayerFile = 'basic/root.sdf'
        rootLayer = Sdf.Layer.FindOrOpen(testLayerFile)

        cache = self._CreatePcpCache(rootLayer)
        cache.RequestPayloads(['/Instance'], [])
        (pi1, err) = cache.ComputePrimIndex('/Instance')
        assert not err

        # Changing documentation to "foo" is a relevant change.
        with Pcp._TestChangeProcessor(cache) as cp:
            rootLayer.GetPrimAtPath('/Instance').SetInfo('documentation', 'foo')
            assert cp.GetSignificantChanges() == ['/Instance'], \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

        (pi1, err) = cache.ComputePrimIndex('/Instance')
        assert not err

        # The test decorator is implemented to treat fields as case insensitive
        # so a case only change to documentation is not a relevant change.
        with Pcp._TestChangeProcessor(cache) as cp:
            rootLayer.GetPrimAtPath('/Instance').SetInfo('documentation', 'FOO')
            assert cp.GetSignificantChanges() == [], \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

        (pi1, err) = cache.ComputePrimIndex('/Instance')
        assert not err

        # Changing documentation to "BAR" is a relevant change though.
        with Pcp._TestChangeProcessor(cache) as cp:
            rootLayer.GetPrimAtPath('/Instance').SetInfo('documentation', 'BAR')
            assert cp.GetSignificantChanges() == ['/Instance'], \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

        (pi1, err) = cache.ComputePrimIndex('/Instance')
        assert not err

        # But a case only change again will not be relevant.
        with Pcp._TestChangeProcessor(cache) as cp:
            rootLayer.GetPrimAtPath('/Instance').SetInfo('documentation', 'Bar')
            assert cp.GetSignificantChanges() == [], \
                "Got significant changes %s" % cp.GetSignificantChanges()
            assert cp.GetSpecChanges() == [], \
                "Got spec changes %s" % cp.GetSpecChanges()
            assert cp.GetPrimChanges() == [], \
                "Got prim changes %s" % cp.GetPrimChanges()

        (pi1, err) = cache.ComputePrimIndex('/Instance')
        assert not err

if __name__ == "__main__":
    unittest.main()
