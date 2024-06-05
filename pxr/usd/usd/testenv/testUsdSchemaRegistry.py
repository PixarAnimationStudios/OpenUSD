#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
            set(["typeName", "testCustomMetadata", "testDictionaryMetadata",
                 "hidden", "documentation"]))
        self.assertEqual(primDef.GetMetadata("typeName"), "MetadataTest")
        self.assertEqual(primDef.GetMetadata("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(primDef.GetMetadata("hidden"), True)
        self.assertEqual(primDef.GetMetadata("testCustomMetadata"), "garply")

        # Dictionary metadata can be gotten by whole value as well as queried
        # for individual keys in the metadata value.
        self.assertEqual(primDef.GetMetadata("testDictionaryMetadata"), 
            {"name" : "foo", "value" : 2})
        self.assertEqual(primDef.GetMetadataByDictKey(
            "testDictionaryMetadata", "name"), "foo")
        self.assertEqual(primDef.GetMetadataByDictKey(
            "testDictionaryMetadata", "value"), 2)

        self.assertEqual(primDef.GetDocumentation(),
                         "Testing brief user doc for schema class")

    def test_AttributeMetadata(self):
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "MetadataTest")
        self.assertTrue(primDef)

        # All data about an attribute can be accessed through both property API
        # on the prim definition and API on the returned attribute defintion
        # accessor. We test that both match up here.
        attrDef = primDef.GetAttributeDefinition("testAttr")
        self.assertTrue(attrDef)
        self.assertEqual(attrDef.GetName(), "testAttr")
        self.assertTrue(attrDef.IsAttribute())
        self.assertFalse(attrDef.IsRelationship())

        # Get spec type
        self.assertEqual(attrDef.GetSpecType(), Sdf.SpecTypeAttribute)
        self.assertEqual(primDef.GetSpecType("testAttr"), Sdf.SpecTypeAttribute)

        # List metadata fields
        self.assertEqual(set(attrDef.ListMetadataFields()), 
            set(["allowedTokens", "custom", "default", "displayGroup",
                 "displayName", "documentation", "hidden", "testCustomMetadata",
                 "testDictionaryMetadata", "typeName", "variability"]))
        self.assertEqual(primDef.ListPropertyMetadataFields("testAttr"), 
                         attrDef.ListMetadataFields())

        # Verify that both the attrribute and prim defs return the same value
        # for every metadata field on the attribute
        for field in attrDef.ListMetadataFields():
            self.assertEqual(attrDef.GetMetadata(field),
                             primDef.GetPropertyMetadata("testAttr", field))

        # Type name has special functions on the attribute def.
        self.assertEqual(attrDef.GetMetadata("typeName"), "string")
        self.assertEqual(attrDef.GetTypeName(), Sdf.ValueTypeNames.String)
        self.assertEqual(attrDef.GetTypeNameToken(), "string")

        # Variability has a special function on the attr def.
        self.assertEqual(attrDef.GetMetadata("variability"), 
                        Sdf.VariabilityVarying)
        self.assertEqual(attrDef.GetVariability(), Sdf.VariabilityVarying)

        # Fallback value has special functions on both the attribute and prim
        # defs.
        self.assertEqual(attrDef.GetMetadata("default"), "foo")
        self.assertEqual(attrDef.GetFallbackValue(), "foo")
        self.assertEqual(primDef.GetAttributeFallbackValue("testAttr"), "foo")

        # Documentation has special functions on both the attribute and prim
        # defs.
        self.assertEqual(attrDef.GetMetadata("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(attrDef.GetDocumentation(),
                         "Testing brief user doc for schema attr")
        self.assertEqual(primDef.GetPropertyDocumentation("testAttr"),
                         "Testing brief user doc for schema attr")

        # Dictionary metadata can be gotten by whole value as well as queried
        # for individual keys in the metadata value.
        self.assertEqual(attrDef.GetMetadata("testDictionaryMetadata"), 
            {"name" : "bar", "value" : 3})
        self.assertEqual(attrDef.GetMetadataByDictKey(
            "testDictionaryMetadata", "name"), "bar")
        self.assertEqual(attrDef.GetMetadataByDictKey(
            "testDictionaryMetadata", "value"), 3)
        # Can get the same dictionary value by key from the prim def via 
        # GetPropertyMetadataByDictKey
        self.assertEqual(primDef.GetPropertyMetadataByDictKey(
            "testAttr", "testDictionaryMetadata", "name"), "bar")
        self.assertEqual(primDef.GetPropertyMetadataByDictKey(
            "testAttr", "testDictionaryMetadata", "value"), 3)

        # The rest of the metadata without special accessor functions.
        self.assertEqual(attrDef.GetMetadata("allowedTokens"),
                         Vt.TokenArray(["bar", "baz"]))
        self.assertEqual(attrDef.GetMetadata("displayGroup"), "Display Group")
        self.assertEqual(attrDef.GetMetadata("displayName"), "Display Name")
        self.assertEqual(attrDef.GetMetadata("hidden"), True)
        self.assertEqual(attrDef.GetMetadata("testCustomMetadata"), "garply")

        # Verify that we get the attribute as a property and it returns all the
        # same metadata values.
        propDef = primDef.GetPropertyDefinition("testAttr")
        self.assertTrue(propDef)
        self.assertEqual(propDef.GetName(), "testAttr")
        self.assertTrue(propDef.IsAttribute())
        self.assertFalse(propDef.IsRelationship())
        for field in attrDef.ListMetadataFields():
            self.assertEqual(attrDef.GetMetadata(field),
                             propDef.GetMetadata(field))

    def test_RelationshipMetadata(self):
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "MetadataTest")
        self.assertTrue(primDef)

        # All data about an attribute can be accessed through both property API
        # on the prim definition and API on the returned attribute defintion
        # accessor. We test that both match up here.
        relDef = primDef.GetRelationshipDefinition("testRel")
        self.assertTrue(relDef)
        self.assertFalse(relDef.IsAttribute())
        self.assertTrue(relDef.IsRelationship())

        # Get spec type
        self.assertEqual(relDef.GetSpecType(), Sdf.SpecTypeRelationship)
        self.assertEqual(primDef.GetSpecType("testRel"), Sdf.SpecTypeRelationship)

        # List metadata fields
        self.assertEqual(set(relDef.ListMetadataFields()), 
            set(["custom", "displayGroup", "displayName", "documentation",
                 "hidden", "testCustomMetadata", "testDictionaryMetadata",
                 "variability"]))
        self.assertEqual(primDef.ListPropertyMetadataFields("testRel"), 
                         relDef.ListMetadataFields())

        # Verify that both the attrribute and prim defs return the same value
        # for every metadata field on the attribute
        for field in relDef.ListMetadataFields():
            self.assertEqual(relDef.GetMetadata(field),
                             primDef.GetPropertyMetadata("testRel", field))

        # Variability has a special function on the rel def.
        self.assertEqual(relDef.GetMetadata("variability"), 
                         Sdf.VariabilityUniform)
        self.assertEqual(relDef.GetVariability(), Sdf.VariabilityUniform)

        # Documentation has special functions on both the attribute and prim
        # defs.
        self.assertEqual(relDef.GetMetadata("documentation"),
                         "Testing documentation metadata")
        self.assertEqual(relDef.GetDocumentation(),
                         "Testing brief user doc for schema rel")
        self.assertEqual(primDef.GetPropertyDocumentation("testRel"),
                         "Testing brief user doc for schema rel")

        # Dictionary metadata can be gotten by whole value as well as queried
        # for individual keys in the metadata value.
        self.assertEqual(relDef.GetMetadata("testDictionaryMetadata"), 
            {"name" : "baz", "value" : 5})
        self.assertEqual(relDef.GetMetadataByDictKey(
            "testDictionaryMetadata", "name"), "baz")
        self.assertEqual(relDef.GetMetadataByDictKey(
            "testDictionaryMetadata", "value"), 5)
        # Can get the same dictionary value by key from the prim def via 
        # GetPropertyMetadataByDictKey
        self.assertEqual(primDef.GetPropertyMetadataByDictKey(
            "testRel", "testDictionaryMetadata", "name"), "baz")
        self.assertEqual(primDef.GetPropertyMetadataByDictKey(
            "testRel", "testDictionaryMetadata", "value"), 5)

        # The rest of the metadata without special accessor functions.
        self.assertEqual(relDef.GetMetadata("displayGroup"), "Display Group")
        self.assertEqual(relDef.GetMetadata("displayName"), "Display Name")
        self.assertEqual(relDef.GetMetadata("hidden"), True)
        self.assertEqual(relDef.GetMetadata("testCustomMetadata"), "garply")

        # Verify that we get the attribute as a property and it returns all the
        # same metadata values.
        propDef = primDef.GetPropertyDefinition("testRel")
        self.assertTrue(propDef)
        self.assertEqual(propDef.GetName(), "testRel")
        self.assertFalse(propDef.IsAttribute())
        self.assertTrue(propDef.IsRelationship())
        for field in relDef.ListMetadataFields():
            self.assertEqual(relDef.GetMetadata(field),
                             propDef.GetMetadata(field))

    def test_InvalidProperties(self):
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "MetadataTest")
        self.assertTrue(primDef)

        def _VerifyInvalidDef(invalidDef, name):
            # Invalid property definition will convert to false.
            self.assertFalse(invalidDef)

            # Querying IsAttribute and IsRelationship is allowed and will always
            # return false.
            self.assertFalse(invalidDef.IsAttribute())
            self.assertFalse(invalidDef.IsRelationship())
    
            # Querying the name is okay as it doesn't depend on valid property
            # data. The name will match the name of the requested property even
            # if the property doesn't actually exist.
            self.assertEqual(invalidDef.GetName(), name)

            # All other data access is a RuntimeError
            with self.assertRaises(RuntimeError):
                invalidDef.GetSpecType()
            with self.assertRaises(RuntimeError):
                invalidDef.ListMetadataFields()
            with self.assertRaises(RuntimeError):
                invalidDef.GetVariability()
            with self.assertRaises(RuntimeError):
                invalidDef.GetMetadata("displayName")
            # Note these methods only exist on Usd.PrimDefintion.Attribute but
            # a runtime error still gets raised if invalidDef is not an 
            # Attribute
            with self.assertRaises(RuntimeError):
                invalidDef.FallbackValue()
            with self.assertRaises(RuntimeError):
                invalidDef.TypeName()

        # Verify invalid default constructed property definitions.
        _VerifyInvalidDef(Usd.PrimDefinition.Property(), "")
        _VerifyInvalidDef(Usd.PrimDefinition.Attribute(), "")
        _VerifyInvalidDef(Usd.PrimDefinition.Relationship(), "")

        # Verify invalid property definitions returned for properties that 
        # don't exist in the prim definition.
        _VerifyInvalidDef(primDef.GetPropertyDefinition("bogus"), "bogus")
        _VerifyInvalidDef(primDef.GetAttributeDefinition("bogus"), "bogus")
        _VerifyInvalidDef(primDef.GetRelationshipDefinition("bogus"), "bogus")

        # Verify that trying to get an empty named schema property does not 
        # return a valid property. We test this explicitly because, under the 
        # hood, we store prim metadata fallbacks in the prim definition as the 
        # empty name property and we don't want that implementation detail 
        # creeping out into the public API.
        _VerifyInvalidDef(primDef.GetPropertyDefinition(""), "")

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

        # Test getting a schema type name from the abstract base schema 
        # UsdTyped. 
        self.assertEqual(
            Usd.SchemaRegistry.GetSchemaTypeName(Usd.Typed), 
            "Typed")
        self.assertEqual(
            Usd.SchemaRegistry.GetConcreteSchemaTypeName(abstractTest),
            "")
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaTypeName(abstractTest), 
            "")

    def test_FindConcretePrimDefinition(self):
        # MetadataTest is a concrete prim schama. Can get the prim definition
        # through FindConcretePrimDefinition.
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            'MetadataTest')
        self.assertTrue(primDef)

        # Prim def has built in property names.
        self.assertEqual(primDef.GetPropertyNames(), ['testAttr', 'testRel'])

        # Prim def has relationship/property spec for 'testRel'
        self.assertTrue(primDef.GetPropertyDefinition('testRel'))
        self.assertFalse(primDef.GetAttributeDefinition('testRel'))
        self.assertTrue(primDef.GetRelationshipDefinition('testRel'))

        # Prim def has attribute/property spec for 'testAttr'.
        self.assertTrue(primDef.GetPropertyDefinition('testAttr'))
        self.assertTrue(primDef.GetAttributeDefinition('testAttr'))
        self.assertFalse(primDef.GetRelationshipDefinition('testAttr'))

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
            ['collection:__INSTANCE_NAME__',
             'collection:__INSTANCE_NAME__:excludes',
             'collection:__INSTANCE_NAME__:expansionRule', 
             'collection:__INSTANCE_NAME__:includeRoot', 
             'collection:__INSTANCE_NAME__:includes',
             'collection:__INSTANCE_NAME__:membershipExpression'])
        
        # Prim def has relationship/property spec for 'excludes'
        self.assertTrue(primDef.GetPropertyDefinition(
            'collection:__INSTANCE_NAME__:excludes'))
        self.assertFalse(primDef.GetAttributeDefinition(
            'collection:__INSTANCE_NAME__:excludes'))
        self.assertTrue(primDef.GetRelationshipDefinition(
            'collection:__INSTANCE_NAME__:excludes'))

        # Prim def has attribute/property spec for 'expansionRule'.
        self.assertTrue(primDef.GetPropertyDefinition(
            'collection:__INSTANCE_NAME__:expansionRule'))
        self.assertTrue(primDef.GetAttributeDefinition(
            'collection:__INSTANCE_NAME__:expansionRule'))
        self.assertFalse(primDef.GetRelationshipDefinition(
            'collection:__INSTANCE_NAME__:expansionRule'))

        # Prim def has attribute/property spec for 'membershipExpression'.
        self.assertTrue(primDef.GetPropertyDefinition(
            'collection:__INSTANCE_NAME__:membershipExpression'))
        self.assertTrue(primDef.GetAttributeDefinition(
            'collection:__INSTANCE_NAME__:membershipExpression'))
        self.assertFalse(primDef.GetRelationshipDefinition(
            'collection:__INSTANCE_NAME__:membershipExpression'))

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

    def test_GetTypeNameAndInstance(self):
        # test multiplyapply api schema token
        typeNameAndInstance = Usd.SchemaRegistry.GetTypeNameAndInstance(
                "CollectionAPI:lightlink")
        self.assertEqual(typeNameAndInstance[0], 'CollectionAPI')
        self.assertEqual(typeNameAndInstance[1], 'lightlink')

        # test singleapply api schema token
        typeNameAndInstance = Usd.SchemaRegistry.GetTypeNameAndInstance(
                "SingleApplyAPI")
        self.assertEqual(typeNameAndInstance[0], "SingleApplyAPI")
        self.assertEqual(typeNameAndInstance[1], "")

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
            "testDictionaryMetadata" : {"name" : "foo", "value" : 2},
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
                "testDictionaryMetadata" : {"name" : "bar", "value" : 3},
                "variability" : Sdf.VariabilityVarying,
                "customData" : 
                {"userDocBrief" : "Testing brief user doc for schema attr"}
            },
            "testRel" : {
                "custom" : False,
                "displayGroup" : "Display Group",
                "displayName" : "Display Name",
                "documentation" : "Testing documentation metadata",
                "hidden" : True,
                "testCustomMetadata" : "garply",
                "testDictionaryMetadata" : {"name" : "baz", "value" : 5},
                "variability" : Sdf.VariabilityUniform,
                "customData" : 
                {"userDocBrief" : "Testing brief user doc for schema rel"}
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
            "apiSchemas" : Sdf.TokenListOp.CreateExplicit(
                ["CollectionAPI:__INSTANCE_NAME__"]),
            "documentation" : apiPrimDef.GetDocumentation(),
            "specifier" : Sdf.SpecifierOver
        }
        apiPrimDefProperties = {
            "collection:__INSTANCE_NAME__" : {
                "custom" : False,
                "typeName" : Sdf.ValueTypeNames.Opaque,
                "documentation" : apiPrimDef.GetPropertyDocumentation(
                    "collection:__INSTANCE_NAME__"),
                "variability" : Sdf.VariabilityUniform
            },
            "collection:__INSTANCE_NAME__:expansionRule" : {
                "custom" : False,
                "default" : "expandPrims",
                "typeName" : Sdf.ValueTypeNames.Token,
                "allowedTokens" : ["explicitOnly", "expandPrims", 
                                   "expandPrimsAndProperties"],
                "documentation" : apiPrimDef.GetPropertyDocumentation(
                    "collection:__INSTANCE_NAME__:expansionRule"),
                "variability" : Sdf.VariabilityUniform
            },
            "collection:__INSTANCE_NAME__:includeRoot" : {
                "custom" : False,
                "default" : None,
                "typeName" : Sdf.ValueTypeNames.Bool,
                "documentation" : apiPrimDef.GetPropertyDocumentation(
                    "collection:__INSTANCE_NAME__:includeRoot"),
                "variability" : Sdf.VariabilityUniform
            },
            "collection:__INSTANCE_NAME__:includes" : {
                "custom" : False,
                "documentation" : apiPrimDef.GetPropertyDocumentation(
                    "collection:__INSTANCE_NAME__:includes"),
                "variability" : Sdf.VariabilityUniform
            },
            "collection:__INSTANCE_NAME__:excludes" : {
                "custom" : False,
                "documentation" : apiPrimDef.GetPropertyDocumentation(
                    "collection:__INSTANCE_NAME__:excludes"),
                "variability" : Sdf.VariabilityUniform
            },
            "collection:__INSTANCE_NAME__:membershipExpression" : {
                "custom" : False,
                "default" : None,
                "typeName" : Sdf.ValueTypeNames.PathExpression,
                "documentation" : apiPrimDef.GetPropertyDocumentation(
                    "collection:__INSTANCE_NAME__:membershipExpression"),
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
            fooPropName = Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
                propName, "foo")
            newPrimDefProperties[fooPropName] = fieldValues

        # Verify the prim spec matches the prim definition.
        _VerifyExpectedPrimData(layer, "/PrimDefs",
                                newPrimDefPrimFields,
                                newPrimDefProperties)

        # Verify that the existing children of the prim spec as FlattenTo
        # doesn't clear or overwrite fields that can never be schema metadata.
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs/ConcreteSchema"))
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs/APISchema"))
        self.assertTrue(layer.GetPrimAtPath("/PrimDefs/FlattenToNewPrim"))

    def test_MultipleApplyNameTemplates(self):
        # Test making template names with any combination of prefix and base 
        # name.
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameTemplate(
            "foo", "bar"), "foo:__INSTANCE_NAME__:bar")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameTemplate(
            "foo", ""), "foo:__INSTANCE_NAME__")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameTemplate(
            "", "bar"), "__INSTANCE_NAME__:bar")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameTemplate(
            "", ""), "__INSTANCE_NAME__")

        # Test making instance versions of the template names.
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__:bar", "inst1"), "foo:inst1:bar")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__:bar", "inst2"), "foo:inst2:bar")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__", "inst1"), "foo:inst1")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__", "inst2"), "foo:inst2")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "__INSTANCE_NAME__:bar", "inst1"), "inst1:bar")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "__INSTANCE_NAME__:bar", "inst2"), "inst2:bar")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "__INSTANCE_NAME__", "inst1"), "inst1")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "__INSTANCE_NAME__", "inst2"), "inst2")

        # Test getting the base name from a name template.
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo:__INSTANCE_NAME__:bar"), "bar")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo:__INSTANCE_NAME__:bar:baz"), "bar:baz")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo:__INSTANCE_NAME__"), "")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "__INSTANCE_NAME__:bar"), "bar")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "__INSTANCE_NAME__"), "")

        # Test IsMultipleApplyNameTemplate
        # These are all name template names.
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "foo:__INSTANCE_NAME__:bar"))
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "foo:__INSTANCE_NAME__:bar:baz"))
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "foo:__INSTANCE_NAME__"))
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "__INSTANCE_NAME__:bar"))
        self.assertTrue(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "__INSTANCE_NAME__"))
        # The following are not template names
        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "foo"))
        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "foo:__INSTANCE_NAME__bar"))
        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            "foo:__INSTANCE_NAME__bar:baz"))
        self.assertFalse(Usd.SchemaRegistry.IsMultipleApplyNameTemplate(
            ""))

        # Test edge cases for making instanced property names and getting the 
        # property base name. We don't check validity of instanceable names or 
        # instance names.

        # Not an instanceable property name, returns the property name as is.
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo", "inst1"), "foo")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo"), "foo")
        # The instance placeholder must be found as an exact full word match to 
        # be an instanceable property
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME___:bar", "inst1"), "foo:__INSTANCE_NAME___:bar")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo:__INSTANCE_NAME___:bar"), "foo:__INSTANCE_NAME___:bar")
        # If the instance name placeholder is found twice, only the first 
        # occurrence is replaced with the instance name.
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__:__INSTANCE_NAME__", "inst1"), 
            "foo:inst1:__INSTANCE_NAME__")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo:__INSTANCE_NAME__:__INSTANCE_NAME__"), "__INSTANCE_NAME__")
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__:__INSTANCE_NAME__:bar", "inst1"), 
            "foo:inst1:__INSTANCE_NAME__:bar")
        self.assertEqual(Usd.SchemaRegistry.GetMultipleApplyNameTemplateBaseName(
            "foo:__INSTANCE_NAME__:__INSTANCE_NAME__:bar"), 
            "__INSTANCE_NAME__:bar")
        # If the instance name is empty with still replace the instance name
        # placeholder with the empty string.
        self.assertEqual(Usd.SchemaRegistry.MakeMultipleApplyNameInstance(
            "foo:__INSTANCE_NAME__:bar", ""), "foo::bar")

if __name__ == "__main__":
    unittest.main()
