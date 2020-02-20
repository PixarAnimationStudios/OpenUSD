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
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestUsdSchemaRegistryMetadataTest")
        self.assertTrue(primDef)

        primSpec = primDef.GetSchemaPrimSpec()
        self.assertEqual(primSpec.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primSpec.GetInfo("hidden"), True)
        self.assertEqual(primSpec.GetInfo("testCustomMetadata"), "garply")

    def test_AttributeMetadata(self):
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestUsdSchemaRegistryMetadataTest")
        attrDef = primDef.GetSchemaAttributeSpec("testAttr")
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
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestUsdSchemaRegistryMetadataTest")
        relDef = primDef.GetSchemaRelationshipSpec("testRel")
        self.assertTrue(relDef)

        self.assertEqual(relDef.GetInfo("displayGroup"), "Display Group")
        self.assertEqual(relDef.GetInfo("displayName"), "Display Name")
        self.assertEqual(relDef.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(relDef.GetInfo("hidden"), True)
        self.assertEqual(relDef.GetInfo("testCustomMetadata"), "garply")

    def test_GetUsdSchemaTypeName(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertEqual(Usd.SchemaRegistry().GetSchemaTypeName(modelAPI),
                         "ModelAPI")
        self.assertEqual(Usd.SchemaRegistry().GetSchemaTypeName(clipsAPI),
                         "ClipsAPI")
        self.assertEqual(Usd.SchemaRegistry().GetSchemaTypeName(collectionAPI),
                         "CollectionAPI")
        
        # A valid type without an associated schema prim definition returns an
        # empty type name.
        self.assertTrue(Tf.Type(Usd.Typed))        
        self.assertEqual(
            Usd.SchemaRegistry().GetSchemaTypeName(Tf.Type(Usd.Typed)), "")

    def test_FindConcretePrimDefinition(self):
        # CollectionAPI is an applied API schama. Can get the prim definition
        # through FindAppliedAPIPrimDefintion.
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            'MetadataTest')
        self.assertTrue(primDef)
        # Prim def has schema spec with USD type name.
        self.assertEqual(primDef.GetSchemaPrimSpec().name, 'MetadataTest')

        # Prim def has built in property names.
        self.assertEqual(primDef.GetPropertyNames(), ['testAttr', 'testRel'])

        # Prim def has relationship/property spec for 'testRel'
        self.assertTrue(primDef.GetSchemaPropertySpec('testRel'))
        self.assertFalse(primDef.GetSchemaAttributeSpec('testRel'))
        self.assertTrue(primDef.GetSchemaRelationshipSpec('testRel'))

        # Prim def has attribute/property spec for 'testAttr'.
        self.assertTrue(primDef.GetSchemaPropertySpec('testAttr'))
        self.assertTrue(primDef.GetSchemaAttributeSpec('testAttr'))
        self.assertFalse(primDef.GetSchemaRelationshipSpec('testAttr'))

        # Non-apply API schema. No prim definition
        self.assertFalse(Usd.SchemaRegistry().FindConcretePrimDefinition(
            'ModelAPI'))
        # Applied API schema. Not returned from FindConcreteTyped...
        self.assertFalse(Usd.SchemaRegistry().FindConcretePrimDefinition(
            'CollectionAPI'))


    def test_FindAppliedAPIPrimDefinition(self):
        # CollectionAPI is an applied API schama. Can get the prim definition
        # through FindAppliedAPIPrimDefintion.
        primDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            'CollectionAPI')
        self.assertTrue(primDef)
        # Prim def has schema spec with USD type name.
        self.assertEqual(primDef.GetSchemaPrimSpec().name, 'CollectionAPI')

        # Prim def has built in property names.
        self.assertEqual(primDef.GetPropertyNames(), 
            ['expansionRule', 'includeRoot', 'excludes', 'includes'])

        # Prim def has relationship/property spec for 'excludes'
        self.assertTrue(primDef.GetSchemaPropertySpec('excludes'))
        self.assertFalse(primDef.GetSchemaAttributeSpec('excludes'))
        self.assertTrue(primDef.GetSchemaRelationshipSpec('excludes'))

        # Prim def has attribute/property spec for 'expansionRule'.
        self.assertTrue(primDef.GetSchemaPropertySpec('expansionRule'))
        self.assertTrue(primDef.GetSchemaAttributeSpec('expansionRule'))
        self.assertFalse(primDef.GetSchemaRelationshipSpec('expansionRule'))

        # API schema but not an applied schema. No prim definition
        self.assertFalse(Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            'ModelAPI'))
        # Concrete typed schema. Not returned from FindAppliedAPI...
        self.assertFalse(Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            'MetadataTest'))

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

        self.assertFalse(Usd.SchemaRegistry().IsConcrete(modelAPI))
        self.assertFalse(Usd.SchemaRegistry().IsConcrete(clipsAPI))
        self.assertFalse(Usd.SchemaRegistry().IsConcrete(collectionAPI))

    def test_IsAppliedAPISchema(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry().IsAppliedAPISchema(modelAPI))
        self.assertFalse(Usd.SchemaRegistry().IsAppliedAPISchema(clipsAPI))
        self.assertTrue(Usd.SchemaRegistry().IsAppliedAPISchema(
                            collectionAPI))

    def test_IsMultipleApplyAPISchema(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry().IsMultipleApplyAPISchema(modelAPI))
        self.assertFalse(Usd.SchemaRegistry().IsMultipleApplyAPISchema(clipsAPI))
        self.assertTrue(Usd.SchemaRegistry().IsMultipleApplyAPISchema(
                            collectionAPI))

if __name__ == "__main__":
    unittest.main()
