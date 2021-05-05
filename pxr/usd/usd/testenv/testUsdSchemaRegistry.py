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

    def test_AttributeMetadata(self):
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "MetadataTest")

        self.assertEqual(set(primDef.ListPropertyMetadataFields("testAttr")), 
            set(["allowedTokens", "custom", "default", "displayGroup",
                 "displayName", "documentation", "hidden",
                 "testCustomMetadata", "typeName", "variability"]))
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
            set(["custom", "displayGroup", "displayName", "documentation",
                 "hidden", "testCustomMetadata", "variability"]))
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
        abstractTest = Tf.Type.FindByName("TestUsdSchemaRegistryAbstractTest")
        concreteTest = Tf.Type.FindByName("TestUsdSchemaRegistryMetadataTest")
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        # Test getting a schema type name from a TfType for a concrete typed
        # schema. 
        self.assertEqual(
            Usd.SchemaRegistry.GetSchemaTypeName(concreteTest), 
            "MetadataTest")
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteSchemaTypeName(concreteTest),
            "MetadataTest")
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaTypeName(concreteTest), 
            "")

        # Test the reverse of getting the TfType for concrete typed schema name.
        self.assertEqual(
            Usd.SchemaRegistry.GetTypeFromSchemaTypeName("MetadataTest"),
            concreteTest)
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName("MetadataTest"),
            concreteTest)
        self.assertEqual(
            Usd.SchemaRegistry.GetAPITypeFromSchemaTypeName("MetadataTest"), 
            Tf.Type.Unknown)

        # Test getting a schema type name from a TfType for an abstract typed
        # schema. 
        self.assertEqual(
            Usd.SchemaRegistry.GetSchemaTypeName(abstractTest), 
            "AbstractTest")
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteSchemaTypeName(abstractTest),
            "")
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaTypeName(abstractTest), 
            "")

        # Test the reverse of getting the TfType for abastract typed schema name.
        self.assertEqual(
            Usd.SchemaRegistry.GetTypeFromSchemaTypeName("AbstractTest"),
            abstractTest)
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName("AbstractTest"),
            Tf.Type.Unknown)
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

    def test_GetSchemaKind(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertEqual(Usd.SchemaRegistry.GetSchemaKind(modelAPI),
                         Usd.SchemaKind.NonAppliedAPI)
        self.assertEqual(Usd.SchemaRegistry.GetSchemaKind(clipsAPI),
                         Usd.SchemaKind.NonAppliedAPI)
        self.assertEqual(Usd.SchemaRegistry.GetSchemaKind(collectionAPI),
                         Usd.SchemaKind.MultipleApplyAPI)

        self.assertEqual(Usd.SchemaRegistry.GetSchemaKind("Bogus"),
                         Usd.SchemaKind.Invalid)

    def test_IsConcrete(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry.IsConcrete(modelAPI))
        self.assertFalse(Usd.SchemaRegistry.IsConcrete(clipsAPI))
        self.assertFalse(Usd.SchemaRegistry.IsConcrete(collectionAPI))

    def test_IsAppliedAPISchema(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry.IsAppliedAPISchema(modelAPI))
        self.assertFalse(Usd.SchemaRegistry.IsAppliedAPISchema(clipsAPI))
        self.assertTrue(Usd.SchemaRegistry.IsAppliedAPISchema(collectionAPI))

    def test_IsMultipleApplyAPISchema(self):
        modelAPI = Tf.Type.FindByName("UsdModelAPI")
        clipsAPI = Tf.Type.FindByName("UsdClipsAPI")
        collectionAPI = Tf.Type.FindByName("UsdCollectionAPI")

        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyAPISchema(modelAPI))
        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyAPISchema(clipsAPI))
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyAPISchema(
                            collectionAPI))

    def test_GetTypeAndInstance(self):
        # test multiplyapply api schema token
        typeAndInstance = Usd.SchemaRegistry.GetTypeAndInstance(
                "CollectionAPI:lightlink")
        self.assertEqual(typeAndInstance[0], 'CollectionAPI')
        self.assertEqual(typeAndInstance[1], 'lightlink')

        # test singleapply api schema token
        typeAndInstance = Usd.SchemaRegistry.GetTypeAndInstance(
                "SingleApplyAPI")
        self.assertEqual(typeAndInstance[0], "SingleApplyAPI")
        self.assertEqual(typeAndInstance[1], "")

    def test_GetPropertyNamespacePrefix(self):
        # CollectionAPI is a multiple apply API so it must have a property
        # namespace prefix.
        self.assertEqual(
            Usd.SchemaRegistry().GetPropertyNamespacePrefix("CollectionAPI"),
            "collection")
        # Other schema types that are not multiple apply API will not have a 
        # property namespace prefix.
        self.assertEqual(
            Usd.SchemaRegistry().GetPropertyNamespacePrefix("MetadataTest"), "")
        self.assertEqual(
            Usd.SchemaRegistry().GetPropertyNamespacePrefix("ClipsAPI"), "")

    def test_FlattenPrimDefinition(self):
        # Verifies that the given spec has exactly the field values enumerated
        # in expectedFieldValues
        def _VerifyExpectedFieldValues(spec, expectedFieldValues):
            for (expectedField, expectedValue) in expectedFieldValues.items():
                self.assertEqual(spec.GetInfo(expectedField), expectedValue)
            for field in spec.ListInfoKeys():
                self.assertIn(field, expectedFieldValues)

        # Verifies that there's a prim spec at path in the layer with the
        # given set of prim spec fields and the expected properties (with their
        # own expected fields.
        def _VerifyExpectedPrimData(layer, path, expectedPrimFields,
                                    expectedProperties):
            # Verify the prim spec and its expected fields.
            spec = layer.GetObjectAtPath(path)
            self.assertTrue(spec)
            _VerifyExpectedFieldValues(spec, expectedPrimFields)

            # Verify the prim spec has exactly the expected properties and 
            # each property has the expected fields for each expected property.
            self.assertEqual(len(spec.properties), len(expectedProperties))
            for (propName, fieldValues) in expectedProperties.items():
                propSpec = spec.properties[propName]
                self.assertTrue(propSpec)
                _VerifyExpectedFieldValues(propSpec, fieldValues)

        # Create a new layer with a sublayer for authoring.
        layer = Sdf.Layer.CreateAnonymous(".usda")
        sublayer = Sdf.Layer.CreateAnonymous(".usda")
        layer.subLayerPaths.append(sublayer.identifier)

        # Test flattening a prim definition for a concrete schema directly 
        # to a new path in the layer with the def specifier. A new prim spec 
        # will be created with given specifier (as well as an over for the 
        # not yet existent parent path).
        concretePrimDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "MetadataTest")
        self.assertTrue(concretePrimDef.FlattenTo(
            layer, "/PrimDefs/ConcreteSchema", Sdf.SpecifierDef))

        # Expected fields and properties from the concrete schema.
        concretePrimDefPrimFields = {
            "apiSchemas" : Sdf.TokenListOp.CreateExplicit([]),
            "documentation" : concretePrimDef.GetDocumentation(),
            "hidden" : True,
            "testCustomMetadata" : "garply",
            "specifier" : Sdf.SpecifierDef,
            "typeName" : "MetadataTest"
        }
        concretePrimDefProperties = {
            "testAttr" : {
                "custom" : False,
                "default" : "foo",
                "typeName" : Sdf.ValueTypeNames.String,
                "allowedTokens" : ["bar", "baz"],
                "displayGroup" : "Display Group",
                "displayName" : "Display Name",
                "documentation" : "Testing documentation metadata",
                "hidden" : True,
                "testCustomMetadata" : "garply",
                "variability" : Sdf.VariabilityVarying
            },
            "testRel" : {
                "custom" : False,
                "displayGroup" : "Display Group",
                "displayName" : "Display Name",
                "documentation" : "Testing documentation metadata",
                "hidden" : True,
                "testCustomMetadata" : "garply",
                "variability" : Sdf.VariabilityUniform
            }
        }

        # Verify the new spec exists and has the correct prim fields and 
        # property specs.
        _VerifyExpectedPrimData(layer, "/PrimDefs/ConcreteSchema",
                                concretePrimDefPrimFields,
                                concretePrimDefProperties)

        # Test flattening a prim definition for an API schema directly 
        # to a new path in the layer with the fallback over specifier. A new 
        # overprim spec will be created.
        apiPrimDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "CollectionAPI")
        self.assertTrue(apiPrimDef.FlattenTo(layer, "/PrimDefs/APISchema"))

        # Expected fields and properties from the CollectionAPI schema. Note
        # that CollectionAPI is a multiple apply schema but the expected 
        # properties have no prefix. This is because the API prim definition is
        # for the CollectionAPI itself, not an applied instance of the API 
        # schema.
        apiPrimDefPrimFields = {
            "apiSchemas" : Sdf.TokenListOp.CreateExplicit([]),
            "documentation" : apiPrimDef.GetDocumentation(),
            "specifier" : Sdf.SpecifierOver
        }
        apiPrimDefProperties = {
            "expansionRule" : {
                "custom" : False,
                "default" : "expandPrims",
                "typeName" : Sdf.ValueTypeNames.Token,
                "allowedTokens" : ["explicitOnly", "expandPrims", "expandPrimsAndProperties"],
                "documentation" : apiPrimDef.GetPropertyDocumentation("expansionRule"),
                "variability" : Sdf.VariabilityUniform
            },
            "includeRoot" : {
                "custom" : False,
                "default" : None,
                "typeName" : Sdf.ValueTypeNames.Bool,
                "documentation" : apiPrimDef.GetPropertyDocumentation("includeRoot"),
                "variability" : Sdf.VariabilityUniform
            },
            "includes" : {
                "custom" : False,
                "documentation" : apiPrimDef.GetPropertyDocumentation("includes"),
                "variability" : Sdf.VariabilityUniform
            },
            "excludes" : {
                "custom" : False,
                "documentation" : apiPrimDef.GetPropertyDocumentation("excludes"),
                "variability" : Sdf.VariabilityUniform
            }
        }

        # Verify the new spec exists and has the correct prim fields and 
        # property specs.
        _VerifyExpectedPrimData(layer, "/PrimDefs/APISchema", 
                                apiPrimDefPrimFields,
                                apiPrimDefProperties)

        # Create stage from our to test the overloads of Flatten that take 
        # a UsdPrim parent and a child name.
        stage = Usd.Stage.Open(layer)
        parentPrim = stage.GetPrimAtPath("/PrimDefs")
        self.assertTrue(parentPrim)

        # Flatten the concrete prim def to a new prim under the parent using
        # the current edit target (root layer).
        flattenPrim = concretePrimDef.FlattenTo(parentPrim, "FlattenToNewPrim")
        self.assertTrue(flattenPrim)

        # Verify the new spec exists on the root layer and has the correct prim
        # fields and property specs. Note that FlattenTo was called with the
        # default over specifier.
        concretePrimDefPrimFields["specifier"] = Sdf.SpecifierOver
        _VerifyExpectedPrimData(layer, "/PrimDefs/FlattenToNewPrim",
                                concretePrimDefPrimFields,
                                concretePrimDefProperties)

        # Flatten the API prim def to the prim we just created using the 
        # sublayer edit target.
        with Usd.EditContext(stage, sublayer):
            flattenPrim = apiPrimDef.FlattenTo(flattenPrim)
            self.assertTrue(flattenPrim)

        # Verify the new spec exists on the sublayer with the API schema fields
        # and properties while the root layer still has the flattened concrete
        # schema spec.
        _VerifyExpectedPrimData(layer, "/PrimDefs/FlattenToNewPrim",
                                concretePrimDefPrimFields,
                                concretePrimDefProperties)
        _VerifyExpectedPrimData(sublayer, "/PrimDefs/FlattenToNewPrim",
                                apiPrimDefPrimFields,
                                apiPrimDefProperties)

        # Flatten the API prim def again to the same prim but now with the 
        # root layer edit target.
        flattenPrim = apiPrimDef.FlattenTo(flattenPrim)
        self.assertTrue(flattenPrim)

        # Verify that the root layer specs fields and properties have been
        # fully replaced to match the API schema prim definition.
        _VerifyExpectedPrimData(layer, "/PrimDefs/FlattenToNewPrim",
                                apiPrimDefPrimFields,
                                apiPrimDefProperties)

        # Build the composed prim definition that would be created for a prim
        # of the type "MetadataTest" with a CollectionAPI named "foo" applied.
        newPrimDef = Usd.SchemaRegistry().BuildComposedPrimDefinition(
            "MetadataTest", ["CollectionAPI:foo"])

        # Flatten the composed prim definition to the already existing root 
        # layer spec for the parent of all the other prim's we created.
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs"))
        self.assertTrue(newPrimDef.FlattenTo(
            layer, "/PrimDefs", Sdf.SpecifierDef))

        # The prim fields for the composed prim definition will be the same
        # as the concrete prim definition as prim fields don't come from 
        # applied API schemas.
        newPrimDefPrimFields = concretePrimDefPrimFields
        # The apiSchemas metadata will always be set to explicit list of applied
        # API schemas.
        newPrimDefPrimFields["apiSchemas"] = \
            Sdf.TokenListOp.CreateExplicit(["CollectionAPI:foo"])
        # The specifier will still be "over" even though we suggested specifier
        # "def" because the prim spec already existed. The suggested specifier
        # only applies to newly created specs.
        newPrimDefPrimFields["specifier"] = Sdf.SpecifierOver

        # The expected properties are the combined set of properties from the 
        # concrete schema and the applied schemas.
        newPrimDefProperties = {}
        for (propName, fieldValues) in concretePrimDefProperties.items():
            newPrimDefProperties[propName] = fieldValues
        # In this case all the properties from the applied multiple apply 
        # will have the "collection:foo:" prefix as the schema is applied in
        # this prim definition.
        for (propName, fieldValues) in apiPrimDefProperties.items():
            newPrimDefProperties["collection:foo:" + propName] = fieldValues

        # Verify the prim spec matches the prim definition.
        _VerifyExpectedPrimData(layer, "/PrimDefs",
                                newPrimDefPrimFields,
                                newPrimDefProperties)

        # Verify that the existing children of the prim spec as FlattenTo
        # doesn't clear or overwrite fields that can never be schema metadata.
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs/ConcreteSchema"))
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs/APISchema"))
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs/FlattenToNewPrim"))

if __name__ == "__main__":
    unittest.main()
