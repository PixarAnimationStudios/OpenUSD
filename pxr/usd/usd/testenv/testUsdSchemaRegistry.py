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
from pxr import Plug, Usd, Sdf, Vt, Tf

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
            "MetadataTest")
        self.assertTrue(primDef)

        self.assertEqual(set(primDef.ListMetadataFields()), 
            set(["typeName", "testCustomMetadata", "hidden", "documentation"]))
        self.assertEqual(primDef.GetMetadata("typeName"), "MetadataTest")
        self.assertEqual(primDef.GetMetadata("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primDef.GetMetadata("hidden"), True)
        self.assertEqual(primDef.GetMetadata("testCustomMetadata"), "garply")

        self.assertEqual(primDef.GetDocumentation(),
                         "Testing documentation metadata")

        primSpec = primDef.GetSchemaPrimSpec()
        self.assertEqual(primSpec.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primSpec.GetInfo("hidden"), True)
        self.assertEqual(primSpec.GetInfo("testCustomMetadata"), "garply")

    def test_AttributeMetadata(self):
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "MetadataTest")

        self.assertEqual(set(primDef.ListPropertyMetadataFields("testAttr")), 
            set(["allowedTokens", "default", "displayGroup", "displayName", 
                 "documentation", "hidden", "testCustomMetadata", "typeName"]))
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "typeName"),
                         "string")
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "allowedTokens"),
                         Vt.TokenArray(["bar", "baz"]))
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "displayGroup"), 
                         "Display Group")
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "displayName"), 
                         "Display Name")
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "hidden"), 
                         True)
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "testCustomMetadata"), 
                         "garply")
        self.assertEqual(primDef.GetPropertyMetadata("testAttr", "default"), 
                         "foo")

        self.assertEqual(primDef.GetAttributeFallbackValue("testAttr"), "foo")
        self.assertEqual(primDef.GetPropertyDocumentation("testAttr"),
                         "Testing documentation metadata")

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
            "MetadataTest")

        self.assertEqual(set(primDef.ListPropertyMetadataFields("testRel")), 
            set(["displayGroup", "displayName", "documentation", "hidden", 
                 "testCustomMetadata", "variability"]))
        self.assertEqual(primDef.GetPropertyMetadata("testRel", "displayGroup"), 
                         "Display Group")
        self.assertEqual(primDef.GetPropertyMetadata("testRel", "displayName"), 
                         "Display Name")
        self.assertEqual(primDef.GetPropertyMetadata("testRel", "documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primDef.GetPropertyMetadata("testRel", "hidden"), 
                         True)
        self.assertEqual(primDef.GetPropertyMetadata("testRel", "testCustomMetadata"), 
                         "garply")
        self.assertEqual(primDef.GetPropertyMetadata("testRel", "variability"), 
                         Sdf.VariabilityUniform)

        self.assertIsNone(primDef.GetAttributeFallbackValue("testRel"))
        self.assertEqual(primDef.GetPropertyDocumentation("testRel"),
                         "Testing documentation metadata")

        relDef = primDef.GetSchemaRelationshipSpec("testRel")
        self.assertTrue(relDef)

        self.assertEqual(relDef.GetInfo("displayGroup"), "Display Group")
        self.assertEqual(relDef.GetInfo("displayName"), "Display Name")
        self.assertEqual(relDef.GetInfo("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(relDef.GetInfo("hidden"), True)
        self.assertEqual(relDef.GetInfo("testCustomMetadata"), "garply")

    def test_GetUsdSchemaTypeName(self):
        testType = Tf.Type.FindByName("TestUsdSchemaRegistryMetadataTest")
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        # Test getting a schema type name from a TfType for a concrete typed
        # schema. 
        self.assertEqual(
            Usd.SchemaRegistry.GetSchemaTypeName(testType), 
            "MetadataTest")
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteSchemaTypeName(testType),
            "MetadataTest")
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaTypeName(testType), 
            "")

        # Test the reverse of getting the TfType for concrete typed schema name.
        self.assertEqual(
            Usd.SchemaRegistry.GetTypeFromSchemaTypeName("MetadataTest"),
            testType)
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName("MetadataTest"),
            testType)
        self.assertEqual(
            Usd.SchemaRegistry.GetAPITypeFromSchemaTypeName("MetadataTest"), 
            Tf.Type.Unknown)

        # Test getting a schema type name from a TfType for an applied API
        # schema.
        self.assertEqual(
            Usd.SchemaRegistry.GetSchemaTypeName(collectionAPI),
            "CollectionAPI")
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteSchemaTypeName(collectionAPI),
            "")
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaTypeName(collectionAPI), 
            "CollectionAPI")

        # Test the reverse of getting the TfType for an applied API schema name.
        self.assertEqual(
            Usd.SchemaRegistry.GetTypeFromSchemaTypeName("CollectionAPI"),
            collectionAPI)
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName("CollectionAPI"),
            Tf.Type.Unknown)
        self.assertEqual(
            Usd.SchemaRegistry.GetAPITypeFromSchemaTypeName("CollectionAPI"), 
            collectionAPI)

        # Test getting a schema type name from a TfType for a non-apply API
        # schema. This is the same API as for applied API schemas but may change
        # in the future?
        self.assertEqual(
            Usd.SchemaRegistry.GetSchemaTypeName(modelAPI),
            "ModelAPI")
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteSchemaTypeName(modelAPI),
            "")
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaTypeName(modelAPI), 
            "ModelAPI")

        # Test the reverse of getting the TfType for a non-apply API schema name
        self.assertEqual(
            Usd.SchemaRegistry.GetTypeFromSchemaTypeName("ModelAPI"),
            modelAPI)
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName("ModelAPI"),
            Tf.Type.Unknown)
        self.assertEqual(
            Usd.SchemaRegistry.GetAPITypeFromSchemaTypeName("ModelAPI"), 
            modelAPI)

        # A valid type without an associated schema prim definition returns an
        # empty type name.
        self.assertTrue(Tf.Type(Usd.Typed))        
        self.assertEqual(
            Usd.SchemaRegistry().GetSchemaTypeName(Tf.Type(Usd.Typed)), "")

    def test_FindConcretePrimDefinition(self):
        # MetadataTest is a concrete prim schama. Can get the prim definition
        # through FindConcretePrimDefinition.
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
        # Applied API schema. Not returned from FindConcretePrimDefinition
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
