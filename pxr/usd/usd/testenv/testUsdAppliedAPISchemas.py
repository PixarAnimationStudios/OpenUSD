#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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
from pxr import Plug, Sdf, Usd, Vt, Tf

class TestUsdAppliedAPISchemas(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdAppliedAPISchemas", \
            "Failed to load expected test plugin"
        cls.SingleApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")
        cls.MultiApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiApplyAPI")
        cls.SingleCanApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleCanApplyAPI")
        cls.MultiCanApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiCanApplyAPI")
        cls.NestedInnerSingleApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedInnerSingleApplyAPI")
        cls.NestedOuterSingleApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedOuterSingleApplyAPI")
        cls.NestedCycle1APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedCycle1API")
        cls.NestedCycle2APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedCycle2API")
        cls.NestedCycle3APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedCycle3API")
        cls.AutoAppliedToAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestAutoAppliedToAPI")
        cls.NestedAutoAppliedToAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedAutoAppliedToAPI")
        cls.NestedAutoAppliedToAPIAppliedToPrimType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedAutoAppliedToAPIAppliedToPrim")
    
    def test_SimpleTypedSchemaPrimDefinition(self):
        """
        Tests the prim definition for a simple typed schema that has no
        built-in API schemas
        """
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestTypedSchema")
        self.assertTrue(primDef)
        self.assertEqual(primDef.GetPropertyNames(), ["testAttr", "testRel"])
        self.assertEqual(primDef.GetAppliedAPISchemas(), [])
        self.assertEqual(primDef.GetDocumentation(), "Testing typed schema")

        # Verify property specs for named properties.
        for propName in primDef.GetPropertyNames():
            self.assertTrue(primDef.GetSchemaPropertySpec(propName))

        # Verify the attribute spec and its fallback value and type
        testAttr = primDef.GetSchemaAttributeSpec("testAttr")
        self.assertEqual(testAttr.default, "foo")
        self.assertEqual(testAttr.typeName.cppTypeName, "std::string")

        # Verify the relationship spec
        self.assertTrue(primDef.GetSchemaRelationshipSpec("testRel"))

    def test_TypedSchemaWithBuiltinAPISchemas(self):
        """
        Tests the prim definition for schema prim type that has API schemas
        applied to it in its generated schema.
        """

        # Find the prim definition for the test single apply schema. It has
        # some properties defined.
        singleApplyAPIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestSingleApplyAPI")
        self.assertTrue(singleApplyAPIDef)
        self.assertEqual(singleApplyAPIDef.GetAppliedAPISchemas(), 
                         ["TestSingleApplyAPI"])
        self.assertEqual(singleApplyAPIDef.GetPropertyNames(), [
            "single:bool_attr", "single:token_attr", "single:relationship"])
        self.assertEqual(singleApplyAPIDef.GetDocumentation(),
            "Test single apply API schema")

        # Find the prim definition for the test multi apply schema. It has
        # some properties defined. Note that the properties in the multi apply
        # definition are not prefixed yet.
        multiApplyAPIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestMultiApplyAPI")
        self.assertTrue(multiApplyAPIDef)
        self.assertEqual(multiApplyAPIDef.GetAppliedAPISchemas(), [])
        self.assertEqual(multiApplyAPIDef.GetPropertyNames(), [
            "bool_attr", "token_attr", "relationship"])
        self.assertEqual(multiApplyAPIDef.GetDocumentation(),
            "Test multi-apply API schema")

        # Find the prim definition for the concrete prim type with built-in
        # API schemas. You can query its API schemas and it will have properties
        # from those schemas already.
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestWithBuiltinAppliedSchema")
        self.assertTrue(primDef)
        self.assertEqual(primDef.GetAppliedAPISchemas(), [
            "TestSingleApplyAPI", "TestMultiApplyAPI:builtin"])
        self.assertEqual(sorted(primDef.GetPropertyNames()), [
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])
        # Note that prim def documentation does not come from the built-in API
        # schemas.
        self.assertEqual(primDef.GetDocumentation(), 
                         "Test with built-in API schemas")

        # Verify property specs for all named properties.
        for propName in primDef.GetPropertyNames():
            self.assertTrue(primDef.GetSchemaPropertySpec(propName))

        # Verify fallback value and type for properties defined in the 
        # concrete prim
        testAttr = primDef.GetSchemaAttributeSpec("testAttr")
        self.assertEqual(testAttr.default, "foo")
        self.assertEqual(testAttr.typeName.cppTypeName, "std::string")

        self.assertTrue(primDef.GetSchemaRelationshipSpec("testRel"))

        # Verify fallback value and type for properties from the single applied
        # schema. These properties will return the same property spec as the
        # API schema prim definition.
        singleBoolAttr = primDef.GetSchemaAttributeSpec("single:bool_attr")
        self.assertEqual(singleBoolAttr, 
            singleApplyAPIDef.GetSchemaAttributeSpec("single:bool_attr"))
        self.assertEqual(singleBoolAttr.default, True)
        self.assertEqual(singleBoolAttr.typeName.cppTypeName, "bool")

        singleTokenAttr = primDef.GetSchemaAttributeSpec("single:token_attr")
        self.assertEqual(singleTokenAttr, 
            singleApplyAPIDef.GetSchemaAttributeSpec("single:token_attr"))
        self.assertEqual(singleTokenAttr.default, "bar")
        self.assertEqual(singleTokenAttr.typeName.cppTypeName, "TfToken")

        singleRelationship = primDef.GetSchemaRelationshipSpec(
            "single:relationship")
        self.assertTrue(singleRelationship)
        self.assertEqual(singleRelationship, 
            singleApplyAPIDef.GetSchemaRelationshipSpec("single:relationship"))

        # Verify fallback value and type for properties from the multi applied
        # schema. These properties will return the same property spec as the
        # API schema prim definition even the properties on the concrete prim
        # definion are namespace prefixed.
        multiTokenAttr = primDef.GetSchemaAttributeSpec(
            "multi:builtin:token_attr")
        self.assertEqual(multiTokenAttr, 
            multiApplyAPIDef.GetSchemaAttributeSpec("token_attr"))
        self.assertEqual(multiTokenAttr.default, "foo")
        self.assertEqual(multiTokenAttr.typeName.cppTypeName, "TfToken")

        multiRelationship = primDef.GetSchemaRelationshipSpec(
            "multi:builtin:relationship")
        self.assertTrue(multiRelationship)
        self.assertEqual(multiRelationship, 
            multiApplyAPIDef.GetSchemaRelationshipSpec("relationship"))

        # Verify the case where the concrete type overrides a property from 
        # one of its applied API schemas. In this case the property spec from
        # the concrete prim is returned instead of the property spec from the
        # API schema.
        multiBoolAttr = primDef.GetSchemaAttributeSpec(
            "multi:builtin:bool_attr")
        apiBoolAttr = multiApplyAPIDef.GetSchemaAttributeSpec("bool_attr")
        self.assertNotEqual(multiBoolAttr, apiBoolAttr)
        self.assertEqual(multiBoolAttr.default, False)
        self.assertEqual(apiBoolAttr.default, True)
        self.assertEqual(multiBoolAttr.typeName.cppTypeName, "bool")
        self.assertEqual(apiBoolAttr.typeName.cppTypeName, "bool")

    def test_UntypedPrimOnStage(self):
        """
        Tests the fallback properties of untyped prims on a stage when API
        schemas are applied
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a prim with no type. It has no applied schemas or properties.
        untypedPrim = stage.DefinePrim("/Untyped")
        self.assertEqual(untypedPrim.GetTypeName(), '')
        self.assertEqual(untypedPrim.GetAppliedSchemas(), [])
        self.assertEqual(untypedPrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(untypedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         [])
        self.assertEqual(untypedPrim.GetPropertyNames(), [])

        # Add an api schema to the prim's metadata.
        untypedPrim.ApplyAPI(self.SingleApplyAPIType)

        # Prim still has no type but does have applied schemas
        self.assertEqual(untypedPrim.GetTypeName(), '')
        self.assertEqual(untypedPrim.GetAppliedSchemas(), ["TestSingleApplyAPI"])
        self.assertEqual(untypedPrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(untypedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestSingleApplyAPI"])
        self.assertTrue(untypedPrim.HasAPI(self.SingleApplyAPIType))

        # The prim has properties from the applied schema and value resolution
        # returns the applied schema's property fallback value.
        self.assertEqual(untypedPrim.GetPropertyNames(), [
            "single:bool_attr", "single:relationship", "single:token_attr"])
        self.assertEqual(untypedPrim.GetAttribute("single:token_attr").Get(), 
                         "bar")

        # Applied schemas are unable to define fallback metadata values for 
        # prims. Just verifying that no fallback exists for "hidden" here as
        # a contrast to the other cases below where this metadata fallback will
        # be defined.
        self.assertFalse("hidden" in untypedPrim.GetAllMetadata())
        self.assertIsNone(untypedPrim.GetMetadata("hidden"))
        self.assertFalse(untypedPrim.HasAuthoredMetadata("hidden"))

        # Untyped prim still has no documentation even with API schemas applied.
        self.assertIsNone(untypedPrim.GetMetadata("documentation"))

    def test_TypedPrimOnStage(self):
        """
        Tests the fallback properties of typed prims on a stage when API
        schemas are applied when the prim type does not start with API schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim. It has no API schemas but has properties from its
        # type schema.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestTypedSchema")
        self.assertEqual(typedPrim.GetTypeName(), 'TestTypedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), [])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestTypedSchema')
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertEqual(typedPrim.GetPropertyNames(), ["testAttr", "testRel"])

        # Add an api schemas to the prim's metadata.
        typedPrim.ApplyAPI(self.SingleApplyAPIType)
        typedPrim.ApplyAPI(self.MultiApplyAPIType, "garply")

        # Prim has the same type and now has API schemas. The properties have
        # been expanded to include properties from the API schemas
        self.assertEqual(typedPrim.GetTypeName(), 'TestTypedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:garply"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestTypedSchema')
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(),
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:garply"])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:garply:bool_attr", 
            "multi:garply:relationship", 
            "multi:garply:token_attr", 
            "single:bool_attr", 
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Property fallback comes from TestSingleApplyAPI
        attr = typedPrim.GetAttribute("single:token_attr")
        self.assertEqual(attr.Get(), "bar")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestMultiApplyAPI
        attr = typedPrim.GetAttribute("multi:garply:bool_attr")
        self.assertEqual(attr.Get(), True)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestTypedSchema
        attr = typedPrim.GetAttribute("testAttr")
        self.assertEqual(attr.Get(), "foo")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)

        # Metadata "hidden" has a fallback value defined in TestTypedSchema. It
        # will be returned by GetMetadata and GetAllMetadata but will return 
        # false for queries about whether it's authored
        self.assertEqual(typedPrim.GetAllMetadata()["hidden"], True)
        self.assertEqual(typedPrim.GetMetadata("hidden"), True)
        self.assertFalse(typedPrim.HasAuthoredMetadata("hidden"))
        self.assertFalse("hidden" in typedPrim.GetAllAuthoredMetadata())

        # Documentation metadata comes from prim type definition even with API
        # schemas applied.
        self.assertEqual(typedPrim.GetMetadata("documentation"), 
                         "Testing typed schema")

    def test_TypedPrimsOnStageWithBuiltinAPISchemas(self):
        """
        Tests the fallback properties of typed prims on a stage when new API
        schemas are applied to a prim whose type already has built-in applied 
        API schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim. It has API schemas already from its prim definition
        # and has properties from both its type and its APIs.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestWithBuiltinAppliedSchema")
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithBuiltinAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:builtin"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithBuiltinAppliedSchema')
        # Note that prim type info does NOT contain the built-in applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Add a new api schemas to the prim's metadata.
        typedPrim.ApplyAPI(self.MultiApplyAPIType, "garply")

        # Prim has the same type and now has both its original API schemas and
        # the new one. Note that the new schema was added using an explicit 
        # list op but was still prepended to the original list. Built-in API 
        # schemas cannot be deleted and any authored API schemas will always be
        # prepended to the built-ins.
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithBuiltinAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
            ["TestMultiApplyAPI:garply", 
             "TestSingleApplyAPI", "TestMultiApplyAPI:builtin"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithBuiltinAppliedSchema')
        # Note that prim type info does NOT contain the built-in applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestMultiApplyAPI:garply"])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))

        # Properties have been expanded to include the new API schema
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "multi:garply:bool_attr", 
            "multi:garply:relationship", 
            "multi:garply:token_attr", 
            "single:bool_attr", 
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Property fallback comes from TestSingleApplyAPI
        attr = typedPrim.GetAttribute("single:token_attr")
        self.assertEqual(attr.Get(), "bar")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestMultiApplyAPI
        attr = typedPrim.GetAttribute("multi:garply:bool_attr")
        self.assertEqual(attr.Get(), True)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback actually comes from TestWithBuiltinAppliedSchema as
        # the typed schema overrides this property from its built-in API schema.
        attr = typedPrim.GetAttribute("multi:builtin:bool_attr")
        self.assertEqual(attr.Get(), False)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestWithBuiltinAppliedSchema
        attr = typedPrim.GetAttribute("testAttr")
        self.assertEqual(attr.Get(), "foo")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)

        # Metadata "hidden" has a fallback value defined in 
        # TestWithBuiltinAppliedSchema. It will be returned by GetMetadata and 
        # GetAllMetadata but will return false for queries about whether it's 
        # authored
        self.assertEqual(typedPrim.GetAllMetadata()["hidden"], False)
        self.assertEqual(typedPrim.GetMetadata("hidden"), False)
        self.assertFalse(typedPrim.HasAuthoredMetadata("hidden"))
        self.assertFalse("hidden" in typedPrim.GetAllAuthoredMetadata())

        # Documentation metadata comes from prim type definition even with API
        # schemas applied.
        self.assertEqual(typedPrim.GetMetadata("documentation"), 
                         "Test with built-in API schemas")

    def test_TypedPrimsOnStageWithBuiltinReapply(self):
        """
        Tests the fallback properties of typed prims on a stage when the same 
        API schemas are applied again to a prim whose type already has applied 
        API schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim. It has API schemas already from its prim definition
        # and has properties from both its type and its APIs.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestWithBuiltinAppliedSchema")
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithBuiltinAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:builtin"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithBuiltinAppliedSchema')
        # Note that prim type info does NOT contain the built-in applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])
        # Property fallback comes from TestSingleApplyAPI
        attr = typedPrim.GetAttribute("single:token_attr")
        self.assertEqual(attr.Get(), "bar")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback actually comes from TestTypedSchema as the typed
        # schema overrides this property from its built-in API schema.
        attr = typedPrim.GetAttribute("multi:builtin:bool_attr")
        self.assertEqual(attr.Get(), False)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestTypedSchema
        attr = typedPrim.GetAttribute("testAttr")
        self.assertEqual(attr.Get(), "foo")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)

        # Add the built-in api schemas again to the prim's metadata.
        typedPrim.ApplyAPI(self.MultiApplyAPIType, "builtin")
        typedPrim.ApplyAPI(self.SingleApplyAPIType)

        # Prim has the same type and now has both its original API schemas and
        # plus the same schemas again appended to the list (i.e. both schemas
        # now show up twice). 
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithBuiltinAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
            ["TestMultiApplyAPI:builtin", "TestSingleApplyAPI", 
             "TestSingleApplyAPI", "TestMultiApplyAPI:builtin"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithBuiltinAppliedSchema')
        # Note that prim type info does NOT contain the built-in applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestMultiApplyAPI:builtin", "TestSingleApplyAPI"])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))

        # The list of properties hasn't changed as there are no "new" schemas,
        # however the defaults may have changed.
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "single:bool_attr", 
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Property fallback comes from TestSingleApplyAPI - no change
        attr = typedPrim.GetAttribute("single:token_attr")
        self.assertEqual(attr.Get(), "bar")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback has now changed from False to True as the 
        # TestTypedSchema originally overrode the fallback from 
        # TestMultiApplyAPI. But by applying TestMultiApplyAPI again with the 
        # same instance, we've re-overridden the attribute getting the default
        # from the applied schema.
        attr = typedPrim.GetAttribute("multi:builtin:bool_attr")
        self.assertEqual(attr.Get(), True)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestTypedSchema - no change
        attr = typedPrim.GetAttribute("testAttr")
        self.assertEqual(attr.Get(), "foo")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)

        # Prim metadata is unchanged from the case above as there is still
        # no way for applied API schemas to impart prim metadata defaults.
        self.assertEqual(typedPrim.GetAllMetadata()["hidden"], False)
        self.assertEqual(typedPrim.GetMetadata("hidden"), False)
        self.assertFalse(typedPrim.HasAuthoredMetadata("hidden"))
        self.assertFalse("hidden" in typedPrim.GetAllAuthoredMetadata())

        # Documentation metadata comes from prim type definition even with API
        # schemas applied.
        self.assertEqual(typedPrim.GetMetadata("documentation"), 
                         "Test with built-in API schemas")

    @unittest.skipIf(Tf.GetEnvSetting('USD_DISABLE_AUTO_APPLY_API_SCHEMAS'),
                    "Auto apply API schemas are disabled")
    def test_TypedPrimsOnStageWithAutoAppliedAPIs(self):
        """
        Tests the fallback properties of typed prims on a stage where API
        schemas are auto applied.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim that has two types of built-in applied schemas. 
        # TestMultiApplyAPI:builtin comes from the apiSchemas metadata defined
        # in TestTypedSchemaForAutoApply's schema definition.
        # TestSingleApplyAPI and TestMultiApplyAPI:autoFoo come from 
        # TestTypedSchemaForAutoApply being listed in the "AutoApplyAPISchemas"
        # plugInfo metadata for both API schemas.
        # The built-in applied schemas that come from the apiSchemas metadata 
        # will always be listed before (and be stronger than) any applied 
        # schemas that come from apiSchemaAutoApplyTo.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestTypedSchemaForAutoApply")
        self.assertEqual(typedPrim.GetTypeName(), 'TestTypedSchemaForAutoApply')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:builtin", 
                          "TestMultiApplyAPI:autoFoo",
                          "TestSingleApplyAPI"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestTypedSchemaForAutoApply')
        # Note that prim type info does NOT contain the applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:autoFoo:bool_attr", 
            "multi:autoFoo:relationship",
            "multi:autoFoo:token_attr", 
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Add a concrete typed prim which receives an auto applied API schema.
        # TestSingleApplyAPI comes from TestTypedSchemaForAutoApplyConcreteBase 
        # being listed in TestSingleApplyAPI's apiSchemaAutoApplyTo data.
        typedPrim.SetTypeName("TestTypedSchemaForAutoApplyConcreteBase")
        self.assertEqual(typedPrim.GetTypeName(), 
                         'TestTypedSchemaForAutoApplyConcreteBase')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestTypedSchemaForAutoApplyConcreteBase')
        # Note that prim type info does NOT contain the auto applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Add a concrete typed prim which receives an auto applied API schema
        # because it is derived from a base class type that does.
        # TestSingleApplyAPI comes from the base class of this type, 
        # TestTypedSchemaForAutoApplyConcreteBase, being listed in 
        # TestSingleApplyAPI's apiSchemaAutoApplyTo data.
        typedPrim.SetTypeName("TestDerivedTypedSchemaForAutoApplyConcreteBase")
        self.assertEqual(typedPrim.GetTypeName(), 
                         'TestDerivedTypedSchemaForAutoApplyConcreteBase')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestDerivedTypedSchemaForAutoApplyConcreteBase')
        # Note that prim type info does NOT contain the auto applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Add a concrete typed prim which receives an auto applied API schema
        # because it is derived from a base class type that does.
        # TestSingleApplyAPI comes from the base class of this type, 
        # TestTypedSchemaForAutoApplyAbstractBase, being listed in 
        # TestSingleApplyAPI's apiSchemaAutoApplyTo data. This is different 
        # from case above in that the base class is an abstract type and cannot
        # be instantiated as a prim type, but API schemas can still be 
        # designated to auto apply to abstract types to have the API applied
        # to prims of all derived types.
        typedPrim.SetTypeName("TestDerivedTypedSchemaForAutoApplyAbstractBase")
        self.assertEqual(typedPrim.GetTypeName(), 
                         'TestDerivedTypedSchemaForAutoApplyAbstractBase')
        self.assertEqual(typedPrim.GetAppliedSchemas(), ['TestSingleApplyAPI'])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestDerivedTypedSchemaForAutoApplyAbstractBase')
        # Note that prim type info does NOT contain the auto applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Verify that we can get the value of the auto apply API metadata for
        # TestSingleApplyAPI from the schema registry.
        self.assertEqual(
            Usd.SchemaRegistry.GetAutoApplyAPISchemas()['TestSingleApplyAPI'], 
            ['TestTypedSchemaForAutoApplyConcreteBase',
             'TestTypedSchemaForAutoApplyAbstractBase',
             'TestAutoAppliedToAPI',
             'TestTypedSchemaForAutoApply'])

    @unittest.skipIf(not Tf.GetEnvSetting('USD_DISABLE_AUTO_APPLY_API_SCHEMAS'),
                    "Auto apply API schemas are not disabled")
    def test_TypedPrimsOnStageWithAutoAppliedAPIs_AutoApplyDisabled(self):
        """
        Tests the disabling of auto apply schemas through the environment 
        variable USD_DISABLE_AUTO_APPLY_API_SCHEMAS.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim that has two types of built-in applied schemas. 
        # TestMultiApplyAPI:builtin comes from the apiSchemas metadata defined
        # in TestTypedSchemaForAutoApply's schema definition and is NOT affected
        # by disabling auto apply API schemas.
        #
        # TestSingleApplyAPI and TestMultiApplyAPI:autoFoo would come from 
        # TestTypedSchemaForAutoApply being listed in the "AutoApplyAPISchemas"
        # plugInfo metadata for both API schemas, but with auto apply disabled,
        # they are not applied to this type.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestTypedSchemaForAutoApply")
        self.assertEqual(typedPrim.GetTypeName(), 
                         'TestTypedSchemaForAutoApply')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:builtin"])

        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType, 'builtin'))
        self.assertFalse(typedPrim.HasAPI(self.MultiApplyAPIType, 'autoFoo'))
        self.assertFalse(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:builtin:bool_attr", 
            "multi:builtin:relationship",
            "multi:builtin:token_attr", 
            "testAttr", 
            "testRel"])

        # Add a concrete typed prim which receives an auto applied API schema.
        # TestSingleApplyAPI would be auto applied to this type, but with auto
        # apply disable, this type has no applied API schemas.
        typedPrim.SetTypeName("TestTypedSchemaForAutoApplyConcreteBase")
        self.assertEqual(typedPrim.GetTypeName(), 
                         'TestTypedSchemaForAutoApplyConcreteBase')
        self.assertEqual(typedPrim.GetAppliedSchemas(), [])

        self.assertFalse(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "testAttr", 
            "testRel"])

        # Verify that the auto apply API schema dictionary is empty when auto
        # apply is disabled..
        self.assertEqual(Usd.SchemaRegistry.GetAutoApplyAPISchemas(), {})

    def test_ApplyRemoveAPI(self):
        """
        Tests the detail of the Apply and Remove API for API schemas.
        """
        stage = Usd.Stage.CreateInMemory()
        rootLayer = stage.GetRootLayer()
        sessionLayer = stage.GetSessionLayer()
        self.assertTrue(rootLayer)
        self.assertTrue(sessionLayer)

        # Add a basic prim with no type. It has no applied schemas or properties.
        prim = stage.DefinePrim("/Prim")
        self.assertEqual(prim.GetAppliedSchemas(), [])
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        # Helper function for verifying the state of the 'apiSchemas' list op
        # field in the prim spec for the test prim on the specified layer.
        def _VerifyListOp(layer, explicit = [], prepended = [],
                          appended = [], deleted = []):
            spec = layer.GetPrimAtPath('/Prim')
            listOp = spec.GetInfo('apiSchemas')
            self.assertEqual(listOp.explicitItems, explicit)
            self.assertEqual(listOp.prependedItems, prepended)
            self.assertEqual(listOp.appendedItems, appended)
            self.assertEqual(listOp.deletedItems, deleted)

        # Apply a single api schema withe default edit target. Adds to the end
        # prepend list.
        prim.ApplyAPI(self.SingleApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), ["TestSingleApplyAPI"])
        self.assertTrue(prim.HasAPI(self.SingleApplyAPIType))
        _VerifyListOp(rootLayer, prepended = ["TestSingleApplyAPI"])

        # Apply the same API schema again. This will not update the list.
        prim.ApplyAPI(self.SingleApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), ["TestSingleApplyAPI"])
        self.assertTrue(prim.HasAPI(self.SingleApplyAPIType))
        _VerifyListOp(rootLayer, prepended = ["TestSingleApplyAPI"])

        # Remove the API schema. This removes the schema from the prepend and
        # puts in it the deleted list.
        prim.RemoveAPI(self.SingleApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer, deleted = ["TestSingleApplyAPI"])

        # Remove the same API again. This is a no op.
        prim.RemoveAPI(self.SingleApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer, deleted = ["TestSingleApplyAPI"])

        # Remove a multi apply schema which is not currently in the list. The
        # This schema instance name is still added to the deleted list.
        prim.RemoveAPI(self.MultiApplyAPIType, "foo")
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer, 
                      deleted = ["TestSingleApplyAPI", "TestMultiApplyAPI:foo"])

        # Apply the same instance of the multi-apply schema we just deleted. It
        # is added to the prepended but is NOT removed from the deleted list.
        # It still ends up in the composed API schemas since deletes are 
        # processed before prepends in the same list op.
        prim.ApplyAPI(self.MultiApplyAPIType, "foo")
        self.assertEqual(prim.GetAppliedSchemas(), ["TestMultiApplyAPI:foo"])
        _VerifyListOp(rootLayer,
                      prepended = ["TestMultiApplyAPI:foo"], 
                      deleted = ["TestSingleApplyAPI", "TestMultiApplyAPI:foo"])

        # Apply a different instance of the multi-apply schema. Its is added to
        # the end of the prepends list.
        prim.ApplyAPI(self.MultiApplyAPIType, "bar")
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:foo", "TestMultiApplyAPI:bar"])
        _VerifyListOp(rootLayer,
                      prepended = ["TestMultiApplyAPI:foo", "TestMultiApplyAPI:bar"], 
                      deleted = ["TestSingleApplyAPI", "TestMultiApplyAPI:foo"])

        # Remove the "bar" instance of the multi-apply schema on the session
        # layer. The schema is added to the deleted list on the session layer
        # and root layer remains the same. It does not show up in the composed
        # API schemas after composition
        with Usd.EditContext(stage, sessionLayer):
            prim.RemoveAPI(self.MultiApplyAPIType, "bar")
        self.assertEqual(prim.GetAppliedSchemas(), ["TestMultiApplyAPI:foo"])
        _VerifyListOp(rootLayer,
                      prepended = ["TestMultiApplyAPI:foo", "TestMultiApplyAPI:bar"], 
                      deleted = ["TestSingleApplyAPI", "TestMultiApplyAPI:foo"])
        _VerifyListOp(sessionLayer,
                      deleted = ["TestMultiApplyAPI:bar"])

        # Re-apply the "bar" instance of the multi-apply schema on the session
        # layer. It is added to the prepend list in the session layer but still
        # remains in the delete list. Note that the "bar" instance is back in
        # the composed API schemas list but now it is first instead of second
        # like it was before as it get deleted and prepended by the session 
        # layer.
        with Usd.EditContext(stage, sessionLayer):
            prim.ApplyAPI(self.MultiApplyAPIType, "bar")
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:bar", "TestMultiApplyAPI:foo"])
        _VerifyListOp(rootLayer,
                      prepended = ["TestMultiApplyAPI:foo", "TestMultiApplyAPI:bar"], 
                      deleted = ["TestSingleApplyAPI", "TestMultiApplyAPI:foo"])
        _VerifyListOp(sessionLayer,
                      prepended = ["TestMultiApplyAPI:bar"], 
                      deleted = ["TestMultiApplyAPI:bar"])

        # These next few cases verifies the behavior when the list op has 
        # appends or explicit entries. (Note that we don't define behaviors for
        # add or reorder). 
        # Update the session layer to have an appended API schema.
        with Usd.EditContext(stage, sessionLayer):
            appendedListOp = Sdf.TokenListOp()
            appendedListOp.appendedItems = ["TestMultiApplyAPI:bar"]
            prim.SetMetadata('apiSchemas', appendedListOp)
        # Update the root layer to have an explicit list op.
        explicitListOp = Sdf.TokenListOp()
        explicitListOp.explicitItems = ["TestMultiApplyAPI:foo"]
        prim.SetMetadata('apiSchemas', explicitListOp)
        # Verify the initial authored and composed lists.
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:foo", "TestMultiApplyAPI:bar"])
        _VerifyListOp(rootLayer,
                      explicit = ["TestMultiApplyAPI:foo"])
        _VerifyListOp(sessionLayer,
                      appended = ["TestMultiApplyAPI:bar"])

        # On the session and root layers, try to apply the API schema that 
        # is already in each respective list. This will be a no op even though
        # the schemas aren't in the prepended lists.
        with Usd.EditContext(stage, sessionLayer):
            prim.ApplyAPI(self.MultiApplyAPIType, "bar")
        prim.ApplyAPI(self.MultiApplyAPIType, "foo")
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:foo", "TestMultiApplyAPI:bar"])
        _VerifyListOp(rootLayer,
                      explicit = ["TestMultiApplyAPI:foo"])
        _VerifyListOp(sessionLayer,
                      appended = ["TestMultiApplyAPI:bar"])

        # Apply the single apply schema to both layers. The root layer adds it
        # to the end of its explicit list while the session layer will add it
        # the prepends. The composed API schemas will only contain the schema
        # once with the prepend from the stronger session layer winning for 
        # ordering.
        with Usd.EditContext(stage, sessionLayer):
            prim.ApplyAPI(self.SingleApplyAPIType)
        prim.ApplyAPI(self.SingleApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:foo", 
                          "TestMultiApplyAPI:bar"])
        _VerifyListOp(rootLayer,
                      explicit = ["TestMultiApplyAPI:foo", "TestSingleApplyAPI"])
        _VerifyListOp(sessionLayer,
                      prepended = ["TestSingleApplyAPI"],
                      appended = ["TestMultiApplyAPI:bar"])

        # Remove the starting API schemas from the root and session layers. In
        # the root layer it is just removed from the explicit list. In the 
        # session layer it is removed from appends and added to the deletes.
        with Usd.EditContext(stage, sessionLayer):
            prim.RemoveAPI(self.MultiApplyAPIType, "bar")
        prim.RemoveAPI(self.MultiApplyAPIType, "foo")
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI"])
        _VerifyListOp(rootLayer,
                      explicit = ["TestSingleApplyAPI"])
        _VerifyListOp(sessionLayer,
                      prepended = ["TestSingleApplyAPI"],
                      deleted = ["TestMultiApplyAPI:bar"])

        # Clear the apiSchemas in both layers for the next tests.
        # XXX: Should we have additional API for clearing the list op like we 
        # do for other list op fields?
        with Usd.EditContext(stage, sessionLayer):
            prim.SetMetadata('apiSchemas', Sdf.TokenListOp())
        prim.SetMetadata('apiSchemas', Sdf.TokenListOp())
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)
        _VerifyListOp(sessionLayer)

        # Trying to apply or remove a multi-apply schema with no instance name 
        # is an error.
        with self.assertRaises(Tf.ErrorException):
            prim.ApplyAPI(self.MultiApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)
        with self.assertRaises(Tf.ErrorException):
            prim.RemoveAPI(self.MultiApplyAPIType)
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)

        # Trying to apply or remove a single apply schema with an instance name 
        # is an error.
        with self.assertRaises(Tf.ErrorException):
            prim.ApplyAPI(self.SingleApplyAPIType, "foo")
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)
        with self.assertRaises(Tf.ErrorException):
            prim.RemoveAPI(self.SingleApplyAPIType, "foo")
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)

        # Trying to apply or remove a no apply schema is an error.
        with self.assertRaises(Tf.ErrorException):
            prim.ApplyAPI(Usd.ModelAPI)
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)
        with self.assertRaises(Tf.ErrorException):
            prim.RemoveAPI(Usd.ModelAPI)
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer)

        # AddAPITypeName will just add by schema type name with no validity 
        # checks. But it still won't add duplicates.
        #
        # Valid type names
        prim.AddAppliedSchema("TestSingleApplyAPI")
        prim.AddAppliedSchema("TestMultiApplyAPI:bar")
        # Invalid type names.
        prim.AddAppliedSchema("BogusTypeName")
        prim.AddAppliedSchema("TestMultiApplyAPI")
        # Duplicate.
        prim.AddAppliedSchema("TestSingleApplyAPI")
        # Even though invalid type names get added to the apiSchemas metadata,
        # they won't show up in GetAppliedSchemas as they won't be composed into
        # the prim's UsdPrimDefinition.
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:bar"])
        _VerifyListOp(rootLayer, 
                      prepended = ["TestSingleApplyAPI", "TestMultiApplyAPI:bar",
                                   "BogusTypeName", "TestMultiApplyAPI"])

        # RemoveAPITypeName will just delete by schema type name with no 
        # validity checks. But it still won't add duplicate delete entries.
        #
        # Valid type names
        prim.RemoveAppliedSchema("TestSingleApplyAPI")
        prim.RemoveAppliedSchema("TestMultiApplyAPI:bar")
        # Invalid type names.
        prim.RemoveAppliedSchema("BogusTypeName")
        prim.RemoveAppliedSchema("TestMultiApplyAPI")
        # Duplicate.
        prim.RemoveAppliedSchema("TestSingleApplyAPI")
        self.assertEqual(prim.GetAppliedSchemas(), [])
        _VerifyListOp(rootLayer, 
                      deleted = ["TestSingleApplyAPI", "TestMultiApplyAPI:bar",
                                 "BogusTypeName", "TestMultiApplyAPI"])

    def test_CanApplyAPI(self):
        """
        Tests the details of the Usd.Prim.CanApplyAPI.
        """
        stage = Usd.Stage.CreateInMemory()
        rootLayer = stage.GetRootLayer()
        sessionLayer = stage.GetSessionLayer()
        self.assertTrue(rootLayer)
        self.assertTrue(sessionLayer)

        # Add prims of various types to test CanApplyAPI.
        prim = stage.DefinePrim(
            "/Prim")
        prim2 = stage.DefinePrim(
            "/Prim2", "TestTypedSchema")
        prim3 = stage.DefinePrim(
            "/Prim3", "TestTypedSchemaForAutoApply")
        prim4 = stage.DefinePrim(
            "/Prim4", "TestDerivedTypedSchemaForAutoApplyConcreteBase")
        prim5 = stage.DefinePrim(
            "/Prim5", "TestDerivedTypedSchemaForAutoApplyAbstractBase")

        # Single apply schema with no specified "apiSchemaCanOnlyApplyTo" 
        # metadata. Can apply to all prims.
        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestSingleApplyAPI"), [])
        self.assertTrue(prim.CanApplyAPI(self.SingleApplyAPIType))
        self.assertTrue(prim2.CanApplyAPI(self.SingleApplyAPIType))
        self.assertTrue(prim3.CanApplyAPI(self.SingleApplyAPIType))
        self.assertTrue(prim4.CanApplyAPI(self.SingleApplyAPIType))
        self.assertTrue(prim5.CanApplyAPI(self.SingleApplyAPIType))

        # Multiple apply schema with no specified "apiSchemaCanOnlyApplyTo" 
        # metadata and no "allowedInstanceNames". Can apply to all prims with 
        # all instance names (with the notable exception of instance names that
        # match a property name from the schema).
        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiApplyAPI", "foo"), [])
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "foo"))
        self.assertTrue(prim.CanApplyAPI(self.MultiApplyAPIType, "foo"))
        self.assertTrue(prim2.CanApplyAPI(self.MultiApplyAPIType, "foo"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiApplyAPIType, "foo"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiApplyAPIType, "foo"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiApplyAPIType, "foo"))

        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiApplyAPI", "bar"), [])
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "bar"))
        self.assertTrue(prim.CanApplyAPI(self.MultiApplyAPIType, "bar"))
        self.assertTrue(prim2.CanApplyAPI(self.MultiApplyAPIType, "bar"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiApplyAPIType, "bar"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiApplyAPIType, "bar"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiApplyAPIType, "bar"))

        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiApplyAPI", "baz"), [])
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "baz"))
        self.assertTrue(prim.CanApplyAPI(self.MultiApplyAPIType, "baz"))
        self.assertTrue(prim2.CanApplyAPI(self.MultiApplyAPIType, "baz"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiApplyAPIType, "baz"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiApplyAPIType, "baz"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiApplyAPIType, "baz"))

        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiApplyAPI", "Bar"), [])
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "Bar"))
        self.assertTrue(prim.CanApplyAPI(self.MultiApplyAPIType, "Bar"))
        self.assertTrue(prim2.CanApplyAPI(self.MultiApplyAPIType, "Bar"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiApplyAPIType, "Bar"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiApplyAPIType, "Bar"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiApplyAPIType, "Bar"))

        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiApplyAPI", "qux"), [])
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "qux"))
        self.assertTrue(prim.CanApplyAPI(self.MultiApplyAPIType, "qux"))
        self.assertTrue(prim2.CanApplyAPI(self.MultiApplyAPIType, "qux"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiApplyAPIType, "qux"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiApplyAPIType, "qux"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiApplyAPIType, "qux"))

        # As mentioned above, property names of the API schema are not valid
        # instance names and will return false for CanApplyAPI
        self.assertFalse(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "bool_attr"))
        self.assertFalse(prim.CanApplyAPI(self.MultiApplyAPIType, "bool_attr"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiApplyAPIType, "bool_attr"))
        self.assertFalse(prim3.CanApplyAPI(self.MultiApplyAPIType, "bool_attr"))
        self.assertFalse(prim4.CanApplyAPI(self.MultiApplyAPIType, "bool_attr"))
        self.assertFalse(prim5.CanApplyAPI(self.MultiApplyAPIType, "bool_attr"))

        self.assertFalse(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiApplyAPI", "relationship"))
        self.assertFalse(prim.CanApplyAPI(self.MultiApplyAPIType, "relationship"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiApplyAPIType, "relationship"))
        self.assertFalse(prim3.CanApplyAPI(self.MultiApplyAPIType, "relationship"))
        self.assertFalse(prim4.CanApplyAPI(self.MultiApplyAPIType, "relationship"))
        self.assertFalse(prim5.CanApplyAPI(self.MultiApplyAPIType, "relationship"))

        # Single apply API schema that does specify an "apiSchemaCanOnlyApplyTo"
        # list. prim3 is a type in the list and prim4 derives from a type in the
        # list so only these return true.
        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestSingleCanApplyAPI"), 
            ["TestTypedSchemaForAutoApply", 
             "TestTypedSchemaForAutoApplyConcreteBase"])
        self.assertFalse(prim.CanApplyAPI(self.SingleCanApplyAPIType))
        self.assertFalse(prim2.CanApplyAPI(self.SingleCanApplyAPIType))
        self.assertTrue(prim3.CanApplyAPI(self.SingleCanApplyAPIType))
        self.assertTrue(prim4.CanApplyAPI(self.SingleCanApplyAPIType))
        self.assertFalse(prim5.CanApplyAPI(self.SingleCanApplyAPIType))

        # Multiple apply API schema that specifies allow instance names 
        # "foo", "bar", and "baz". All other instance names aren't allowed 
        # and will return false.
        self.assertFalse(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiCanApplyAPI", "Bar"))
        self.assertFalse(prim.CanApplyAPI(self.MultiCanApplyAPIType, "Bar"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiCanApplyAPIType, "Bar"))
        self.assertFalse(prim3.CanApplyAPI(self.MultiCanApplyAPIType, "Bar"))
        self.assertFalse(prim4.CanApplyAPI(self.MultiCanApplyAPIType, "Bar"))
        self.assertFalse(prim5.CanApplyAPI(self.MultiCanApplyAPIType, "Bar"))

        self.assertFalse(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiCanApplyAPI", "qux"))
        self.assertFalse(prim.CanApplyAPI(self.MultiCanApplyAPIType, "qux"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiCanApplyAPIType, "qux"))
        self.assertFalse(prim3.CanApplyAPI(self.MultiCanApplyAPIType, "qux"))
        self.assertFalse(prim4.CanApplyAPI(self.MultiCanApplyAPIType, "qux"))
        self.assertFalse(prim5.CanApplyAPI(self.MultiCanApplyAPIType, "qux"))

        # Same multiple apply API schema with allowed instance name "baz". 
        # The API schema type specifies an "apiSchemaCanOnlyApplyTo" list so 
        # this instance can only be applied to those types. prim3 is a type in 
        # the list and prim5 derives from a type in the list so only these 
        # return true.
        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiCanApplyAPI", "baz"), 
            ["TestTypedSchemaForAutoApply", 
             "TestTypedSchemaForAutoApplyAbstractBase"])
        self.assertEqual(
            Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
                "TestMultiCanApplyAPI", "baz"), 
            Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
                "TestMultiCanApplyAPI"))
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiCanApplyAPI", "baz"))
        self.assertFalse(prim.CanApplyAPI(self.MultiCanApplyAPIType, "baz"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiCanApplyAPIType, "baz"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiCanApplyAPIType, "baz"))
        self.assertFalse(prim4.CanApplyAPI(self.MultiCanApplyAPIType, "baz"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiCanApplyAPIType, "baz"))

        # Same multiple apply API schema with allowed instance name "foo". 
        # The API schema type specifies an "apiSchemaCanOnlyApplyTo" list 
        # specifically for "foo" so this instance can only be applied to those 
        # types. prim3 is a type in the list and prim4 derives from a type in 
        # the list so only these return true.
        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiCanApplyAPI", "foo"), 
            ["TestTypedSchemaForAutoApply", 
             "TestTypedSchemaForAutoApplyConcreteBase"])
        self.assertNotEqual(
            Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
                "TestMultiCanApplyAPI", "foo"), 
            Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
                "TestMultiCanApplyAPI"))
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiCanApplyAPI", "foo"))
        self.assertFalse(prim.CanApplyAPI(self.MultiCanApplyAPIType, "foo"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiCanApplyAPIType, "foo"))
        self.assertTrue(prim3.CanApplyAPI(self.MultiCanApplyAPIType, "foo"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiCanApplyAPIType, "foo"))
        self.assertFalse(prim5.CanApplyAPI(self.MultiCanApplyAPIType, "foo"))

        # Same multiple apply API schema with allowed instance name "bar". 
        # The API schema type specifies yet another "apiSchemaCanOnlyApplyTo" 
        # list specifically for "bar" so this instance can only be applied to 
        # those types. prim4 and prim5 each derive from a different type in 
        # the list so only these return true.
        self.assertEqual(Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
            "TestMultiCanApplyAPI", "bar"), 
            ["TestTypedSchemaForAutoApplyAbstractBase", 
             "TestTypedSchemaForAutoApplyConcreteBase"])
        self.assertNotEqual(
            Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
                "TestMultiCanApplyAPI", "bar"), 
            Usd.SchemaRegistry.GetAPISchemaCanOnlyApplyToTypeNames(
                "TestMultiCanApplyAPI"))
        self.assertTrue(Usd.SchemaRegistry.IsAllowedAPISchemaInstanceName(
            "TestMultiCanApplyAPI", "bar"))
        self.assertFalse(prim.CanApplyAPI(self.MultiCanApplyAPIType, "bar"))
        self.assertFalse(prim2.CanApplyAPI(self.MultiCanApplyAPIType, "bar"))
        self.assertFalse(prim3.CanApplyAPI(self.MultiCanApplyAPIType, "bar"))
        self.assertTrue(prim4.CanApplyAPI(self.MultiCanApplyAPIType, "bar"))
        self.assertTrue(prim5.CanApplyAPI(self.MultiCanApplyAPIType, "bar"))

        # Error conditions
        # Coding error if called on single apply schema with an instance name.
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(prim.CanApplyAPI(self.SingleApplyAPIType, "foo"))
        # Coding error if called on multiple apply schema without an instance 
        # name.
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(prim.CanApplyAPI(self.MultiApplyAPIType))
        # Coding error if called on multiple apply schema with an empty instance 
        # name.
        with self.assertRaises(Tf.ErrorException):
            self.assertFalse(prim.CanApplyAPI(self.MultiApplyAPIType, ""))

        # Verify whyNot annotations when CanApplyAPI is false.
        result = prim4.CanApplyAPI(self.MultiCanApplyAPIType, "bar")
        self.assertTrue(result)
        self.assertEqual(result.whyNot, "")

        result = prim.CanApplyAPI(self.MultiCanApplyAPIType, "qux")
        self.assertFalse(result)
        self.assertEqual(result.whyNot, 
                         "'qux' is not an allowed instance name for multiple "
                         "apply API schema 'TestMultiCanApplyAPI'.")

        result = prim.CanApplyAPI(self.MultiCanApplyAPIType, "bar")
        self.assertFalse(result)
        self.assertEqual(result.whyNot, 
                         "API schema 'TestMultiCanApplyAPI:bar' can only be "
                         "applied to prims of the following types: "
                         "TestTypedSchemaForAutoApplyAbstractBase, "
                         "TestTypedSchemaForAutoApplyConcreteBase.")

        result = prim.CanApplyAPI(self.MultiApplyAPIType, "bool_attr")
        self.assertFalse(result)
        self.assertEqual(result.whyNot, 
                         "'bool_attr' is not an allowed instance name for "
                         "multiple apply API schema 'TestMultiApplyAPI'.")

    def test_NestedAPISchemas(self):
        """
        Tests the application of API schemas that have nested built-in API 
        schemas
        """
        stage = Usd.Stage.CreateInMemory()

        # Simple helper for testing that a prim has expected attributes that 
        # resolve to expected values.
        def _VerifyAttrValues(prim, expectedAttrValues):
            values = {name : prim.GetAttribute(name).Get() 
                         for name in expectedAttrValues.keys()}
            self.assertEqual(values, expectedAttrValues)

        # Add a prim with no type and apply the TestNestedInnerSingleApplyAPI.
        innerSinglePrim = stage.DefinePrim("/InnerSingle")
        innerSinglePrim.ApplyAPI(self.NestedInnerSingleApplyAPIType)

        # The authored applied API schemas for the prim is only the applied
        # TestNestedInnerSingleApplyAPI.
        self.assertEqual(innerSinglePrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(innerSinglePrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedInnerSingleApplyAPI"])

        # The composed applied API schemas however also contain the 
        # TestSingleApplyAPI and the "bar" instance of TestMultiApplyAPI as 
        # these are built-in APIs of TestNestedInnerSingleApplyAPI 
        expectedAPISchemas = [
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar"]
        self.assertEqual(innerSinglePrim.GetAppliedSchemas(), 
                         expectedAPISchemas)
        # The prim "has" all these built-in APIs as well.
        self.assertTrue(innerSinglePrim.HasAPI(self.NestedInnerSingleApplyAPIType))
        self.assertTrue(innerSinglePrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(innerSinglePrim.HasAPI(self.MultiApplyAPIType))
        self.assertTrue(innerSinglePrim.HasAPI(self.MultiApplyAPIType, "bar"))

        # Properties come from all composed built-in APIs
        expectedPropNames = [
            # Properties from TestNestedInnerSingleApplyAPI
            "innerSingle:int_attr", 
            "innerSingle:relationship", 
            "innerSingle:token_attr",
            # Properties from TestMultiApplyAPI:bar
            "multi:bar:bool_attr", 
            "multi:bar:relationship", 
            "multi:bar:token_attr",
            # Properties from TestSingleApplyAPI
            "single:bool_attr",
            "single:relationship",
            "single:token_attr"]
        self.assertEqual(innerSinglePrim.GetPropertyNames(), expectedPropNames)

        # Verify that the attribute fallback values come from the API schemas
        # that define them. The attribute "multi:bar:token_attr" is defined in
        # TestNestedInnerSingleApplyAPI and overrides the attr fallback value 
        # defined in TestMultiApplyAPI:bar
        expectedAttrValues = {
            "multi:bar:token_attr" : "inner_override",
            "multi:bar:bool_attr" : True,
            "innerSingle:token_attr" : "inner",
            "innerSingle:int_attr" : 3,
            "single:token_attr" : "bar",
            "single:bool_attr" : True}
        _VerifyAttrValues(innerSinglePrim, expectedAttrValues)

        # Get the prim definition for the API schema and verify its applied
        # API schemas and properties match what was imparted on the prim.
        innerSingleApplyAPIDef = \
            Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
                "TestNestedInnerSingleApplyAPI")
        self.assertTrue(innerSingleApplyAPIDef)
        self.assertEqual(innerSingleApplyAPIDef.GetAppliedAPISchemas(), 
                         expectedAPISchemas)
        self.assertEqual(sorted(innerSingleApplyAPIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(innerSingleApplyAPIDef.GetDocumentation(),
            "Test nested single apply API schema: inner schema")

        # Add a prim with no type and apply the TestNestedOuterSingleApplyAPI.
        outerSinglePrim = stage.DefinePrim("/OuterSingle")
        outerSinglePrim.ApplyAPI(self.NestedOuterSingleApplyAPIType)

        # The authored applied API schemas for the prim is only the applied
        # TestNestedOuterSingleApplyAPI.
        self.assertEqual(outerSinglePrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(outerSinglePrim.GetPrimTypeInfo().GetAppliedAPISchemas(),
                         ["TestNestedOuterSingleApplyAPI"])

        # The composed applied API schemas however also contain the 
        # the "foo" instance of TestMultiApplyAPI and 
        # TestNestedInnerSingleApplyAPI and as these are built-in APIs of 
        # TestNestedOuterSingleApplyAPI. However, because 
        # TestNestedInnerSingleApplyAPI has its own built-in API schemas, those
        # are also pulled into the composed applied API schemas for this prim
        self.assertEqual(outerSinglePrim.GetTypeName(), '')
        expectedAPISchemas = [
            "TestNestedOuterSingleApplyAPI",
            "TestMultiApplyAPI:foo",
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar"]
        self.assertEqual(outerSinglePrim.GetAppliedSchemas(), 
                         expectedAPISchemas)

        # Properties come from all composed built-in APIs
        expectedPropNames = [
            "innerSingle:int_attr",
            "innerSingle:relationship",
            "innerSingle:token_attr",
            # Properties from TestMultiApplyAPI:bar (included from 
            # TestNestedInnerSingleApplyAPI)
            "multi:bar:bool_attr", 
            "multi:bar:relationship", 
            "multi:bar:token_attr",
            # Properties from TestMultiApplyAPI:foo 
            "multi:foo:bool_attr", 
            "multi:foo:relationship", 
            "multi:foo:token_attr",
            # Properties from TestNestedOuterSingleApplyAPI
            "outerSingle:int_attr",
            "outerSingle:relationship",
            "outerSingle:token_attr",
            # Properties from TestSingleApplyAPI (included from 
            # TestNestedInnerSingleApplyAPI)
            "single:bool_attr",
            "single:relationship",
            "single:token_attr"]
        self.assertEqual(outerSinglePrim.GetPropertyNames(), expectedPropNames)

        # Verify that the attribute fallback values come from the API schemas
        # that define them. The attribute override for "multi:bar:token_attr" 
        # from TestNestedInnerSingleApplyAPI still comes through. 
        # Also The attribute "single:token_attr" is defined in
        # TestNestedOuterSingleApplyAPI and overrides the attr fallback value 
        # defined in TestSingleApplyAPI        
        expectedAttrValues = {
            "innerSingle:token_attr" : "inner",
            "innerSingle:int_attr" : 3,
            "multi:bar:token_attr" : "inner_override",
            "multi:bar:bool_attr" : True,
            "multi:foo:token_attr" : "foo",
            "multi:foo:bool_attr" : True,
            "outerSingle:token_attr" : "outer",
            "outerSingle:int_attr" : 4,
            "single:token_attr" : "outer_override",
            "single:bool_attr" : True}
        _VerifyAttrValues(outerSinglePrim, expectedAttrValues)

        # Get the prim definition for the API schema and verify its applied
        # API schemas and properties match what was imparted on the prim.
        outerSingleApplyAPIDef = \
            Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
                "TestNestedOuterSingleApplyAPI")
        self.assertTrue(outerSingleApplyAPIDef)
        self.assertEqual(outerSingleApplyAPIDef.GetAppliedAPISchemas(), 
                         expectedAPISchemas)
        self.assertEqual(sorted(outerSingleApplyAPIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(outerSingleApplyAPIDef.GetDocumentation(),
            "Test nested single apply API schema: outer schema")

        # Apply the TestNestedInnerSingleApplyAPI to the same prim. This API
        # is already included through the TestNestedOuterSingleApplyAPI.
        outerSinglePrim.ApplyAPI(self.NestedInnerSingleApplyAPIType)

        # The authored applied API schemas for the prim now contain both the 
        # applied APIs.
        self.assertEqual(outerSinglePrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(outerSinglePrim.GetPrimTypeInfo().GetAppliedAPISchemas(),
                         ["TestNestedOuterSingleApplyAPI",
                          "TestNestedInnerSingleApplyAPI"])

        # The composed applied API schemas are fully expanded from both authored
        # APIs and will contain duplicates. We don't check for duplicates that 
        # occur because different authored APIs include the same applied schema
        # even though we do remove duplicates when determining what API schemas
        # a single API includes.
        self.assertEqual(outerSinglePrim.GetTypeName(), '')
        expectedAPISchemas = [
            # API schemas from expanded TestNestedOuterSingleApplyAPI
            "TestNestedOuterSingleApplyAPI",
            "TestMultiApplyAPI:foo",
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar",
            # API schemas from expanded TestNestedInnerSingleApplyAPI (even
            # though they're already in the list)
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar"]
        self.assertEqual(outerSinglePrim.GetAppliedSchemas(), 
                         expectedAPISchemas)

        # The properties list and attribute values haven't changed though 
        # because no actual new API schemas were added and the strength order 
        # of the APIs haven't changed.
        self.assertEqual(outerSinglePrim.GetPropertyNames(), expectedPropNames)
        _VerifyAttrValues(outerSinglePrim, expectedAttrValues)

        # Now let's swap the order of these two APIs so that the inner comes
        # before the outer.
        outerSinglePrim.RemoveAPI(self.NestedOuterSingleApplyAPIType)
        outerSinglePrim.ApplyAPI(self.NestedOuterSingleApplyAPIType)
        self.assertEqual(outerSinglePrim.GetPrimTypeInfo().GetAppliedAPISchemas(),
                         ["TestNestedInnerSingleApplyAPI",
                          "TestNestedOuterSingleApplyAPI"])

        # The order of the expanded API schemas has now changed too.
        expectedAPISchemas = [
            # API schemas from expanded TestNestedInnerSingleApplyAPI
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar",
            # API schemas from expanded TestNestedOuterSingleApplyAPI
            "TestNestedOuterSingleApplyAPI",
            "TestMultiApplyAPI:foo",
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar"]
        self.assertEqual(outerSinglePrim.GetAppliedSchemas(), 
                         expectedAPISchemas)

        # Once again The properties list doesn't change.
        self.assertEqual(outerSinglePrim.GetPropertyNames(), expectedPropNames)
        # However, the attribute value for single:token_attr has changed
        # because the override from TestNestedOuterSingleApplyAPI is no longer
        # stronger than the value from TestSingleApplyAPI as the first instance
        # of TestSingleApplyAPI now comes before it.
        expectedAttrValues["single:token_attr"] = "bar"
        _VerifyAttrValues(outerSinglePrim, expectedAttrValues)


    def test_NestedCycleAPISchema(self):
        """
        Tests the application of API schemas that have nested built-in API 
        schemas that would cause an inclusion cycle
        """
        stage = Usd.Stage.CreateInMemory()

        # Simple helper for testing that a prim has expected attributes that 
        # resolve to expected values.
        def _VerifyAttrValues(prim, expectedAttrValues):
            values = {name : prim.GetAttribute(name).Get() 
                         for name in expectedAttrValues.keys()}
            self.assertEqual(values, expectedAttrValues)

        # Test behavior when nested API schema form a cycle. In this example
        # TestNestedCycle1API includes TestNestedCycle2API which includes 
        # TestNestedCycle3API which includes TestNestedCycle1API again. Create
        # three prims, each applying one of the API schemas.
        nestedCyclePrim1 = stage.DefinePrim("/Cycle1")
        nestedCyclePrim2 = stage.DefinePrim("/Cycle2")
        nestedCyclePrim3 = stage.DefinePrim("/Cycle3")
        nestedCyclePrim1.ApplyAPI(self.NestedCycle1APIType)
        nestedCyclePrim2.ApplyAPI(self.NestedCycle2APIType)
        nestedCyclePrim3.ApplyAPI(self.NestedCycle3APIType)

        # For each prim the authored applied API schemas for the prim are still
        # only the single API that was applied.
        self.assertEqual(nestedCyclePrim1.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(nestedCyclePrim1.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedCycle1API"])
        self.assertEqual(nestedCyclePrim2.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(nestedCyclePrim2.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedCycle2API"])
        self.assertEqual(nestedCyclePrim3.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(nestedCyclePrim3.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedCycle3API"])

        # The composed applied API schemas all contain all three API schemas,
        # however the order is different based on which schema was actually
        # the authored one. Note that during API schema expansion we don't try
        # to include the same API schema more than once which allows us to 
        # handle cycles in this way.
        self.assertEqual(nestedCyclePrim1.GetTypeName(), '')
        self.assertEqual(nestedCyclePrim1.GetAppliedSchemas(), [
            "TestNestedCycle1API",
            "TestNestedCycle2API",
            "TestNestedCycle3API"])
        self.assertEqual(nestedCyclePrim2.GetTypeName(), '')
        self.assertEqual(nestedCyclePrim2.GetAppliedSchemas(), [
            "TestNestedCycle2API",
            "TestNestedCycle3API",
            "TestNestedCycle1API"])
        self.assertEqual(nestedCyclePrim3.GetTypeName(), '')
        self.assertEqual(nestedCyclePrim3.GetAppliedSchemas(), [
            "TestNestedCycle3API",
            "TestNestedCycle1API",
            "TestNestedCycle2API"])
        # All three prims "has" all three built-in APIs as well.
        self.assertTrue(nestedCyclePrim1.HasAPI(self.NestedCycle1APIType))
        self.assertTrue(nestedCyclePrim1.HasAPI(self.NestedCycle2APIType))
        self.assertTrue(nestedCyclePrim1.HasAPI(self.NestedCycle3APIType))

        self.assertTrue(nestedCyclePrim2.HasAPI(self.NestedCycle1APIType))
        self.assertTrue(nestedCyclePrim2.HasAPI(self.NestedCycle2APIType))
        self.assertTrue(nestedCyclePrim2.HasAPI(self.NestedCycle3APIType))

        self.assertTrue(nestedCyclePrim3.HasAPI(self.NestedCycle1APIType))
        self.assertTrue(nestedCyclePrim3.HasAPI(self.NestedCycle2APIType))
        self.assertTrue(nestedCyclePrim3.HasAPI(self.NestedCycle3APIType))

        # All three prims have all the same properties since they all have all
        # three API schemas applied.
        expectedPropNames = [
            "cycle1:token_attr",
            "cycle2:token_attr",
            "cycle3:token_attr",
            "cycle:int_attr"]
        self.assertEqual(nestedCyclePrim1.GetPropertyNames(), expectedPropNames)
        self.assertEqual(nestedCyclePrim2.GetPropertyNames(), expectedPropNames)
        self.assertEqual(nestedCyclePrim3.GetPropertyNames(), expectedPropNames)

        # For the three token attributes, each is only defined in its respective
        # API schema so they have the same fallback in each prim. 
        # 'cycle:int_attr' is defined in all three of the cycle API schemas but
        # with different default values. Here the order of the applied schemas
        # matters and the strongest applied API's value wins.
        expectedAttrValues = {
            "cycle1:token_attr" : "cycle1",
            "cycle2:token_attr" : "cycle2",
            "cycle3:token_attr" : "cycle3"}
        expectedAttrValues["cycle:int_attr"] = 1
        _VerifyAttrValues(nestedCyclePrim1, expectedAttrValues)
        expectedAttrValues["cycle:int_attr"] = 2
        _VerifyAttrValues(nestedCyclePrim2, expectedAttrValues)
        expectedAttrValues["cycle:int_attr"] = 3
        _VerifyAttrValues(nestedCyclePrim3, expectedAttrValues)

        # Get the prim definitions for each of these API schemas and verify its 
        # applied API schemas and properties match what was imparted on the 
        # prims.
        cycle1APIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestNestedCycle1API")
        self.assertTrue(cycle1APIDef)
        self.assertEqual(cycle1APIDef.GetAppliedAPISchemas(), 
            ["TestNestedCycle1API", 
             "TestNestedCycle2API", 
             "TestNestedCycle3API"])
        self.assertEqual(sorted(cycle1APIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(cycle1APIDef.GetDocumentation(),
            "Test nested single apply API schema with a cycle #1")

        cycle2APIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestNestedCycle2API")
        self.assertTrue(cycle2APIDef)
        self.assertEqual(cycle2APIDef.GetAppliedAPISchemas(), 
            ["TestNestedCycle2API", 
             "TestNestedCycle3API", 
             "TestNestedCycle1API"])
        self.assertEqual(sorted(cycle2APIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(cycle2APIDef.GetDocumentation(),
            "Test nested single apply API schema with a cycle #2")

        cycle3APIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestNestedCycle3API")
        self.assertTrue(cycle3APIDef)
        self.assertEqual(cycle3APIDef.GetAppliedAPISchemas(), 
            ["TestNestedCycle3API", 
             "TestNestedCycle1API", 
             "TestNestedCycle2API"])
        self.assertEqual(sorted(cycle3APIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(cycle3APIDef.GetDocumentation(),
            "Test nested single apply API schema with a cycle #3")

    def test_ConcreteTypeWithBuiltinNestedAPISchemas(self):
        """
        Tests a concrete schema type with built-in API schemas that include
        other API schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Simple helper for testing that a prim has expected attributes that 
        # resolve to expected values.
        def _VerifyAttrValues(prim, expectedAttrValues):
            values = {name : prim.GetAttribute(name).Get() 
                         for name in expectedAttrValues.keys()}
            self.assertEqual(values, expectedAttrValues)

        # Test a typed prim whose concrete typed schema has built-in API schemas
        # that nest other API schemas.
        typedPrim = stage.DefinePrim(
            "/TypedPrim", "TestWithBuiltinNestedAppliedSchema")

        # The prim has a type but no authored API schemas.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         "TestWithBuiltinNestedAppliedSchema")
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        # The composed API schemas are fully expanded from the two built-in API
        # schemas of the TestWithBuiltinNestedAppliedSchema type.
        expectedAPISchemas = [
            # Expanded API schemas from built-in TestNestedOuterSingleApplyAPI
            "TestNestedOuterSingleApplyAPI",
            "TestMultiApplyAPI:foo",
            "TestNestedInnerSingleApplyAPI",
            "TestSingleApplyAPI",
            "TestMultiApplyAPI:bar",
            # Expanded API schemas from built-in TestNestedCycle1API
            "TestNestedCycle1API",
            "TestNestedCycle2API",
            "TestNestedCycle3API"]
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         expectedAPISchemas)

        # Properties come from the type and all composed built-in APIs
        expectedPropNames = [
            # Properties from expanded built-in TestNestedCycle1API
            "cycle1:token_attr",
            "cycle2:token_attr",
            "cycle3:token_attr",
            "cycle:int_attr",
            # Properties from expanded built-in TestNestedOuterSingleApplyAPI
            "innerSingle:int_attr",
            "innerSingle:relationship",
            "innerSingle:token_attr",
            "multi:bar:bool_attr", 
            "multi:bar:relationship", 
            "multi:bar:token_attr",
            "multi:foo:bool_attr", 
            "multi:foo:relationship", 
            "multi:foo:token_attr",
            "outerSingle:int_attr",
            "outerSingle:relationship",
            "outerSingle:token_attr",
            "single:bool_attr",
            "single:relationship",
            "single:token_attr",
            # Properties from the prim type TestWithBuiltinNestedAppliedSchema
            "testAttr",
            "testRel"]
        self.assertEqual(typedPrim.GetPropertyNames(), expectedPropNames)

        # Get the prim definition for the concrete typed schema and verify its 
        # applied API schemas and properties match what was imparted on the 
        # prim.
        typedPrimDef = \
            Usd.SchemaRegistry().FindConcretePrimDefinition(
                "TestWithBuiltinNestedAppliedSchema")
        self.assertTrue(typedPrimDef)
        self.assertEqual(typedPrimDef.GetAppliedAPISchemas(), 
                         expectedAPISchemas)
        self.assertEqual(sorted(typedPrimDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(typedPrimDef.GetDocumentation(),
            "Test with built-in nested API schemas")

    @unittest.skipIf(Tf.GetEnvSetting('USD_DISABLE_AUTO_APPLY_API_SCHEMAS'),
                    "Auto apply API schemas are disabled")
    def test_APISchemasAutoAppliedToAPISchemas(self):
        """
        Tests the behaviors of API schemas that are auto applied to other API
        schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Define a prim with an empty type name and apply TestAutoAppliedToAPI.
        # TestAutoAppliedToAPI includes other API schemas through a combination 
        # of built-in APIs and auto applied APIs.
        prim = stage.DefinePrim("/Prim")
        prim.ApplyAPI(self.AutoAppliedToAPIType)
        self.assertEqual(prim.GetTypeName(), '')
        self.assertEqual(prim.GetAppliedSchemas(), [
            # Authored applied API
            "TestAutoAppliedToAPI",
            # Built-in API of 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:builtin",
            # Defined in plugin metadata that 'TestMultiApplyAPI:autoFoo' auto
            # applies to 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:autoFoo",
            # 'TestSingleApplyAPI' defines in its schema def that it auto 
            # applies to 'TestAutoAppliedToAPI'
            "TestSingleApplyAPI"])
        self.assertTrue(prim.HasAPI(self.AutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "builtin"))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "autoFoo"))
        self.assertTrue(prim.HasAPI(self.SingleApplyAPIType))

        # Prim's authored type is empty and its authored API schemas is just the
        # single authored schema.
        self.assertEqual(prim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestAutoAppliedToAPI"])

        # Prim's built-in properties come from all of the applied API schemas.
        self.assertEqual(prim.GetPropertyNames(), [
            "multi:autoFoo:bool_attr",
            "multi:autoFoo:relationship",
            "multi:autoFoo:token_attr",
            "multi:builtin:bool_attr",
            "multi:builtin:relationship",
            "multi:builtin:token_attr",
            "single:bool_attr",
            "single:relationship",
            "single:token_attr",
            "testAttr",
            "testRel"])

        # Define a prim with an empty type name and apply 
        # TestNestedAutoAppliedToAPI.
        # TestAutoAppliedToAPI auto applies to TestNestedAutoAppliedToAPI and 
        # brings with it all of the API schemas that are built-in to it and auto
        # applied to it.
        prim = stage.DefinePrim("/Prim2")
        prim.ApplyAPI(self.NestedAutoAppliedToAPIType)
        self.assertEqual(prim.GetTypeName(), '')
        self.assertEqual(prim.GetAppliedSchemas(), [
            # Authored applied API
            "TestNestedAutoAppliedToAPI",
            # Built-in API of 'TestNestedAutoAppliedToAPI'
            "TestMultiApplyAPI:foo",
            # 'TestAutoAppliedToAPI' defines in its schema def that it auto 
            # applies to 'TestNestedAutoAppliedToAPI'
            "TestAutoAppliedToAPI",
            # Built-in API of 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:builtin",
            # Defined in plugin metadata that 'TestMultiApplyAPI:autoFoo' auto
            # applies to 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:autoFoo",
            # 'TestSingleApplyAPI' defines in its schema def that it auto 
            # applies to 'TestAutoAppliedToAPI'
            "TestSingleApplyAPI"])
        self.assertTrue(prim.HasAPI(self.NestedAutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.AutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "foo"))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "builtin"))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "autoFoo"))
        self.assertTrue(prim.HasAPI(self.SingleApplyAPIType))

        # Prim's authored type is empty and its authored API schemas is just the
        # single authored schema.
        self.assertEqual(prim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedAutoAppliedToAPI"])

        # Prim's built-in properties come from all of the applied API schemas.
        self.assertEqual(prim.GetPropertyNames(), [
            "multi:autoFoo:bool_attr",
            "multi:autoFoo:relationship",
            "multi:autoFoo:token_attr",
            "multi:builtin:bool_attr",
            "multi:builtin:relationship",
            "multi:builtin:token_attr",
            "multi:foo:bool_attr",
            "multi:foo:relationship",
            "multi:foo:token_attr",
            "single:bool_attr",
            "single:relationship",
            "single:token_attr",
            "testAttr",
            "testRel"])

        # Define a prim with type name TestNestedAutoAppliedToAPIAppliedToPrim.
        # TestNestedAutoAppliedToAPI is defined to auto apply to this prim type
        # and brings with it all of the API schemas that are built-in to it and
        # auto applied to it.
        prim = stage.DefinePrim("/Prim3", 
                                "TestNestedAutoAppliedToAPIAppliedToPrim")
        self.assertEqual(prim.GetTypeName(), 
                         'TestNestedAutoAppliedToAPIAppliedToPrim')
        self.assertEqual(prim.GetAppliedSchemas(), [
            # 'TestNestedAutoAppliedToAPI' defines in its schema def that it 
            # auto applies to 'TestNestedAutoAppliedToAPIAppliedToPrim'
            "TestNestedAutoAppliedToAPI",
            # Built-in API of 'TestNestedAutoAppliedToAPI'
            "TestMultiApplyAPI:foo",
            # 'TestAutoAppliedToAPI' defines in its schema def that it auto 
            # applies to 'TestNestedAutoAppliedToAPI'
            "TestAutoAppliedToAPI",
            # Built-in API of 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:builtin",
            # Defined in plugin metadata that 'TestMultiApplyAPI:autoFoo' auto
            # applies to 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:autoFoo",
            # 'TestSingleApplyAPI' defines in its schema def that it auto 
            # applies to 'TestAutoAppliedToAPI'
            "TestSingleApplyAPI"])

        # Prim's authored applied API schemas is empty as the API schemas are
        # part of the type (through auto apply).
        self.assertEqual(prim.GetPrimTypeInfo().GetTypeName(), 
                         'TestNestedAutoAppliedToAPIAppliedToPrim')
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        # Prim's built-in properties come from all of the applied API schemas.
        self.assertTrue(prim.HasAPI(self.NestedAutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.AutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "foo"))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "builtin"))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "autoFoo"))
        self.assertTrue(prim.HasAPI(self.SingleApplyAPIType))
        self.assertEqual(prim.GetPropertyNames(), [
            "multi:autoFoo:bool_attr",
            "multi:autoFoo:relationship",
            "multi:autoFoo:token_attr",
            "multi:builtin:bool_attr",
            "multi:builtin:relationship",
            "multi:builtin:token_attr",
            "multi:foo:bool_attr",
            "multi:foo:relationship",
            "multi:foo:token_attr",
            "single:bool_attr",
            "single:relationship",
            "single:token_attr",
            "testAttr",
            "testRel"])

    @unittest.skipIf(not Tf.GetEnvSetting('USD_DISABLE_AUTO_APPLY_API_SCHEMAS'),
                    "Auto apply API schemas are NOT disabled")
    def test_APISchemasAutoAppliedToAPISchemas_AutoApplyDisabled(self):
        """
        Tests the behaviors of API schemas that are auto applied to other API
        schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Define a prim with an empty type name and apply TestAutoAppliedToAPI.
        # TestAutoAppliedAPI includes other API schemas through a combination of
        # built-in APIs and auto applied APIs. The auto applied schemas are
        # disabled in the this test case.
        prim = stage.DefinePrim("/Prim")
        prim.ApplyAPI(self.AutoAppliedToAPIType)
        self.assertEqual(prim.GetTypeName(), '')
        self.assertEqual(prim.GetAppliedSchemas(), [
            # Authored applied API
            "TestAutoAppliedToAPI",
            # Built-in API of 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:builtin"
            # 'TestMultiApplyAPI:autoFoo' and 'TestSingleApplyAPI' would be 
            # auto applied so they do not show up when auto apply is disabled
            ])
        self.assertTrue(prim.HasAPI(self.AutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "builtin"))
        self.assertFalse(prim.HasAPI(self.MultiApplyAPIType, "autoFoo"))
        self.assertFalse(prim.HasAPI(self.SingleApplyAPIType))

        # Prim's authored type is empty and its authored API schemas is just the
        # single authored schema.
        self.assertEqual(prim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestAutoAppliedToAPI"])

        # Prim's built-in properties come from all of the applied API schemas.
        self.assertEqual(prim.GetPropertyNames(), [
            "multi:builtin:bool_attr",
            "multi:builtin:relationship",
            "multi:builtin:token_attr",
            "testAttr",
            "testRel"])

        # Define a prim with an empty type name and apply 
        # TestNestedAutoAppliedToAPI.
        # TestAutoAppliedAPI auto applies to TestNestedAutoAppliedToAPI but
        # auto apply is disabled in this test case.
        prim = stage.DefinePrim("/Prim2")
        prim.ApplyAPI(self.NestedAutoAppliedToAPIType)
        self.assertEqual(prim.GetTypeName(), '')
        self.assertEqual(prim.GetAppliedSchemas(), [
            # Authored applied API
            "TestNestedAutoAppliedToAPI",
            # Built-in API of 'TestNestedAutoAppliedToAPI'
            "TestMultiApplyAPI:foo",
            # 'TestAutoAppliedToAPI' would be auto applied it doesn't show up 
            # when auto apply is disabled, nor do any of the API schemas that
            # would be included by it.
            ])
        self.assertTrue(prim.HasAPI(self.NestedAutoAppliedToAPIType))
        self.assertFalse(prim.HasAPI(self.AutoAppliedToAPIType))
        self.assertTrue(prim.HasAPI(self.MultiApplyAPIType, "foo"))
        self.assertFalse(prim.HasAPI(self.MultiApplyAPIType, "builtin"))
        self.assertFalse(prim.HasAPI(self.MultiApplyAPIType, "autoFoo"))
        self.assertFalse(prim.HasAPI(self.SingleApplyAPIType))

        # Prim's authored type is empty and its authored API schemas is just the
        # single authored schema.
        self.assertEqual(prim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedAutoAppliedToAPI"])

        # Prim's built-in properties come from all of the applied API schemas.
        self.assertEqual(prim.GetPropertyNames(), [
            "multi:foo:bool_attr",
            "multi:foo:relationship",
            "multi:foo:token_attr"])

        # Define a prim with type name TestNestedAutoAppliedToAPIAppliedToPrim.
        # TestNestedAutoAppliedToAPI is defined to auto apply to this prim type
        # auto apply is disabled in this test case.
        prim = stage.DefinePrim("/Prim3", 
                                "TestNestedAutoAppliedToAPIAppliedToPrim")
        self.assertEqual(prim.GetTypeName(), 
                         'TestNestedAutoAppliedToAPIAppliedToPrim')
        # 'TestNestedAutoAppliedToAPI' would be auto applied so it doesn't show 
        # up when auto apply is disabled, nor do any of the API schemas that
        # would be included by it.
        self.assertEqual(prim.GetAppliedSchemas(), [])

        self.assertEqual(prim.GetPrimTypeInfo().GetTypeName(), 
                         'TestNestedAutoAppliedToAPIAppliedToPrim')
        self.assertEqual(prim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])
        self.assertEqual(prim.GetPropertyNames(), [])


if __name__ == "__main__":
    unittest.main()
