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

import os, unittest
from pxr import Plug, Usd, Vt, Tf

class TestUsdSchemaRegistry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdSchemaRegistry", \
            "Failed to load expected test plugin"
    
    def test_PrimMetadata(self):
        primDef = Usd.SchemaRegistry.GetPrimDefinition(
            "TestUsdSchemaRegistryMetadataTest")
        self.assertTrue(primDef)

        self.assertEqual(primDef.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primDef.GetInfo("hidden"), True)
        self.assertEqual(primDef.GetInfo("testCustomMetadata"), "garply")

    def test_AttributeMetadata(self):
        attrDef = Usd.SchemaRegistry.GetAttributeDefinition(
            "TestUsdSchemaRegistryMetadataTest", "testAttr")
        self.assertTrue(attrDef)

        self.assertEqual(attrDef.GetInfo("allowedTokens"),
                         Vt.TokenArray(["bar", "baz"]))
        self.assertEqual(attrDef.GetInfo("displayGroup"), "Display Group")
        self.assertEqual(attrDef.GetInfo("displayName"), "Display Name")
        self.assertEqual(attrDef.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(attrDef.GetInfo("hidden"), True)
        self.assertEqual(attrDef.GetInfo("testCustomMetadata"), "garply")

    def test_RelationshipMetadata(self):
        relDef = Usd.SchemaRegistry.GetRelationshipDefinition(
            "TestUsdSchemaRegistryMetadataTest", "testRel")
        self.assertTrue(relDef)

        self.assertEqual(relDef.GetInfo("displayGroup"), "Display Group")
        self.assertEqual(relDef.GetInfo("displayName"), "Display Name")
        self.assertEqual(relDef.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(relDef.GetInfo("hidden"), True)
        self.assertEqual(relDef.GetInfo("testCustomMetadata"), "garply")

    def test_IsTyped(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry.IsTyped(modelAPI))
        self.assertFalse(Usd.SchemaRegistry.IsTyped(clipsAPI))
        self.assertFalse(Usd.SchemaRegistry.IsTyped(collectionAPI))

    def test_IsConcrete(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry.IsConcrete(modelAPI))
        self.assertFalse(Usd.SchemaRegistry.IsConcrete(clipsAPI))
        self.assertFalse(Usd.SchemaRegistry.IsConcrete(collectionAPI))

    def test_IsMultipleApplyAPISchema(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyAPISchema(modelAPI))
        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyAPISchema(clipsAPI))
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyAPISchema(
                            collectionAPI))

if __name__ == "__main__":
    unittest.main()
