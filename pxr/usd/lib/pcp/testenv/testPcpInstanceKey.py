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

from pxr import Pcp, Sdf
import unittest

class TestPcpInstanceKey(unittest.TestCase):
    def _LoadPcpCache(self, layerPath):
        rootLayer = Sdf.Layer.FindOrOpen(layerPath)
        cache = Pcp.Cache(Pcp.LayerStackIdentifier(rootLayer), usd=True)
        return cache

    def _GetInstanceKey(self, cache, primPath):
        (pi, err) = cache.ComputePrimIndex(primPath)
        self.assertEqual(err, [])

        key = Pcp.InstanceKey(pi)
        self.assertEqual(key, key)
        if pi.IsInstanceable():
            self.assertNotEqual(key, Pcp.InstanceKey())
        else:
            self.assertEqual(key, Pcp.InstanceKey())

        print "Pcp.InstanceKey('%s'): " % primPath
        print key, "\n"
        return key

    def test_Default(self):
        """Test default constructed (invalid) instance key for 
        code coverage"""
        invalidKey = Pcp.InstanceKey()
        self.assertEqual(invalidKey, invalidKey)
        print "Pcp.InstanceKey(): "
        print invalidKey

    def test_Basic(self):
        """Test instance key functionality on simple
        asset structure including references and inherits"""
        cache = self._LoadPcpCache('basic.sdf')

        prop1Key = self._GetInstanceKey(cache, '/Set_1/Prop_1')
        prop2Key = self._GetInstanceKey(cache, '/Set_1/Prop_2')
        prop3Key = self._GetInstanceKey(cache, '/Set_1/Prop_3')

        self.assertEqual(prop1Key, prop2Key)
        self.assertNotEqual(prop1Key, prop3Key)
        self.assertNotEqual(prop2Key, prop3Key)

        # Even though /NotAnInstance is tagged with instance = True,
        # it does not introduce any instance-able data via a composition arc
        # and is not treated as a real instance. Thus, it's instance key
        # is empty.
        notAnInstanceKey = self._GetInstanceKey(cache, '/NotAnInstance')
        self.assertEqual(notAnInstanceKey, Pcp.InstanceKey())

    def test_Variants(self):
        """Test instance key functionality on asset
        structure involving references and variants."""
        cache = self._LoadPcpCache('variants.sdf')

        key1 = self._GetInstanceKey(cache, '/Model_1')
        key2 = self._GetInstanceKey(cache, '/Model_2')
        key3 = self._GetInstanceKey(cache, '/Model_3')
        key4 = self._GetInstanceKey(cache, '/Model_4')
        key5 = self._GetInstanceKey(cache, '/Model_5')
        key6 = self._GetInstanceKey(cache, '/Model_6')

        # Model_1, 2, and 3 should all have the same instance key because
        # they share the same reference and the same composed variant selection,
        # even though 1 and 2 have the variant selection authored locally and
        # 3 has the selection authored within the reference.
        self.assertEqual(key1, key2)
        self.assertEqual(key1, key3)

        # Model_4, 5, and 6 have different variant selections and composition
        # arcs, so they should have different keys.
        self.assertNotEqual(key1, key4)
        self.assertNotEqual(key1, key5)
        self.assertNotEqual(key1, key6)

        # Model_5 and 6 have locally-defined variant sets, which can introduce
        # different sets of name children. Because of this, they must also have
        # different instance keys.
        self.assertNotEqual(key5, key6)

    def test_ImpliedArcsWithNoSpecs(self):
        """Test instance key functionality with implied inherits and
        specializes."""
        cache = self._LoadPcpCache('implied_arcs/root.sdf')

        # Both Model prims should share the same instance key even though
        # they are referenced from two different assets. This is because
        # there are no specs for the implied inherits or specializes in the 
        # referencing assets, which means these prims should have the 
        # exact same set of opinions.
        key1 = self._GetInstanceKey(cache, '/Set/SetA/Model')
        key2 = self._GetInstanceKey(cache, '/Set/SetB/Model')
        self.assertEqual(key1, key2)

if __name__ == "__main__":
    unittest.main()
