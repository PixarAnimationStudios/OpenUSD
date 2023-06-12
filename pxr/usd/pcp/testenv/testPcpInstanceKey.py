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

from __future__ import print_function

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

        print("Pcp.InstanceKey('%s'): " % primPath)
        print(key, "\n")
        return key

    def test_Default(self):
        """Test default constructed (invalid) instance key for 
        code coverage"""
        invalidKey = Pcp.InstanceKey()
        self.assertEqual(invalidKey, invalidKey)
        print("Pcp.InstanceKey(): ")
        print(invalidKey)

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

    def test_Hashing(self):
        cache = self._LoadPcpCache("basic.sdf")

        self.assertEqual(
            hash(self._GetInstanceKey(cache, "/Set_1")),
            hash(self._GetInstanceKey(cache, "/Set_1"))
        )

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

    def test_SubrootReferences(self):
        """Test instance key functionality for subroot reference arcs, mainly
        verifying that we incorporate nodes that are technically considered 
        "ancestral" but must be considered as they are brought in through 
        the subtree that is composed for direct subroot arc."""

        cache = self._LoadPcpCache('subroot_arcs.sdf')

        # For each instance it's useful to know the basic prim index graph
        # ---> = direct reference
        # -a-> = ancestral reference
        # Paths in parentheses have no specs.
        #
        # Model_overrides/Looks is the spec that defines instanceable = true
        # 
        # Instances/Ref_Override_Looks 
        #   ---> Model_overrides/Looks 
        #     -a-> Model_source/Looks
        key = self._GetInstanceKey(cache, '/Instances/Ref_Overrides_Looks')
        # Instances/Ref_A_Looks 
        #   ---> (Model_A/Looks) 
        #     -a-> Model_overrides/Looks 
        #       -a-> Model_source/Looks
        keyA = self._GetInstanceKey(cache, '/Instances/Ref_A_Looks')
        # Instances/Ref_B_Looks 
        #   ---> Model_B/Looks 
        #     -a-> Model_overrides/Looks 
        #       -a-> Model_source/Looks
        keyB = self._GetInstanceKey(cache, '/Instances/Ref_B_Looks')
        # Instances/Ref_C_Looks 
        #   ---> (Model_C/Looks) 
        #     -a-> Model_B/Looks 
        #       -a-> Model_overrides/Looks 
        #         -a-> Model_source/Looks
        keyC = self._GetInstanceKey(cache, '/Instances/Ref_C_Looks')

        # The Ref_Override_Looks and Ref_A_Looks keys are the same because 
        # Model_A/Looks provides no specs. The ancestral reference nodes are
        # still accounted for because they're under a direct reference.
        self.assertNotEqual(key, Pcp.InstanceKey())
        self.assertEqual(key, keyA)

        # The Ref_B_Looks and Ref_C_Looks keys are the same because 
        # Model_C/Looks provides no specs but they differ from the Ref_A_Looks
        # keys because Model_B/Looks does provide specs. The ancestral 
        # reference nodes here are also under a direct reference and are 
        # included.
        self.assertNotEqual(keyB, Pcp.InstanceKey())
        self.assertEqual(keyB, keyC)
        self.assertNotEqual(keyA, keyB)

        # Model_A/Looks
        #   -a-> Model_overrides/Looks 
        #     -a-> Model_source/Looks
        notAnInstanceKey = self._GetInstanceKey(cache, '/Model_A/Looks')
        # Verifying that Model_A/Looks itself is not even instanceable because
        # its ancestral reference to Model_overrides/Looks is indeed ancestral
        # only, so it is ignored for determining instanceable.
        self.assertEqual(notAnInstanceKey, Pcp.InstanceKey())


if __name__ == "__main__":
    unittest.main()
