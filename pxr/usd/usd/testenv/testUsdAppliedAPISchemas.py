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
from pxr import Plug, Sdf, Usd, Vt, Tf, Gf

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
        cls.NestedInnerMultiApplyBaseAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedInnerMultiApplyBaseAPI")
        cls.NestedInnerMultiApplyDerivedAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedInnerMultiApplyDerivedAPI")
        cls.NestedOuterMultiApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedOuterMultiApplyAPI")
        cls.NestedMultiApplyInSingleApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedMultiApplyInSingleApplyAPI")
        cls.NestedCycle1APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedCycle1API")
        cls.NestedCycle2APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedCycle2API")
        cls.NestedCycle3APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedCycle3API")
        cls.NestedMultiApplyCycle1APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedMultiApplyCycle1API")
        cls.NestedMultiApplyCycle2APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedMultiApplyCycle2API")
        cls.NestedMultiApplyCycle3APIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedMultiApplyCycle3API")
        cls.AutoAppliedToAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestAutoAppliedToAPI")
        cls.NestedAutoAppliedToAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedAutoAppliedToAPI")
        cls.NestedAutoAppliedToAPIAppliedToPrimType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestNestedAutoAppliedToAPIAppliedToPrim")
        cls.PropertyOversOneAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversOneAPI")
        cls.PropertyOversTwoAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversTwoAPI")
        cls.PropertyOversThreeAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversThreeAPI")
        cls.PropertyOversFourAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversFourAPI")
        cls.PropertyOversMultiOneAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversMultiOneAPI")
        cls.PropertyOversMultiTwoAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversMultiTwoAPI")
        cls.PropertyOversMultiThreeAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversMultiThreeAPI")
        cls.PropertyOversAutoApplyAPIType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversAutoApplyAPI")
        cls.PropertyOversTypedPrimBasePrimType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversTypedPrimBase")
        cls.PropertyOversTypedPrimDerivedPrimType = \
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestPropertyOversTypedPrimDerived")

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
            "single:bool_attr", "single:relationship", "single:token_attr"])
        self.assertEqual(singleApplyAPIDef.GetDocumentation(),
            "Test single apply API schema")

        # Find the prim definition for the test multi apply schema. It has
        # some properties defined. Note that the properties in the multi apply
        # definition are not prefixed yet.
        multiApplyAPIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestMultiApplyAPI")
        self.assertTrue(multiApplyAPIDef)
        self.assertEqual(multiApplyAPIDef.GetAppliedAPISchemas(), 
            ["TestMultiApplyAPI:__INSTANCE_NAME__"])
        self.assertEqual(multiApplyAPIDef.GetPropertyNames(), [
            "multi:__INSTANCE_NAME__:bool_attr", 
            "multi:__INSTANCE_NAME__:relationship",
            "multi:__INSTANCE_NAME__:token_attr"])
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
            multiApplyAPIDef.GetSchemaAttributeSpec(
                "multi:__INSTANCE_NAME__:token_attr"))
        self.assertEqual(multiTokenAttr.default, "foo")
        self.assertEqual(multiTokenAttr.typeName.cppTypeName, "TfToken")

        multiRelationship = primDef.GetSchemaRelationshipSpec(
            "multi:builtin:relationship")
        self.assertTrue(multiRelationship)
        self.assertEqual(multiRelationship, 
            multiApplyAPIDef.GetSchemaRelationshipSpec(
                "multi:__INSTANCE_NAME__:relationship"))

        # Verify the case where the concrete type overrides a property from 
        # one of its applied API schemas. In this case the property spec from
        # the concrete prim is returned instead of the property spec from the
        # API schema.
        multiBoolAttr = primDef.GetSchemaAttributeSpec(
            "multi:builtin:bool_attr")
        apiBoolAttr = multiApplyAPIDef.GetSchemaAttributeSpec(
            "multi:__INSTANCE_NAME__:bool_attr")
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
                          "TestSingleApplyAPI",
                          "TestMultiApplyAPI:autoFoo"])
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

    def test_NestedSingleApplyAPISchemas(self):
        """
        Tests the application of single apply API schemas that have nested 
        built-in API schemas
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

    def test_NestedMultiApplyAPISchemas(self):
        """
        Tests the application of multiple apply API schemas that have nested 
        built-in API schemas
        """
        stage = Usd.Stage.CreateInMemory()

        # Simple helper for testing that a prim has expected attributes that 
        # resolve to expected values.
        def _VerifyAttrValues(prim, expectedAttrValues):
            values = {name : prim.GetAttribute(name).Get() 
                         for name in expectedAttrValues.keys()}
            self.assertEqual(values, expectedAttrValues)

        # Add a prim with no type and apply the 
        # TestNestedInnerMultiApplyDerivedAPI using the instance "foo".
        innerMultiPrim = stage.DefinePrim("/InnerMulti")
        innerMultiPrim.ApplyAPI(self.NestedInnerMultiApplyDerivedAPIType, "foo")

        # The authored applied API schemas for the prim is only the "foo" 
        # instance of the applied TestNestedInnerMultiApplyDerivedAPI.
        self.assertEqual(innerMultiPrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(innerMultiPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedInnerMultiApplyDerivedAPI:foo"])

        # The composed applied API schemas however also contain the 
        # the "foo" instance of TestNestedInnerMultiApplyBaseAPI as it is a
        # built-in API of TestNestedInnerMultiApplyDerivedAPI 
        expectedAPISchemas = [
            "TestNestedInnerMultiApplyDerivedAPI:foo",
            "TestNestedInnerMultiApplyBaseAPI:foo"]
        self.assertEqual(innerMultiPrim.GetAppliedSchemas(), 
                         expectedAPISchemas)
        # The prim "has" all these built-in APIs as well.
        self.assertTrue(innerMultiPrim.HasAPI(self.NestedInnerMultiApplyDerivedAPIType))
        self.assertTrue(innerMultiPrim.HasAPI(self.NestedInnerMultiApplyBaseAPIType))
        self.assertTrue(innerMultiPrim.HasAPI(self.NestedInnerMultiApplyDerivedAPIType, "foo"))
        self.assertTrue(innerMultiPrim.HasAPI(self.NestedInnerMultiApplyBaseAPIType, "foo"))

        # Properties come from all composed built-in APIs
        expectedPropNames = [
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo
            "innerMulti:foo:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo
            "innerMulti:foo:int_attr",
            "innerMulti:foo:relationship",
            "innerMulti:foo:token_attr"]
        self.assertEqual(innerMultiPrim.GetPropertyNames(), expectedPropNames)

        # Verify that the attribute fallback values come from the API schemas
        # that define them. The attribute "innerMulti:foo:token_attr" is defined
        # in TestNestedInnerMultiApplyDerivedAPI and overrides the attr fallback
        # value defined in TestNestedInnerMultiApplyBaseAPI
        expectedAttrValues = {
            "innerMulti:foo:derived:int_attr" : 4,
            "innerMulti:foo:int_attr" : 3,
            "innerMulti:foo:token_attr" : "inner_derived"}
        _VerifyAttrValues(innerMultiPrim, expectedAttrValues)

        # Apply the TestNestedInnerMultiApplyDerivedAPI to the same prim again,
        # now with the instance "bar"
        innerMultiPrim.ApplyAPI(self.NestedInnerMultiApplyDerivedAPIType, "bar")
        self.assertEqual(innerMultiPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestNestedInnerMultiApplyDerivedAPI:foo",
                          "TestNestedInnerMultiApplyDerivedAPI:bar"])

        # Now the same API schemas included "foo" are also included for "bar"
        expectedAPISchemas = [
            "TestNestedInnerMultiApplyDerivedAPI:foo",
            "TestNestedInnerMultiApplyBaseAPI:foo",
            "TestNestedInnerMultiApplyDerivedAPI:bar",
            "TestNestedInnerMultiApplyBaseAPI:bar"]
        self.assertEqual(innerMultiPrim.GetAppliedSchemas(), 
                         expectedAPISchemas)

        # There are now also "bar" instances of all the same properties.
        expectedPropNames = [
            "innerMulti:bar:derived:int_attr",
            "innerMulti:bar:int_attr",
            "innerMulti:bar:relationship",
            "innerMulti:bar:token_attr",
            "innerMulti:foo:derived:int_attr",
            "innerMulti:foo:int_attr",
            "innerMulti:foo:relationship",
            "innerMulti:foo:token_attr"]
        self.assertEqual(innerMultiPrim.GetPropertyNames(), expectedPropNames)

        # And the "bar" instances of the attributes have the same fallback 
        # values.
        expectedAttrValues = {
            "innerMulti:foo:derived:int_attr" : 4,
            "innerMulti:foo:int_attr" : 3,
            "innerMulti:foo:token_attr" : "inner_derived",
            "innerMulti:bar:derived:int_attr" : 4,
            "innerMulti:bar:int_attr" : 3,
            "innerMulti:bar:token_attr" : "inner_derived"}
        _VerifyAttrValues(innerMultiPrim, expectedAttrValues)

        # Get the prim definition for the API schema and verify its applied
        # API schemas and properties are template versions of the proeperties.
        innerMultiApplyAPIDef = \
            Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
                "TestNestedInnerMultiApplyDerivedAPI")
        self.assertTrue(innerMultiApplyAPIDef)

        expectedAPISchemas = [
            "TestNestedInnerMultiApplyDerivedAPI:__INSTANCE_NAME__",
            "TestNestedInnerMultiApplyBaseAPI:__INSTANCE_NAME__"]
        self.assertEqual(innerMultiApplyAPIDef.GetAppliedAPISchemas(),
                         expectedAPISchemas)

        expectedPropNames = [
            "innerMulti:__INSTANCE_NAME__:derived:int_attr",
            "innerMulti:__INSTANCE_NAME__:int_attr",
            "innerMulti:__INSTANCE_NAME__:relationship",
            "innerMulti:__INSTANCE_NAME__:token_attr"]
        self.assertEqual(sorted(innerMultiApplyAPIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(innerMultiApplyAPIDef.GetDocumentation(),
            "Test nested multi apply API schema: inner schema derived")

        # Add a prim with no type and apply the TestNestedOuterMultiApplyAPI 
        # with the instance "foo".
        outerMultiPrim = stage.DefinePrim("/OuterMulti")
        outerMultiPrim.ApplyAPI(self.NestedOuterMultiApplyAPIType, "foo")

        # The authored applied API schemas for the prim is only the applied
        # TestNestedOuterMultiApplyAPI:foo.
        self.assertEqual(outerMultiPrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(outerMultiPrim.GetPrimTypeInfo().GetAppliedAPISchemas(),
                         ["TestNestedOuterMultiApplyAPI:foo"])

        # TestNestedOuterMultiApplyAPI's definition includes 
        # TestNestedInnerMultiApplyDerivedAPI:builtin and 
        # TestNestedInnerMultiApplyDerivedAPI:outerMulti. Thus, the composed 
        # applied API schemas also contain both a "foo:builtin" and a 
        # "foo:outerMulti" instance of TestNestedInnerMultiApplyDerivedAPI, 
        # which in turn include "foo:builtin" and "foo:outerMulti" instances of 
        # TestNestedInnerMultiApplyBaseAPI. Since TestNestedOuterMultiApplyAPI
        # also includes TestNestedInnerMultiApplyBaseAPI a "foo" instance of it
        # is included as well.
        self.assertEqual(outerMultiPrim.GetTypeName(), '')
        expectedAPISchemas = [
            "TestNestedOuterMultiApplyAPI:foo",
            "TestNestedInnerMultiApplyDerivedAPI:foo:builtin",
            "TestNestedInnerMultiApplyBaseAPI:foo:builtin",
            "TestNestedInnerMultiApplyDerivedAPI:foo:outerMulti",
            "TestNestedInnerMultiApplyBaseAPI:foo:outerMulti",
            "TestNestedInnerMultiApplyBaseAPI:foo"]
        self.assertEqual(outerMultiPrim.GetAppliedSchemas(),
                         expectedAPISchemas)

        # Properties come from all composed built-in APIs
        expectedPropNames = sorted([
            # Properties from TestNestedOuterMultiApplyAPI:foo
            "outerMulti:foo:int_attr",
            "outerMulti:foo:relationship",
            "outerMulti:foo:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:builtin
            "innerMulti:foo:builtin:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:builtin
            "innerMulti:foo:builtin:int_attr",
            "innerMulti:foo:builtin:relationship",
            "innerMulti:foo:builtin:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:int_attr",
            "innerMulti:foo:outerMulti:relationship",
            "innerMulti:foo:outerMulti:token_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo
            "innerMulti:foo:int_attr",
            "innerMulti:foo:relationship",
            "innerMulti:foo:token_attr"])
        self.assertEqual(outerMultiPrim.GetPropertyNames(), expectedPropNames)

        # Verify that the attribute fallback values come from the API schemas
        # that define them. The "innerMulti:foo:XXX:token_attr" values from
        # from TestNestedInnerMultiApplyDerivedAPI override the values from
        # TestNestedInnerMultiApplyBaseAPI. innerMulti:foo:token_attr uses
        # the BaseAPI value since this instance is from the BaseAPI being 
        # included on its own.
        expectedAttrValues = {
            # Properties from TestNestedOuterMultiApplyAPI:foo
            "outerMulti:foo:int_attr" : 5,
            "outerMulti:foo:token_attr" : "outer",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:builtin
            "innerMulti:foo:builtin:derived:int_attr" : 4,
            "innerMulti:foo:builtin:token_attr" : "inner_derived",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:builtin
            "innerMulti:foo:builtin:int_attr" : 3,
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:derived:int_attr" : 4,
            "innerMulti:foo:outerMulti:token_attr" : "inner_derived",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:int_attr" : 3,
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo
            "innerMulti:foo:int_attr" : 3,
            "innerMulti:foo:token_attr" : "inner_base"}
        _VerifyAttrValues(outerMultiPrim, expectedAttrValues)

        # Get the prim definition for the API schema and verify its applied
        # API schemas and properties match what was imparted on the prim.
        outerMultiApplyAPIDef = \
            Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
                "TestNestedOuterMultiApplyAPI")
        self.assertTrue(outerMultiApplyAPIDef)

        # Note that the __INSTANCE_NAME__ is alway directly after the API
        # schema name even when it is included as an encapsulated subinstance.
        expectedAPISchemas = [
            "TestNestedOuterMultiApplyAPI:__INSTANCE_NAME__",
            "TestNestedInnerMultiApplyDerivedAPI:__INSTANCE_NAME__:builtin",
            "TestNestedInnerMultiApplyBaseAPI:__INSTANCE_NAME__:builtin",
            "TestNestedInnerMultiApplyDerivedAPI:__INSTANCE_NAME__:outerMulti",
            "TestNestedInnerMultiApplyBaseAPI:__INSTANCE_NAME__:outerMulti",
            "TestNestedInnerMultiApplyBaseAPI:__INSTANCE_NAME__"]
        self.assertEqual(outerMultiApplyAPIDef.GetAppliedAPISchemas(),
                         expectedAPISchemas)

        expectedPropNames = sorted([
            # Properties from TestNestedOuterMultiApplyAPI
            "outerMulti:__INSTANCE_NAME__:int_attr",
            "outerMulti:__INSTANCE_NAME__:relationship",
            "outerMulti:__INSTANCE_NAME__:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:builtin
            "innerMulti:__INSTANCE_NAME__:builtin:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:builtin
            "innerMulti:__INSTANCE_NAME__:builtin:int_attr",
            "innerMulti:__INSTANCE_NAME__:builtin:relationship",
            "innerMulti:__INSTANCE_NAME__:builtin:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:outerMulti
            "innerMulti:__INSTANCE_NAME__:outerMulti:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:outerMulti
            "innerMulti:__INSTANCE_NAME__:outerMulti:int_attr",
            "innerMulti:__INSTANCE_NAME__:outerMulti:relationship",
            "innerMulti:__INSTANCE_NAME__:outerMulti:token_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI
            "innerMulti:__INSTANCE_NAME__:int_attr",
            "innerMulti:__INSTANCE_NAME__:relationship",
            "innerMulti:__INSTANCE_NAME__:token_attr"])
        self.assertEqual(sorted(outerMultiApplyAPIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(outerMultiApplyAPIDef.GetDocumentation(),
            "Test nested multi apply API schema: outer schema")

        # Add a prim with no type and apply the 
        # TestNestedMultiApplyInSingleApplyAPI.
        singleApplyPrim = stage.DefinePrim("/SingleApply")
        singleApplyPrim.ApplyAPI(self.NestedMultiApplyInSingleApplyAPIType)

        # The authored applied API schemas for the prim is only the applied
        # TestNestedMultiApplyInSingleApplyAPI.
        self.assertEqual(singleApplyPrim.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(singleApplyPrim.GetPrimTypeInfo().GetAppliedAPISchemas(),
                         ["TestNestedMultiApplyInSingleApplyAPI"])

        # TestNestedMultiApplyInSingleApplyAPI includes 
        # TestNestedOuterMultiApplyAPI:foo and 
        # TestNestedInnerMultiApplyDerivedAPI:bar so "foo" and "bar" instances
        # of these multi apply schemas are fully expanded into the composed API
        # schemas udner TestNestedMultiApplyInSingleApplyAPI.
        self.assertEqual(singleApplyPrim.GetTypeName(), '')
        expectedAPISchemas = [
            "TestNestedMultiApplyInSingleApplyAPI",
            # Expanded from TestNestedOuterMultiApplyAPI:foo
            "TestNestedOuterMultiApplyAPI:foo",
            "TestNestedInnerMultiApplyDerivedAPI:foo:builtin",
            "TestNestedInnerMultiApplyBaseAPI:foo:builtin",
            "TestNestedInnerMultiApplyDerivedAPI:foo:outerMulti",
            "TestNestedInnerMultiApplyBaseAPI:foo:outerMulti",
            "TestNestedInnerMultiApplyBaseAPI:foo",
            # Expanded from TestNestedInnerMultiApplyDerivedAPI:bar
            "TestNestedInnerMultiApplyDerivedAPI:bar",
            "TestNestedInnerMultiApplyBaseAPI:bar"]
        self.assertEqual(singleApplyPrim.GetAppliedSchemas(),
                         expectedAPISchemas)

        # Properties come from all composed built-in APIs
        expectedPropNames = sorted([
            # Properties from TestNestedMultiApplyInSingleApplyAPI
            "int_attr",
            # Properties from TestNestedOuterMultiApplyAPI:foo
            "outerMulti:foo:int_attr",
            "outerMulti:foo:relationship",
            "outerMulti:foo:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:builtin
            "innerMulti:foo:builtin:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:builtin
            "innerMulti:foo:builtin:int_attr",
            "innerMulti:foo:builtin:relationship",
            "innerMulti:foo:builtin:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:int_attr",
            "innerMulti:foo:outerMulti:relationship",
            "innerMulti:foo:outerMulti:token_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo
            "innerMulti:foo:int_attr",
            "innerMulti:foo:relationship",
            "innerMulti:foo:token_attr",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:bar
            "innerMulti:bar:derived:int_attr",
            # Properties from TestNestedInnerMultiApplyBaseAPI:bar
            "innerMulti:bar:int_attr",
            "innerMulti:bar:relationship",
            "innerMulti:bar:token_attr"])
        self.assertEqual(singleApplyPrim.GetPropertyNames(), expectedPropNames)

        # Verify that the attribute fallback values come from the API schemas
        # that define them.
        expectedAttrValues = {
            # Property only defined in TestNestedMultiApplyInSingleApplyAPI
            "int_attr" : 10,
            # Property from TestNestedInnerMultiApplyDerivedAPI:foo:builtin
            # overridden in TestNestedMultiApplyInSingleApplyAPI
            "innerMulti:foo:builtin:derived:int_attr" : 20,
            # Property from TestNestedInnerMultiApplyBaseAPI:bar overridden in
            # TestNestedMultiApplyInSingleApplyAPI
            "innerMulti:bar:int_attr" : 30,
            # Properties from TestNestedOuterMultiApplyAPI:foo
            "outerMulti:foo:int_attr" : 5,
            "outerMulti:foo:token_attr" : "outer",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:builtin
            "innerMulti:foo:builtin:token_attr" : "inner_derived",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:builtin
            "innerMulti:foo:builtin:int_attr" : 3,
            # Properties from TestNestedInnerMultiApplyDerivedAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:derived:int_attr" : 4,
            "innerMulti:foo:outerMulti:token_attr" : "inner_derived",
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo:outerMulti
            "innerMulti:foo:outerMulti:int_attr" : 3,
            # Properties from TestNestedInnerMultiApplyBaseAPI:foo
            "innerMulti:foo:int_attr" : 3,
            "innerMulti:foo:token_attr" : "inner_base",
            # Properties from TestNestedInnerMultiApplyDerivedAPI:bar
            "innerMulti:bar:derived:int_attr" : 4,
            # Properties from TestNestedInnerMultiApplyBaseAPI:bar
            "innerMulti:bar:token_attr" : "inner_derived"}
        _VerifyAttrValues(singleApplyPrim, expectedAttrValues)

        # Get the prim definition for the API schema and verify its applied
        # API schemas and properties match what was imparted on the prim. This
        # is an exact match because this a single apply API schema that contains
        # specific instances of the multi apply API schemas.
        singleApplyAPIDef = \
            Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
                "TestNestedMultiApplyInSingleApplyAPI")
        self.assertTrue(singleApplyAPIDef)
        self.assertEqual(singleApplyAPIDef.GetAppliedAPISchemas(),
                         expectedAPISchemas)
        self.assertEqual(sorted(singleApplyAPIDef.GetPropertyNames()),
                         expectedPropNames)
        self.assertEqual(singleApplyAPIDef.GetDocumentation(),
            "Test single apply API with builtin nested multi apply API schema "
            "instances")

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

    def test_NestedMultiApplyCycleAPISchema(self):
        """
        Tests the handling of inclusion cycles that are particular to how 
        built-in multiple apply schemas are processed.
        """
        stage = Usd.Stage.CreateInMemory()

        # Test behavior when nested API schema form a cycle. In this example we
        # have two types of cycles. 
        # 
        # The first is an "inheritance" cycle where 
        # TestNestedMultiApplyCycle1API includes TestNestedMultiApplyCycle3API
        # directly which includes TestNestedMultiApplyCycle2API which comes
        # and includes TestNestedMultiApplyCycle1API again. Since these are
        # all "inheritance" style built-ins, these will all use the same 
        # instance name and can be handled gracefully, like we do with single
        # apply built-in cycles, by always skipping duplicate API schemas when
        # expanding.
        #
        # The second type of cycle comes from encapsulated sub-instance built-in
        # API schemas, where TestNestedMultiApplyCycle1API includes a "cycle1"
        # sub-instance of TestNestedMultiApplyCycle2API which includes a 
        # "cycle2" sub-instance of TestNestedMultiApplyCycle3API which then 
        # includes a "cycle3" sub-instance of TestNestedMultiApplyCycle1API. 
        # Because of the way instance names nest (e.g. applying Cycle1:foo
        # will include Cycle2:foo:cycle1 will include Cycle3:foo:cycle1:cycle2
        # will include Cycle1:foo:cycle1:cycle2:cycle3 and so on) these kinds of
        # cycles will become infinite as every instance will be unique as we 
        # expand. These types of cycles must be broken by making sure that we 
        # don't add a built-in API if it is the same schema type as one of its
        # direct ancestors during the depth first expansion of included API 
        # schemas.
        nestedCyclePrim1 = stage.DefinePrim("/Cycle1")
        nestedCyclePrim2 = stage.DefinePrim("/Cycle2")
        nestedCyclePrim3 = stage.DefinePrim("/Cycle3")
        nestedCyclePrim1.ApplyAPI(self.NestedMultiApplyCycle1APIType, "foo")
        nestedCyclePrim2.ApplyAPI(self.NestedMultiApplyCycle2APIType, "foo")
        nestedCyclePrim3.ApplyAPI(self.NestedMultiApplyCycle3APIType, "foo")

        # For each prim the authored applied API schemas for the prim are still
        # only the single API that was applied.
        self.assertEqual(nestedCyclePrim1.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(
            nestedCyclePrim1.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestNestedMultiApplyCycle1API:foo"])
        self.assertEqual(nestedCyclePrim2.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(
            nestedCyclePrim2.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestNestedMultiApplyCycle2API:foo"])
        self.assertEqual(nestedCyclePrim3.GetPrimTypeInfo().GetTypeName(), '')
        self.assertEqual(
            nestedCyclePrim3.GetPrimTypeInfo().GetAppliedAPISchemas(), 
            ["TestNestedMultiApplyCycle3API:foo"])

        # The composed applied API schemas include all the possible instances
        # of all three API schemas that can be added before the cycle detection
        # stops the depth first traversal of the built-ins. The commented out
        # entries represent the schemas that trip the cycle detection and 
        # therefore were not added when expanded halting their branch of API 
        # schema expansion.
        self.assertEqual(nestedCyclePrim1.GetTypeName(), '')
        self.assertEqual(nestedCyclePrim1.GetAppliedSchemas(), [
            "TestNestedMultiApplyCycle1API:foo",
                "TestNestedMultiApplyCycle2API:foo:cycle1",
                    "TestNestedMultiApplyCycle3API:foo:cycle1:cycle2",
                        # "TestNestedMultiApplyCycle1API:foo:cycle1:cycle2:cycle3",
                        # "TestNestedMultiApplyCycle2API:foo:cycle1:cycle2",
                    # "TestNestedMultiApplyCycle1API:foo:cycle1"
                "TestNestedMultiApplyCycle3API:foo",
                    # "TestNestedMultiApplyCycle1API:foo:cycle3",
                    "TestNestedMultiApplyCycle2API:foo",
                        # "TestNestedMultiApplyCycle3API:foo:cycle2",
                        # "TestNestedMultiApplyCycle1API:foo"
            ])
        self.assertEqual(nestedCyclePrim2.GetTypeName(), '')
        self.assertEqual(nestedCyclePrim2.GetAppliedSchemas(), [
            "TestNestedMultiApplyCycle2API:foo",
                "TestNestedMultiApplyCycle3API:foo:cycle2",
                    "TestNestedMultiApplyCycle1API:foo:cycle2:cycle3",
                        # "TestNestedMultiApplyCycle2API:foo:cycle2:cycle3:cycle1",
                        # "TestNestedMultiApplyCycle3API:foo:cycle2:cycle3",
                    # "TestNestedMultiApplyCycle2API:foo:cycle2"
                "TestNestedMultiApplyCycle1API:foo",
                    # "TestNestedMultiApplyCycle2API:foo:cycle1",
                    "TestNestedMultiApplyCycle3API:foo",
                        # "TestNestedMultiApplyCycle1API:foo:cycle3",
                        # "TestNestedMultiApplyCycle2API:foo"
            ])
        self.assertEqual(nestedCyclePrim3.GetTypeName(), '')
        self.assertEqual(nestedCyclePrim3.GetAppliedSchemas(), [
            "TestNestedMultiApplyCycle3API:foo",
                "TestNestedMultiApplyCycle1API:foo:cycle3",
                    "TestNestedMultiApplyCycle2API:foo:cycle3:cycle1",
                        # "TestNestedMultiApplyCycle3API:foo:cycle3:cycle1:cycle2",
                        # "TestNestedMultiApplyCycle1API:foo:cycle3:cycle1",
                    # "TestNestedMultiApplyCycle3API:foo:cycle3"
                "TestNestedMultiApplyCycle2API:foo",
                    # "TestNestedMultiApplyCycle3API:foo:cycle2",
                    "TestNestedMultiApplyCycle1API:foo",
                        # "TestNestedMultiApplyCycle2API:foo:cycle1",
                        # "TestNestedMultiApplyCycle3API:foo"
            ])

        # Each of the three API schemas provides a "token_attr" so each of 
        # prims has the prefixed "token_attr" for the API schemas that managed
        # to be included for each one.
        expectedPropNames = [
            "cycle1:foo:token_attr",
            "cycle2:foo:cycle1:token_attr",
            "cycle2:foo:token_attr",
            "cycle3:foo:cycle1:cycle2:token_attr",
            "cycle3:foo:token_attr"
        ]
        self.assertEqual(nestedCyclePrim1.GetPropertyNames(), expectedPropNames)
        expectedPropNames = [
            "cycle1:foo:cycle2:cycle3:token_attr",
            "cycle1:foo:token_attr",
            "cycle2:foo:token_attr",
            "cycle3:foo:cycle2:token_attr",
            "cycle3:foo:token_attr"
        ]
        self.assertEqual(nestedCyclePrim2.GetPropertyNames(), expectedPropNames)
        expectedPropNames = [
            "cycle1:foo:cycle3:token_attr",
            "cycle1:foo:token_attr",
            "cycle2:foo:cycle3:cycle1:token_attr",
            "cycle2:foo:token_attr",
            "cycle3:foo:token_attr"
        ]
        self.assertEqual(nestedCyclePrim3.GetPropertyNames(), expectedPropNames)

        # Get the prim definitions for each of these API schemas and verify its
        # applied API schemas and properties match what was imparted on the
        # prims.
        cycle1APIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestNestedMultiApplyCycle1API")
        self.assertTrue(cycle1APIDef)
        self.assertEqual(cycle1APIDef.GetAppliedAPISchemas(),
            ["TestNestedMultiApplyCycle1API:__INSTANCE_NAME__",
             "TestNestedMultiApplyCycle2API:__INSTANCE_NAME__:cycle1",
             "TestNestedMultiApplyCycle3API:__INSTANCE_NAME__:cycle1:cycle2",
             "TestNestedMultiApplyCycle3API:__INSTANCE_NAME__",
             "TestNestedMultiApplyCycle2API:__INSTANCE_NAME__"])
        self.assertEqual(sorted(cycle1APIDef.GetPropertyNames()),
            ["cycle1:__INSTANCE_NAME__:token_attr",
             "cycle2:__INSTANCE_NAME__:cycle1:token_attr",
             "cycle2:__INSTANCE_NAME__:token_attr",
             "cycle3:__INSTANCE_NAME__:cycle1:cycle2:token_attr",
             "cycle3:__INSTANCE_NAME__:token_attr"])

        cycle2APIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestNestedMultiApplyCycle2API")
        self.assertTrue(cycle2APIDef)
        self.assertEqual(cycle2APIDef.GetAppliedAPISchemas(),
            ["TestNestedMultiApplyCycle2API:__INSTANCE_NAME__",
             "TestNestedMultiApplyCycle3API:__INSTANCE_NAME__:cycle2",
             "TestNestedMultiApplyCycle1API:__INSTANCE_NAME__:cycle2:cycle3",
             "TestNestedMultiApplyCycle1API:__INSTANCE_NAME__",
             "TestNestedMultiApplyCycle3API:__INSTANCE_NAME__"])
        self.assertEqual(sorted(cycle2APIDef.GetPropertyNames()),
            ["cycle1:__INSTANCE_NAME__:cycle2:cycle3:token_attr",
             "cycle1:__INSTANCE_NAME__:token_attr",
             "cycle2:__INSTANCE_NAME__:token_attr",
             "cycle3:__INSTANCE_NAME__:cycle2:token_attr",
             "cycle3:__INSTANCE_NAME__:token_attr"])

        cycle3APIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestNestedMultiApplyCycle3API")
        self.assertTrue(cycle3APIDef)
        self.assertEqual(cycle3APIDef.GetAppliedAPISchemas(),
            ["TestNestedMultiApplyCycle3API:__INSTANCE_NAME__",
             "TestNestedMultiApplyCycle1API:__INSTANCE_NAME__:cycle3",
             "TestNestedMultiApplyCycle2API:__INSTANCE_NAME__:cycle3:cycle1",
             "TestNestedMultiApplyCycle2API:__INSTANCE_NAME__",
             "TestNestedMultiApplyCycle1API:__INSTANCE_NAME__"])
        self.assertEqual(sorted(cycle3APIDef.GetPropertyNames()),
            ["cycle1:__INSTANCE_NAME__:cycle3:token_attr",
             "cycle1:__INSTANCE_NAME__:token_attr",
             "cycle2:__INSTANCE_NAME__:cycle3:cycle1:token_attr",
             "cycle2:__INSTANCE_NAME__:token_attr",
             "cycle3:__INSTANCE_NAME__:token_attr"])

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
            # 'TestSingleApplyAPI' defines in its schema def that it auto 
            # applies to 'TestAutoAppliedToAPI'
            "TestSingleApplyAPI",
            # Defined in plugin metadata that 'TestMultiApplyAPI:autoFoo' auto
            # applies to 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:autoFoo"])
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
            # 'TestSingleApplyAPI' defines in its schema def that it auto 
            # applies to 'TestAutoAppliedToAPI'
            "TestSingleApplyAPI",
            # Defined in plugin metadata that 'TestMultiApplyAPI:autoFoo' auto
            # applies to 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:autoFoo"])
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
            # 'TestSingleApplyAPI' defines in its schema def that it auto 
            # applies to 'TestAutoAppliedToAPI'
            "TestSingleApplyAPI",
            # Defined in plugin metadata that 'TestMultiApplyAPI:autoFoo' auto
            # applies to 'TestAutoAppliedToAPI'
            "TestMultiApplyAPI:autoFoo"])

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

    @unittest.skipIf(Tf.GetEnvSetting('USD_DISABLE_AUTO_APPLY_API_SCHEMAS'),
                    "Auto apply API schemas are disabled")
    def test_PropertyTypeConflicts(self):
        """
        Test the resolution of property type conflicts between prim type and 
        API schema prim definitions when API schemas are applied to prims.
        """
        stage = Usd.Stage.CreateInMemory()

        # Helper for verifying the attribute types and computed values of 
        # any arbitrary set of attributes on the prim. The arguments are the 
        # prim followed by any number of keyword arguments of the form:
        #   <attrName> = (<attrTypeNameStr>, <attrValue>, <docStr>)
        # For each keyword arg, it verifies that the prim has an attribute
        # named <attrName> whose typeName computes to <attrTypeNameStr> and 
        # computed valued computes to <attrValue>. Also verifies that the 
        # attribute's documentation matches <docStr>
        def _VerifyAttrTypes(prim, **kwargs):
            for name, (attrTypeNameStr, attrValue, docStr) in kwargs.items():
                attr = prim.GetAttribute(name)
                self.assertEqual(attr.GetTypeName(), attrTypeNameStr)
                self.assertEqual(attr.Get(), attrValue)
                self.assertEqual(attr.GetDocumentation(), docStr)

        # Helper for verifying that an arbitrary set of properties is or isn't 
        # a relationship. The arguments are the prim followed by any number of 
        # keyword arguments of the form:
        #   <propertyName> = (<isRelationship>, <docStr>)
        # For each keyword arg, it verifies that the prim has a property
        # named <propertyName> that is a relationship iff <isRelationship> is 
        # True. Also verifies that the property's documentation matches <docStr>
        def _VerifyIsRel(prim, **kwargs):
            for name, (isRelationship, docStr) in kwargs.items():
                prop = prim.GetProperty(name)
                self.assertTrue(prop)
                if isRelationship:
                    self.assertTrue(prim.GetRelationship(name))
                else:
                    self.assertFalse(prim.GetRelationship(name))
                self.assertEqual(prop.GetDocumentation(), docStr)

        # We've defined 3 API schema types for applying directly to a prim
        authoredOneAPIName = "TestPropTypeConflictAuthoredOneAPI"
        authoredTwoAPIName = "TestPropTypeConflictAuthoredTwoAPI" 
        nestedAPIName = "TestPropTypeConflictNestedAuthoredAPI"
        # We also typed prim type for this test case that includes a separate
        # built-in API schema type.
        conflictPrimTypeName = "TestPropTypeConflictsPrim"
        builtinAPIName = "TestPropTypeConflictBuiltinAPI" 

        # Each of the defined schema types above uses a single doc string for
        # all of it properties which we can use to help verify which schemas
        # the property definitions come from.
        authoredOneAPIDocStr = "From TestPropTypeConflictAuthoredOneAPI"
        authoredTwoAPIDocStr = "From TestPropTypeConflictAuthoredTwoAPI"
        nestedAPIDocStr = "From TestPropTypeConflictNestedAuthoredAPI"
        conflictPrimTypeDocStr = "From TestPropTypeConflictsPrim"
        builtinAPIDocStr = "From TestPropTypeConflictBuiltinAPI"

        # Test 1: Prim with no type name; apply the authoredOneAPI and 
        # authoredTwoAPI schemas in that strength order.
        untypedPrim = stage.DefinePrim("/UntypedPrim")
        untypedPrim.AddAppliedSchema(authoredOneAPIName)
        untypedPrim.AddAppliedSchema(authoredTwoAPIName)
        self.assertEqual(untypedPrim.GetAppliedSchemas(),
                         [authoredOneAPIName,
                          authoredTwoAPIName])

        # Only the authoredOneAPI defines attr1 and attr2
        # Only the authoredTwoAPI defines attr3 and attr4
        # Both define attr5 and attr6, so authoredOneAPI's version of those 
        # attributes are used since it's stronger.
        _VerifyAttrTypes(untypedPrim,
            attr1 = ("int",     0,                  authoredOneAPIDocStr),
            attr2 = ("double",  0.0,                authoredOneAPIDocStr),
            attr3 = ("int",     10,                 authoredTwoAPIDocStr),
            attr4 = ("string",  "foo",              authoredTwoAPIDocStr),
            attr5 = ("point3f", Gf.Vec3f(0, 0, 0),  authoredOneAPIDocStr),
            attr6 = ("int",     20,                 authoredOneAPIDocStr))
        # Both define rel1 and rel2, but since authoredOneAPI is stronger, it wins
        # and only rel2 is a relationship.
        _VerifyIsRel(untypedPrim,
            rel1 = (False, authoredOneAPIDocStr),
            rel2 = (True, authoredOneAPIDocStr))

        # Test 2: Prim with no type name; apply the authoredTwoAPI and builtAPI
        # schemas in that strength order. This is the reverse order of Test 1
        untypedPrim = stage.DefinePrim("/UntypedPrim2")
        untypedPrim.AddAppliedSchema(authoredTwoAPIName)
        untypedPrim.AddAppliedSchema(authoredOneAPIName)
        self.assertEqual(untypedPrim.GetAppliedSchemas(),
                         [authoredTwoAPIName,
                          authoredOneAPIName])

        # Only the authoredOneAPI defines attr1 and attr2 (same as Test 1)
        # Only the authoredTwoAPI defines attr3 and attr4 (same as Test 1)
        # Both define attr5 and attr6, but now authoredTwoAPI is stronger so
        # its version of those attributes are used. Note that this leads to a
        # different type and default value for attr5 and just a different 
        # default value for attr6.
        _VerifyAttrTypes(untypedPrim,
            attr1 = ("int",     0,                  authoredOneAPIDocStr),
            attr2 = ("double",  0.0,                authoredOneAPIDocStr),
            attr3 = ("int",     10,                 authoredTwoAPIDocStr),
            attr4 = ("string",  "foo",              authoredTwoAPIDocStr),
            attr5 = ("float3",  Gf.Vec3f(1, 2, 3),  authoredTwoAPIDocStr),
            attr6 = ("int",     10,                 authoredTwoAPIDocStr))
        # Both define rel1 and rel2, but now since authoredTwoAPI is stronger, 
        # it wins and only rel1 is a relationship (opposite of Test 1).
        _VerifyIsRel(untypedPrim,
            rel1 = (True, authoredTwoAPIDocStr),
            rel2 = (False, authoredTwoAPIDocStr))

        # Test 3: Prim with type name set to the conflictPrimType; no authored
        # applied API schemas. The conflictPrimType has a single built-in API 
        # schema builtinAPI.
        prim = stage.DefinePrim("/TypedPrim", conflictPrimTypeName)
        self.assertEqual(prim.GetAppliedSchemas(),
                         [builtinAPIName])

        # The conflictPrimType schema defines all of attr1-6 attributes EXCEPT
        # attr4. It also defines rel1 and rel2. So outside of attr4, the typed 
        # schema's version of these properties are used. 
        # For attr4, it is defined in the builtinAPI schema so that attribute's
        # opinion comes from the builtinAPI. Note that builtinAPI does define
        # attr2, rel1, and rel2 (all with different types than the 
        # conflictPrimType schema) but the stronger conflictPrimType still wins
        # over its built-in APIs.
        _VerifyAttrTypes(prim, 
            attr1 = ("int",     1,                  conflictPrimTypeDocStr),
            attr2 = ("int",     2,                  conflictPrimTypeDocStr),
            attr3 = ("int",     3,                  conflictPrimTypeDocStr),
            attr4 = ("int",     4,                  builtinAPIDocStr),
            attr5 = ("color3f", Gf.Vec3f(1, 1, 1),  conflictPrimTypeDocStr),
            attr6 = ("int",     6,                  conflictPrimTypeDocStr))
        _VerifyIsRel(prim,
            rel1 = (True, conflictPrimTypeDocStr),
            rel2 = (False, conflictPrimTypeDocStr))

        # Test 4: Take the same prim from Test 3 above, with type name set to 
        # the conflictPrimType, and author authoredOneAPI and authoredTwoAPI 
        # applied schemas in that strength order (like in Test 1).
        prim.AddAppliedSchema(authoredOneAPIName)
        prim.AddAppliedSchema(authoredTwoAPIName)
        self.assertEqual(prim.GetAppliedSchemas(),
                         [authoredOneAPIName,
                          authoredTwoAPIName,
                          builtinAPIName])

        _VerifyAttrTypes(prim,
            # attr1 is defined in authoredOneAPI which is now the strongest 
            # opinion. Since its type name matches attr1's type name in the prim
            # type's definition, it can be used for attr1 in the composed prim
            #  definition.
            attr1 = ("int",     0,                  authoredOneAPIDocStr),
            # attr2 is defined in authoredOneAPI which would be the strongest 
            # opinion. However, its type name is "double" which doesn't match
            # the existing prim type's definition of the attribute which has it
            # as "int". We use the prim type's definition for this attribute
            # ignoring the API schema.
            attr2 = ("int",     2,                  conflictPrimTypeDocStr),
            # attr3 is defined in authoredTwoAPI (and not authoredOneAPI) which
            # makes it the strongest opinion. Since its type name matches
            # attr3's type name in the prim type's definition, it can be used
            # for attr3 in the composed prim definition.
            attr3 = ("int",     10,                 authoredTwoAPIDocStr),
            # attr4 is defined in authoredTwoAPI (and not authoredOneAPI) which 
            # would be the strongest opinion. However, its type name is "string" 
            # which doesn't match the existing prim type's definition of the 
            # attribute which has it as "int". We use the prim type's definition
            # for this attribute ignoring the API schema. Note the prim type's 
            # definition actually gets its opinion on attr4 from the builtinAPI
            # (as the typed schema itself doesn't define attr4). The builtinAPI
            # attr4 opinion is part of the composed prim type definition which
            # is type wins over any applied API schemas authored over the prim.
            attr4 = ("int",     4,                  builtinAPIDocStr),
            # attr5 is defined in both authoredOneAPI and authoredTwoAPI which
            # makes authoredOneAPI the strongest opiniion. However its type of
            # "point3f" doesn't match the prim type's opinion of the type name
            # "color3f". Even though the default values for these types are
            # stored as GfVec3f, the type name mismatch means we still used the
            # prim type's version in the composed prim definition.
            attr5 = ("color3f", Gf.Vec3f(1, 1, 1),  conflictPrimTypeDocStr),
            # attr6 is defined in both authoredOneAPI and authoredTwoAPI which
            # makes authoredOneAPI the strongest opiniion. And since its type
            # name matches the prim type definition's attr6 type name, we use
            # the attr6 from authoredOneAPI.
            attr6 = ("int",     20,                 authoredOneAPIDocStr))
        # For rel1 and rel2, authoredOneAPI, authoredTwoAPI and the prim type 
        # all define them both. Both authoredTwoAPI and the prim type definition 
        # define rel1 as a relationship and rel2 as an attribute, but 
        # authoredOneAPI defines the reverse with rel2 as the relationship. 
        # Since authoredOneAPI is stronger than authoredTwoAPI, its opinions for
        # these properties override the ones from authoredTwoAPI. However, since
        # the property types of these strongest opinions don't match match the
        # property types from the prim type definition, they fall back to the
        # prim type definition's opinions of these properties in the composed
        # prim definition.
        _VerifyIsRel(prim,
            rel1 = (True, conflictPrimTypeDocStr),
            rel2 = (False, conflictPrimTypeDocStr))

        # Test 5: Prim with no type name; apply the nestedAPI schema only to 
        # the prim. The nestedAPI has authoredOneAPI included as a built-in and
        # authoredTwoAPI auto-applied to it. So all three schemas end up as 
        # applied schemas on the prim.
        nestedAPIPrim = stage.DefinePrim("/NestedAPIPrim")
        nestedAPIPrim.AddAppliedSchema(nestedAPIName)
        self.assertEqual(nestedAPIPrim.GetAppliedSchemas(),
                         [nestedAPIName,
                          authoredOneAPIName,
                          authoredTwoAPIName])

        # The nestedAPI schema defines its own opinions for all of attr1-6 
        # attributes as well as rel1 and rel2. Since it is stronger than all its
        # built-in API schemas, its version of all these properties win for this
        # prim, regardless of the property and attributes types.
        _VerifyAttrTypes(nestedAPIPrim,
            attr1 = ("int",     1,                  nestedAPIDocStr),
            attr2 = ("int",     2,                  nestedAPIDocStr),
            attr3 = ("int",     3,                  nestedAPIDocStr),
            attr4 = ("int",     4,                  nestedAPIDocStr),
            attr5 = ("color3f", Gf.Vec3f(1, 1, 1),  nestedAPIDocStr),
            attr6 = ("token",   "bar",              nestedAPIDocStr))
        _VerifyIsRel(nestedAPIPrim,
            rel1 = (True, nestedAPIDocStr),
            rel2 = (False, nestedAPIDocStr))

        # Test 6: Prim with no type name; apply the bultinAPI, authoredTwoAPI, 
        # and nestedAPI schemas to the prim in that strength order. The same
        # three schemas are applied to the prim as in Test 5, but now the 
        # strength order is different.
        nestedAPIPrim2 = stage.DefinePrim("/NestedAPIPrim2")
        nestedAPIPrim2.AddAppliedSchema(authoredOneAPIName)
        nestedAPIPrim2.AddAppliedSchema(authoredTwoAPIName)
        nestedAPIPrim2.AddAppliedSchema(nestedAPIName)
        self.assertEqual(nestedAPIPrim2.GetAppliedSchemas(),
                         [authoredOneAPIName,
                          authoredTwoAPIName,
                          nestedAPIName,
                          authoredOneAPIName,
                          authoredTwoAPIName])

        # Only the authoredOneAPI and nestedAPI define attr1 and attr2; 
        # authoredOneAPI is stronger and wins
        # Only the authoredTwoAPI and nestedAPI define attr3 and attr4; 
        # authoredTwoAPI is stronger and wins
        # All define attr5 and attr6, so authoredOneAPI's version of those 
        # attributes are used since it's strongest.
        _VerifyAttrTypes(nestedAPIPrim2,
            attr1 = ("int",     0,                  authoredOneAPIDocStr),
            attr2 = ("double",  0.0,                authoredOneAPIDocStr),
            attr3 = ("int",     10,                 authoredTwoAPIDocStr),
            attr4 = ("string",  "foo",              authoredTwoAPIDocStr),
            attr5 = ("point3f", Gf.Vec3f(0, 0, 0),  authoredOneAPIDocStr),
            attr6 = ("int",     20,                 authoredOneAPIDocStr))
        # All define rel1 and rel2, but since authoredOneAPI is strongest, it 
        # wins and only rel2 is a relationship.
        _VerifyIsRel(nestedAPIPrim2,
            rel1 = (False, authoredOneAPIDocStr),
            rel2 = (True, authoredOneAPIDocStr))

        # Test 7: Prim with type name set to the conflictPrimType; author the 
        # nestedAPI schema on this prim. nestedAPI still brings in the 
        # authoredOneAPI and authoredTwoAPI as its own built-ins to the applied 
        # API schemas which will be stronger than the prim type definition 
        # itself.
        prim = stage.DefinePrim("/TypedPrim2", conflictPrimTypeName)
        prim.AddAppliedSchema(nestedAPIName)
        self.assertEqual(prim.GetAppliedSchemas(),
                         [nestedAPIName,
                          authoredOneAPIName,
                          authoredTwoAPIName,
                          builtinAPIName])

        # nestedAPI defines all same properties as the prim type definition,
        # is the strongest opinion, and, with the exception of attr6, uses the 
        # same property types and type names as the prim type definition. So 
        # all properties except attr6 in the composed definition use property
        # definitions from nestedAPI.
        # For attr6, the strongest opinion from nestedAPI has the type name 
        # "token" which doesn't match the type name "int" in the prim type
        # definition. So, we have to use the opinion from the prim type 
        # definition. Note that both authoredOneAPI and authoredTwoAPI are 
        # technically stronger than prim type definition in this case AND have 
        # the attribute type of "int" for attr6. However, we still fall back to
        # the prim type definition's opinion as we only consider the strongest
        # opinion from the authored applied API schemas.
        _VerifyAttrTypes(prim, 
            attr1 = ("int",     1,                  nestedAPIDocStr),
            attr2 = ("int",     2,                  nestedAPIDocStr),
            attr3 = ("int",     3,                  nestedAPIDocStr),
            attr4 = ("int",     4,                  nestedAPIDocStr),
            attr5 = ("color3f", Gf.Vec3f(1, 1, 1),  nestedAPIDocStr),
            attr6 = ("int",     6,                  conflictPrimTypeDocStr))
        _VerifyIsRel(prim,
            rel1 = (True, nestedAPIDocStr),
            rel2 = (False, nestedAPIDocStr))

        # Now also apply authoredOneAPI and authoredTwoAPI directly to the same
        # prim (previously they were included as built-ins under nestedAPI).
        # These are still weaker than nestedAPI but this means they're also now 
        # siblings of nestedAPI in the composed definition as opposed to 
        # built-in to nestedAPI's own prim definition.
        prim.AddAppliedSchema(authoredOneAPIName)
        prim.AddAppliedSchema(authoredTwoAPIName)
        self.assertEqual(prim.GetAppliedSchemas(),
                         [nestedAPIName,
                          authoredOneAPIName,
                          authoredTwoAPIName,
                          authoredOneAPIName,
                          authoredTwoAPIName,
                          builtinAPIName])

        # This extra condition changes nothing about the composed prim 
        # definition but is here to verify that the same behavior for attr6 
        # above (where only the strongest authored API schema opinion for an 
        # attribute is considered) still hold for sibling applied API schemas.
        _VerifyAttrTypes(prim, 
            attr1 = ("int", 1, nestedAPIDocStr),
            attr2 = ("int", 2, nestedAPIDocStr),
            attr3 = ("int", 3, nestedAPIDocStr),
            attr4 = ("int", 4, nestedAPIDocStr),
            attr5 = ("color3f", Gf.Vec3f(1, 1, 1), nestedAPIDocStr),
            attr6 = ("int", 6, conflictPrimTypeDocStr))
        _VerifyIsRel(prim,
            rel1 = (True, nestedAPIDocStr),
            rel2 = (False, nestedAPIDocStr))

        # Test 8: Prim with type name set to the conflictPrimType; author the 
        # authoredOneAPI, authoredTwoAPI, and nestedAPI schemas on this prim in
        # that strength order. This brings in the same 3 applied API schemas as
        # Test 7, but now authoredOneAPI and authoredTwoAPI are stronger than
        # when they were just brought in as built-ins of nestedAPI.
        prim = stage.DefinePrim("/TypedPrim3", conflictPrimTypeName)
        prim.AddAppliedSchema(authoredOneAPIName)
        prim.AddAppliedSchema(authoredTwoAPIName)
        prim.AddAppliedSchema(nestedAPIName)
        self.assertEqual(prim.GetAppliedSchemas(),
                         [authoredOneAPIName,
                          authoredTwoAPIName,
                          nestedAPIName,
                          authoredOneAPIName,
                          authoredTwoAPIName,
                          builtinAPIName])

        # The results of this test case are identical to Test 4 above as 
        # builtAPI is the strongest opinion and authoredTwoAPI is next and the 
        # same property type conflicts exist. Even though nestedAPI is stronger
        # than the prim type definition and has opinions for attr2, attr4, 
        # attr5, rel1, and rel2 of the matching property/attribute type, these 
        # are never the strongest API schema property opinion and are not
        # considered.
        _VerifyAttrTypes(prim,
            attr1 = ("int",     0,                  authoredOneAPIDocStr),
            attr2 = ("int",     2,                  conflictPrimTypeDocStr),
            attr3 = ("int",     10,                 authoredTwoAPIDocStr),
            attr4 = ("int",     4,                  builtinAPIDocStr),
            attr5 = ("color3f", Gf.Vec3f(1, 1, 1),  conflictPrimTypeDocStr),
            attr6 = ("int",     20,                 authoredOneAPIDocStr))
        _VerifyIsRel(prim,
            rel1 = (True, conflictPrimTypeDocStr),
            rel2 = (False, conflictPrimTypeDocStr))

    @unittest.skipIf(Tf.GetEnvSetting('USD_DISABLE_AUTO_APPLY_API_SCHEMAS'),
                    "Auto apply API schemas are disabled")
    def test_PropertyOversInAPISchemas(self):
        """
        Tests the behavior of API schema override properties in schemas that
        built-in API schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Simple helper for testing that a prim has expected attributes that 
        # resolve to expected values.
        def _VerifyAttrValues(prim, expectedAttrValues):
            values = {name : prim.GetAttribute(name).Get() 
                         for name in expectedAttrValues.keys()}
            self.assertEqual(values, expectedAttrValues)

        def _VerifyAttribute(prim, name, typeName,
                             variability = Sdf.VariabilityVarying,
                             custom = False,
                             **kwargs):
            kwargs['typeName'] = typeName
            kwargs['variability'] = variability
            kwargs['custom'] = custom
            attr = prim.GetAttribute(name)
            self.assertTrue(attr)
            self.assertEqual(attr.GetAllMetadata(), kwargs)

        # Built-in API schema inclusion structure for the PropertyOvers API 
        # schemas defined for this test:
        #
        #                           OneAPI
        #                             |
        #                 -------------------------
        #                 |                       |
        #              TwoAPI                    ThreeAPI
        #                 |                       |
        #                 |                 -------------
        #                 |                 |           |
        #          MultiOneAPI:two       FourAPI   AutoApplyAPI (via auto-apply)
        #                 |      
        #        -------------------------------------------------
        #        |                     |                         |
        # MultiTwoAPI:two MultiTwoAPI:two:multiOne MultiThreeAPI:two:multiOne
        #
        # The Typed schemas structure is:
        # 
        #                           inherits
        #      TypePrimDerived ------------------> TypedPrimBase
        #             |                                  |
        #             |                                  |
        #             |                                  |
        #          ThreeAPI                           TwoAPI
        #             |                                  |
        #            ...                                ...
        #

        # prim_1 : Prim with PropertyOversOneAPI applied which includes a tree
        # of all instances of the API schemas we're testing.
        prim_1 = stage.DefinePrim("/Prim1")
        prim_1.ApplyAPI(self.PropertyOversOneAPIType)
        self.assertEqual(prim_1.GetAppliedSchemas(), [
            "TestPropertyOversOneAPI",
            "TestPropertyOversTwoAPI",
            "TestPropertyOversMultiOneAPI:two",
            "TestPropertyOversMultiTwoAPI:two",
            "TestPropertyOversMultiTwoAPI:two:multiOne",
            "TestPropertyOversMultiThreeAPI:two:multiOne",
            "TestPropertyOversThreeAPI",
            "TestPropertyOversFourAPI",
            "TestPropertyOversAutoApplyAPI"])

        # prim_2 : Prim with PropertyOversTwoAPI applied which includes the 
        # first subtree of the instances of the API schemas we're testing.
        prim_2 = stage.DefinePrim("/Prim2")
        prim_2.ApplyAPI(self.PropertyOversTwoAPIType)
        self.assertEqual(prim_2.GetAppliedSchemas(), [
            "TestPropertyOversTwoAPI",
            "TestPropertyOversMultiOneAPI:two",
            "TestPropertyOversMultiTwoAPI:two",
            "TestPropertyOversMultiTwoAPI:two:multiOne",
            "TestPropertyOversMultiThreeAPI:two:multiOne"])

        # prim_3 : Prim with PropertyOversThreeAPI applied which includes the
        # second subtree of the instances of the API schemas we're testing.
        prim_3 = stage.DefinePrim("/Prim3")
        prim_3.ApplyAPI(self.PropertyOversThreeAPIType)
        self.assertEqual(prim_3.GetAppliedSchemas(), [
            "TestPropertyOversThreeAPI",
            "TestPropertyOversFourAPI",
            "TestPropertyOversAutoApplyAPI"])

        # prim_4 : Prim with PropertyOversFourAPI applied which doesn't include 
        # any other API schemas of its own, but is included in the API schema
        # trees of the prims mentioned above.
        prim_4 = stage.DefinePrim("/Prim4")
        prim_4.ApplyAPI(self.PropertyOversFourAPIType)
        self.assertEqual(prim_4.GetAppliedSchemas(), [
            "TestPropertyOversFourAPI"])

        # prim_m_1 : Prim with an instance of multiple apply schema
        # PropertyOversMultiOneAPI applied using the instance name "two". The
        # API schema includes a tree of other multiple apply API schemas and
        # this instance matches the subtree included by PropertyOversTwoAPI
        prim_m_1 = stage.DefinePrim("/PrimM1")
        prim_m_1.ApplyAPI(self.PropertyOversMultiOneAPIType, "two")
        self.assertEqual(prim_m_1.GetAppliedSchemas(), [
            "TestPropertyOversMultiOneAPI:two",
            "TestPropertyOversMultiTwoAPI:two",
            "TestPropertyOversMultiTwoAPI:two:multiOne",
            "TestPropertyOversMultiThreeAPI:two:multiOne"
            ])

        # prim_m_1 : Prim with an instance of multiple apply schema
        # PropertyOversMultiTwoAPI applied using the instance name "two". This
        # schema doesn't include any other API schemas of its own, but this
        # instance of the schema is included in the API schema trees of the
        # prims mentioned above.
        prim_m_2 = stage.DefinePrim("/PrimM2")
        prim_m_2.ApplyAPI(self.PropertyOversMultiTwoAPIType, "two")
        self.assertEqual(prim_m_2.GetAppliedSchemas(), [
            "TestPropertyOversMultiTwoAPI:two",
            ])

        # primBase : Prim of type PropertyOversTypedPrimBase which includes the
        # built-in PropertyOversTwoAPI and therefore all of the API schemas that
        # includes.
        primBase = stage.DefinePrim("/PrimBase", 
                                    "TestPropertyOversTypedPrimBase")
        self.assertTrue(primBase)
        self.assertEqual(primBase.GetAppliedSchemas(), [
            "TestPropertyOversTwoAPI",
            "TestPropertyOversMultiOneAPI:two",
            "TestPropertyOversMultiTwoAPI:two",
            "TestPropertyOversMultiTwoAPI:two:multiOne",
            "TestPropertyOversMultiThreeAPI:two:multiOne"])

        # primDerived : Prim of type PropertyOversTypedPrimDerived which both
        # inherits from PropertyOversTypedPrimBase and prepends its own built-in
        # PropertyOversThreeAPI. Thus it includes properties from the base prim
        # type as well as the all the include API schemas imparted by
        # PropertyOversThreeAPI and PropertyOversTwoAPI
        primDerived = stage.DefinePrim("/PrimDerived",
                                       "TestPropertyOversTypedPrimDerived")
        self.assertTrue(primDerived)
        self.assertEqual(primDerived.GetAppliedSchemas(), [
            "TestPropertyOversThreeAPI",
            "TestPropertyOversFourAPI",
            "TestPropertyOversAutoApplyAPI",
            "TestPropertyOversTwoAPI",
            "TestPropertyOversMultiOneAPI:two",
            "TestPropertyOversMultiTwoAPI:two",
            "TestPropertyOversMultiTwoAPI:two:multiOne",
            "TestPropertyOversMultiThreeAPI:two:multiOne"])

        # Verify the expected properties of all of the prims we defined above. 
        # We check the exact fields of all of these expected properties later
        # but this serves as a first verification that we got what we expected.
        #
        # But more importantly this is a useful way to test that none our prims
        # include any form of the "overrides_nothing" property which is defined
        # in every one of these test schemas as an API schema override but is
        # never defined as a concrete property in any of them.
        self.assertEqual(prim_1.GetPropertyNames(), [
            "defined_in_auto",
            "defined_in_four_1",
            "defined_in_four_2",
            "defined_in_three",
            "defined_in_two",
            "int_defined_in_two",
            "multi:two:defined_in_m1",
            "multi:two:defined_in_m2",
            "multi:two:multiOne:defined_in_m2",
            "otherMulti:two:multiOne:defined_in_m3",
            "uniform_token_defined_in_four"
            ])

        self.assertEqual(prim_2.GetPropertyNames(), [
            "defined_in_two",
            "int_defined_in_two",
            "multi:two:defined_in_m1",
            "multi:two:defined_in_m2",
            "multi:two:multiOne:defined_in_m2",
            "otherMulti:two:multiOne:defined_in_m3"
            ])

        self.assertEqual(prim_3.GetPropertyNames(), [
            "defined_in_auto",
            "defined_in_four_1",
            "defined_in_four_2",
            "defined_in_three",
            "uniform_token_defined_in_four"
            ])

        self.assertEqual(prim_4.GetPropertyNames(), [
            "defined_in_four_1",
            "defined_in_four_2",
            "uniform_token_defined_in_four"
            ])

        self.assertEqual(prim_m_1.GetPropertyNames(), [
            "multi:two:defined_in_m1",
            "multi:two:defined_in_m2",
            "multi:two:multiOne:defined_in_m2",
            "otherMulti:two:multiOne:defined_in_m3"
            ])

        self.assertEqual(prim_m_2.GetPropertyNames(), [
            "multi:two:defined_in_m2"
            ])

        self.assertEqual(primBase.GetPropertyNames(), [
            "defined_in_base",
            "defined_in_two",
            "int_defined_in_two",
            "multi:two:defined_in_m1",
            "multi:two:defined_in_m2",
            "multi:two:multiOne:defined_in_m2",
            "otherMulti:two:multiOne:defined_in_m3"
            ])

        self.assertEqual(primDerived.GetPropertyNames(), [
            "defined_in_auto",
            "defined_in_base",
            "defined_in_four_1",
            "defined_in_four_2",
            "defined_in_three",
            "defined_in_two",
            "int_defined_in_two",
            "multi:two:defined_in_m1",
            "multi:two:defined_in_m2",
            "multi:two:multiOne:defined_in_m2",
            "otherMulti:two:multiOne:defined_in_m3",
            "over_in_base",
            "uniform_token_defined_in_four"
            ])

        # Property: defined_in_two
        # Defined in PropertyOversTwoAPI
        #   value = "two"
        #   allowedTokens = "two", "2"
        #   doc = "Defined in Two"
        # Override in PropertyOversOneAPI sets default value to "1"
        # Override in PropertyOversThreeAPI tries to set doc string
        # Override in PropertyOversFourAPI tries to set default value to "4"
        # Override in PropertyOversTypedPrimBase sets default value to "base_over"
        propName = "defined_in_two"
        # Default value changed by OneAPI, overrides from ThreeAPI and FourAPI
        # are ignored because they don't affect sibling TwoAPI. 
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["two", "2"]),
                         default = "1",
                         documentation = "Defined in Two")
        # Matches def in TwoAPI as there are no overrides.
        _VerifyAttribute(prim_2, propName, 'token',
                         allowedTokens = Vt.TokenArray(["two", "2"]),
                         default = "two",
                         documentation = "Defined in Two")
        # ThreeAPI and FourAPI only define overrides but there's no actual
        # property def included in their built-ins, so the properties don't
        # exist.
        self.assertFalse(prim_3.GetAttribute(propName))
        self.assertFalse(prim_4.GetAttribute(propName))

        # The TypedPrimBase includes PropertyOversTwoAPI and overrides the
        # default. Both the base and derived types get this property with the
        # default value override.
        _VerifyAttribute(primBase, propName, 'token',
                         allowedTokens = Vt.TokenArray(["two", "2"]),
                         default = "base_over",
                         documentation = "Defined in Two")
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["two", "2"]),
                         default = "base_over",
                         documentation = "Defined in Two")

        # Property: defined_in_three
        # Defined in PropertyOversThreeAPI
        #   value = "three"
        #   allowedTokens = "three", "3"
        #   doc = "Defined in Three"
        # Override in PropertyOversTwoAPI tries to set doc string
        # Override in PropertyOversTypedPrimBase sets default value to "base_over"
        propName = "defined_in_three"
        # Doc value override in built-in TwoAPI, which is a stronger sibling
        # of built-in ThreeAPI, is ignored because we don't process overrides
        # across sibling API schemas.
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "three",
                         documentation = "Defined in Three")
        # TwoAPI defines an override but doesn't include any API schemas that
        # define this property.
        self.assertFalse(prim_2.GetAttribute(propName))
        # Matches def in ThreeAPI as there are no overrides.
        _VerifyAttribute(prim_3, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "three",
                         documentation = "Defined in Three")
        # Not defined in FourAPI at all.
        self.assertFalse(prim_4.GetAttribute(propName))

        # Since the TypedPrimBase overrides the default value of the property,
        # the derived TypedPrimDerived inherits this override and applies it
        # to this property it includes via PropertyOversThreeAPI. However, the
        # TypedPrimBase does not itself include PropertyOversThreeAPI so it
        # doesn't include the property at all.
        self.assertFalse(primBase.GetAttribute(propName))
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "base_over",
                         documentation = "Defined in Three")

        # Property: defined_in_four_1
        # Defined in PropertyOversFourAPI
        #   value = "four"
        #   allowedTokens = "four", "4"
        #   doc = "Defined in Four"
        # Override in PropertyOversOneAPI sets default value to "1"
        # Override in PropertyOversTwoAPI sets default value to "2" and
        #   allowedTokens to ["two", "2"]
        # Override in PropertyOversTypedPrimDerived sets default value to
        #   "derived_over"
        propName = "defined_in_four_1"
        # Gets overrides from both OneAPI and TwoAPI. However TwoAPI's override
        # is ignored since FourAPI is not included through TwoAPI or any of
        # its built-in APIs. So only the default value is overridden to use
        # OneAPI's value.
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["four", "4"]),
                         default = "1",
                         documentation = "Defined in Four")
        # TwoAPI defines an override but doesn't include any API schemas that
        # define this property.
        self.assertFalse(prim_2.GetAttribute(propName))
        # ThreeAPI includes FourAPI but doesn't define overrides so it and
        # FourAPI match the defined property.
        _VerifyAttribute(prim_3, propName, 'token',
                         allowedTokens = Vt.TokenArray(["four", "4"]),
                         default = "four",
                         documentation = "Defined in Four")
        _VerifyAttribute(prim_4, propName, 'token',
                         allowedTokens = Vt.TokenArray(["four", "4"]),
                         default = "four",
                         documentation = "Defined in Four")

        # The TypedPrimDerived includes PropertyOversFourAPI and overrides the
        # default value. The base type does not include this API schema and
        # does not have this property.
        self.assertFalse(primBase.GetAttribute(propName))
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["four", "4"]),
                         default = "derived_over",
                         documentation = "Defined in Four")

        # Property: defined_in_four_2
        # Defined in PropertyOversFourAPI
        #   value = "four"
        #   allowedTokens = "four", "4"
        #   doc = "Defined in Four also"
        # Override in PropertyOversOneAPI sets default value to "1"
        # Override in PropertyOversThreeAPI sets default value to "3" and
        #   allowedTokens to ["three", "3"]
        # Override in TypedPrimBase that sets the default value to "base_over"
        #   and allowedTokens to ["base_over", "over_base"]
        # Override in TypedPrimDerived that sets the default value to
        #   "derived_over"
        propName = "defined_in_four_2"
        # Gets overrides from both OneAPI and ThreeAPI. OneAPI's default value
        # override overwrites the default value override from ThreeAPI. OneAPI
        # has no opinion on the allowedTokens so ThreeAPI's override comes 
        # through.
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "1",
                         documentation = "Defined in Four also")
        # TwoAPI doesn't include FourAPI
        self.assertFalse(prim_2.GetAttribute(propName))
        # ThreeAPI overrides both the default value and the allowed tokens.
        _VerifyAttribute(prim_3, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "3",
                         documentation = "Defined in Four also")
        # FourAPI defines the property. 
        _VerifyAttribute(prim_4, propName, 'token',
                         allowedTokens = Vt.TokenArray(["four", "4"]),
                         default = "four",
                         documentation = "Defined in Four also")

        # TypedPrimDerived includes PropertyOversThreeAPI (which includes 
        # PropertyOversFourAPI) and overrides the default value. But it also
        # inherits from TypedPrimBase which also overrides the default value and
        # the allowedTokens. Since TypePrimDerived has a stronger override
        # opinion about the default value, its default value is used. Since
        # TypePrimDerived does NOT have an opinion about the allowedTokens, the
        # base's override for allowed Tokens is used.
        # TypedPrimBase still does not include PropertyOversFourAPI itself so
        # it does not have this property.
        self.assertFalse(primBase.GetAttribute(propName))
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["base_over", "over_base"]),
                         default = "derived_over",
                         documentation = "Defined in Four also")

        # Property: multi:two:defined_in_m1
        # Defined in PropertyOversMultiOneAPI as multi:_INST_:defined_in_m1
        #   value = "multi_one"
        #   allowedTokens = "multi_one", "m1"
        #   doc = "Defined in MultiOne"
        # Override in PropertyOversOneAPI sets default value to "1"
        # Override in PropertyOversTypedPrimDerived sets default value to
        # "derived_over"
        propName = "multi:two:defined_in_m1"
        # OneAPI overrides the explicit instance of multi-apply property to
        # set its value to "1"
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_one", "m1"]),
                         default = "1",
                         documentation = "Defined in MultiOne")
        # TwoAPI includes MultiOneAPI:two but doesn't override this property
        _VerifyAttribute(prim_2, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_one", "m1"]),
                         default = "multi_one",
                         documentation = "Defined in MultiOne")
        # MultiOneAPI:two defines this property
        _VerifyAttribute(prim_m_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_one", "m1"]),
                         default = "multi_one",
                         documentation = "Defined in MultiOne")
        # MulitTwoAPI doesn't include a def for this property.
        self.assertFalse(prim_m_2.GetAttribute(propName))

        # TypedPrimBase includes OversTwoAPI which includes MultiOneAPI:two
        # but does not define any overrides to the property. TypedPrimDerived
        # does define a default value override so that value is used in the
        # derived type prim.
        _VerifyAttribute(primBase, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_one", "m1"]),
                         default = "multi_one",
                         documentation = "Defined in MultiOne")
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_one", "m1"]),
                         default = "derived_over",
                         documentation = "Defined in MultiOne")

        # Property: multi:two:defined_in_m2
        # Defined in PropertyOversMultiTwoAPI as multi:_INST_:defined_in_m2
        #   value = "multi_two"
        #   allowedTokens = "multi_two", "m2"
        #   doc = "Defined in MultiTwo"
        # Override of multi:_INST_:defined_in_m2 in PropertyOversMultiOneAPI
        #   sets default value to "m1"
        # 
        # Override of multi:two:defined_in_m2 in PropertyOversTypedPrimBase sets
        #   default value to "base_over" and allowed tokens to
        #   ["base_over", "over_base"]
        # Override of multi:two:defined_in_m2 in PropertyOversTypedPrimDerived
        #   sets default value to "default_over"
        propName = "multi:two:defined_in_m2"
        # The default value override from MultiOneAPI applies over the property
        # defined in MultiTwoAPI for all API schemas that directly or
        # indirectly include an instance of MultiOneAPI
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "m1",
                         documentation = "Defined in MultiTwo")
        _VerifyAttribute(prim_2, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "m1",
                         documentation = "Defined in MultiTwo")
        _VerifyAttribute(prim_m_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "m1",
                         documentation = "Defined in MultiTwo")
        # The prim that only includes the MultiTwoAPI:two instance uses
        # MultiTwo's definition of the prim as it is defined.
        _VerifyAttribute(prim_m_2, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "multi_two",
                         documentation = "Defined in MultiTwo")

        # TypedPrimBase includes OversTwoAPI which includes MultiTwoAPI:two
        # so this property is present. Since TypedPrimBase overrides the
        # default value and allowed tokens, those values are used in the
        # TypedPrimBase prim
        _VerifyAttribute(primBase, propName, 'token',
                         allowedTokens = Vt.TokenArray(["base_over", "over_base"]),
                         default = "base_over",
                         documentation = "Defined in MultiTwo")
        # TypedPrimDerived additionally overrides the default value with its
        # own override so its default value is used. The override for allowed
        # tokens from TypedPrimBase is used since the TypedPrimDerived doesn't
        # specify its own value for the field.
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["base_over", "over_base"]),
                         default = "derived_over",
                         documentation = "Defined in MultiTwo")

        # Property: multi:two:multiOne:defined_in_m2
        # Defined in PropertyOversMultiTwoAPI as multi:_INST_:defined_in_m2
        #   value = "multi_two"
        #   allowedTokens = "multi_two", "m2"
        #   doc = "Defined in MultiTwo"
        # Override of multi:_INST_:multiOne:defined_in_m2 in
        #   PropertyOversMultiOneAPI sets default value to "m1"
        propName = "multi:two:multiOne:defined_in_m2"

        # This test case serves as a complement to the one above.
        # MultiOneAPI includes MultiTwoAPI twice.
        #   First as the MultiTwoAPI:_INST_
        #   Second as the MultiTwoAPI:_INST_:multiOne.
        # These two instances provide to MultiOneAPI definitions for the
        # respective properties
        #   multi:_INST_:defined_in_m2
        #   multi:_INST_:multiOne:defined_in_m2
        # MultiOneAPI provides overrides for both of these properties, in this
        # case overriding the default value to "multiOne:m1"
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "multiOne:m1",
                         documentation = "Defined in MultiTwo")
        _VerifyAttribute(prim_2, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "multiOne:m1",
                         documentation = "Defined in MultiTwo")
        _VerifyAttribute(prim_m_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "multiOne:m1",
                         documentation = "Defined in MultiTwo")
        # This prim includes MultiTwoAPI:two not MultiTwoAPI:two:multiOne, so
        # it doesn't have this property.
        self.assertFalse(prim_m_2.GetAttribute(propName))

        # No additional overrides in the typed prim schemas for this property
        # so the properties match the one's from the other prim's above.
        _VerifyAttribute(primBase, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "multiOne:m1",
                         documentation = "Defined in MultiTwo")
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_two", "m2"]),
                         default = "multiOne:m1",
                         documentation = "Defined in MultiTwo")

        # XXX: This case is meant to test the behavior of overriding an property
        # in a multiple apply API schema that includes another multiple apply
        # API schema with a different property namespace prefix. This is 
        # supported in the generateSchema.usda layers, but there's currently 
        # no way to specify this in the schema.usda for usdGenSchema to 
        # produce generateSchema.usda files that do these overrides. The
        # generatedSchema.usda for this test is manually updated to include this
        # override so that we can still test this functionality until we're able
        # to generate these types of overrides in usdGenSchema in the future.
        #
        # Property: otherMulti:two:multiOne:defined_in_m3
        # Defined in PropertyOversMultiThreeAPI as otherMulti:_INST_:defined_in_m3
        #   value = "multi_three"
        #   allowedTokens = "multi_three", "m3"
        #   doc = "Defined in MultiThree"
        # Override of otherMulti:_INST_:multiOne:defined_in_m3 in
        #   PropertyOversMultiOneAPI sets default value to "m1"
        propName = "otherMulti:two:multiOne:defined_in_m3"

        # This is similar to how there's a property override for
        # multi:_INST_:defined_in_m2 in MultiOneAPI in an above case. Here,
        # we're proving that we can define a multiple apply API schema property
        # override in another multi apply API schema even if they have
        # differing property namespace prefixes as long as we define the
        # override using the correct prefix.
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_three", "m3"]),
                         default = "m1",
                         documentation = "Defined in MultiThree")
        _VerifyAttribute(prim_2, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_three", "m3"]),
                         default = "m1",
                         documentation = "Defined in MultiThree")
        _VerifyAttribute(prim_m_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_three", "m3"]),
                         default = "m1",
                         documentation = "Defined in MultiThree")
        self.assertFalse(prim_m_2.GetAttribute(propName))

        # No overrides in the typed prim schemas.
        _VerifyAttribute(primBase, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_three", "m3"]),
                         default = "m1",
                         documentation = "Defined in MultiThree")
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["multi_three", "m3"]),
                         default = "m1",
                         documentation = "Defined in MultiThree")

        # Property: int_defined_in_two
        # Defined in PropertyOversTwoAPI
        #   value = 2
        #   typeName = int
        #   doc = "Defined in Two"
        # Override in PropertyOversOneAPI tries to override the typeName to 
        #   'token' and set the value to "1"
        # Override in PropertyOversTypedPrimBase overrides the default value to 
        #   10 using the correct typeName 'int' and overrides the doc string.
        # Override in PropertyOversTypedPrimDerived tries to override the 
        #   typeName to 'token' and set the value to "1"
        propName = "int_defined_in_two"
        # It is invalid for an override to change the typeName of the defined
        # property. The override in OneAPI is ignored.
        _VerifyAttribute(prim_1, propName, 'int',
                         default = 2,
                         documentation = "Int defined in Two")
        _VerifyAttribute(prim_2, propName, 'int',
                         default = 2,
                         documentation = "Int defined in Two")

        # TypedPrimBase includes PropertyOversTwoAPI and has an override for
        # the property (using the correct type) that sets the default to 10 and
        # changes the doc string.
        _VerifyAttribute(primBase, propName, 'int',
                         default = 10,
                         documentation = "Int override in Base")
        # TypedPrimDerived has an override with a mismatched typeName. We 
        # however compose all type inheritance first so the override from the 
        # TypedPrimDerived is composed over TypedPrimBase's override before it
        # is composed with the API schema properties so no override of this 
        # property is applied because of the type mismatch. 
        _VerifyAttribute(primDerived, propName, 'int',
                         default = 2,
                         documentation = "Int defined in Two")

        # Property: uniform_token_defined_in_four
        # Defined in PropertyOversFourAPI
        #   value = "unt_form"
        #   variability = uniform
        #   allowedTokens = "uni_four", "uni_4"
        #   doc = "Uniform token defined in Four"
        # Override in PropertyOversThreeAPI tries to override the default value
        #   and allowed token but declares the attribute as a non-uniform token
        #   attribute.
        # Override in PropertyOversOneAPI overrides the default value on a 
        #   properly uniform attribute.
        # Override in PropertyOversTypedPrimDerived tries to override the 
        #   default value but declares the attribute as a non-uniform token
        #   attribute.
        propName = "uniform_token_defined_in_four"
        # OversOneAPI includes OversThreeAPI which includes OversFourAPI.
        # The OversThreeAPI's override is a variability mismatch which causes
        # it to be ignored but that doesn't prevent the correctly matched
        # override from OversOneAPI from overriding the default value.
        _VerifyAttribute(prim_1, propName, 'token',
                         variability = Sdf.VariabilityUniform,
                         allowedTokens = Vt.TokenArray(["uni_four", "uni_4"]),
                         default = "uni_1",
                         documentation = "Uniform token defined in Four")
        self.assertFalse(prim_2.GetAttribute(propName))
        # The OversThreeAPI override variability mismatch causes it to be
        # ignored.
        _VerifyAttribute(prim_3, propName, 'token',
                         variability = Sdf.VariabilityUniform,
                         allowedTokens = Vt.TokenArray(["uni_four", "uni_4"]),
                         default = "uni_four",
                         documentation = "Uniform token defined in Four")
        _VerifyAttribute(prim_4, propName, 'token',
                         variability = Sdf.VariabilityUniform,
                         allowedTokens = Vt.TokenArray(["uni_four", "uni_4"]),
                         default = "uni_four",
                         documentation = "Uniform token defined in Four")

        # TypedPrimBase does not include the property. TypedPrimDerived's
        # override is a variability mismatch and is ignored.
        self.assertFalse(primBase.GetAttribute(propName))
        _VerifyAttribute(primDerived, propName, 'token',
                         variability = Sdf.VariabilityUniform,
                         allowedTokens = Vt.TokenArray(["uni_four", "uni_4"]),
                         default = "uni_four",
                         documentation = "Uniform token defined in Four")

        # Property: defined_in_auto
        # Defined in PropertyOversAutoApplyAPI
        #   value = "auto"
        #   allowedTokens = "auto"
        #   doc = "Defined in Auto"
        # Override in PropertyOversOneAPI sets default value to "1"
        # Override in PropertyOversThreeAPI sets default value to "3" and
        #   allowedTokens to ["three", "3"]
        # Override in PropertyOversFour attempts to set default value and 
        #   doc string
        #
        # This case is to verify that API schema property overrides can apply
        # to built-in schemas that are auto-applied which behave the same as
        # other built-in API schemas.
        propName = "defined_in_auto"
        # OneAPI includes ThreeAPI which AutoApplyAPI is applied to. OneAPI
        # composes its override with ThreeAPI's override which composes over
        # the auto apply schemas attribute definition.
        _VerifyAttribute(prim_1, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "1",
                         documentation = "Defined in Auto")
        # TwoAPI does not include AutoApplyAPI
        self.assertFalse(prim_2.GetAttribute(propName))
        # AutoApplyAPI is auto applied to ThreeAPI so ThreeAPI has the attribute
        # with its override composed over it. Note that FourAPI (which ThreeAPI
        # includes) declares an override that would change the doc string and
        # default value, but it's a sibling of AutoApplyAPI under ThreeAPI so
        # the override is not composed.
        _VerifyAttribute(prim_3, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "3",
                         documentation = "Defined in Auto")
        # FourAPI defines an override for the attribute, but it does not include
        # the AutoApplyAPI so it does not have the attribute.
        self.assertFalse(prim_4.GetAttribute(propName))

        # For comparison, define a prim with just the auto apply API applied.
        # It has the originally defined field with no overrides for the 
        # property.
        prim_auto = stage.DefinePrim("/PrimAuto")
        prim_auto.ApplyAPI(self.PropertyOversAutoApplyAPIType)
        _VerifyAttribute(prim_auto, propName, 'token',
                         allowedTokens = Vt.TokenArray(["auto"]),
                         default = "auto",
                         documentation = "Defined in Auto")

        # The TypedPrimDerived includes PropertyOversThreeAPI with no overrides
        # of its own so the value matches ThreeAPI. The base type does not 
        # include this API schema and does not have this property.
        self.assertFalse(primBase.GetAttribute(propName))
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["three", "3"]),
                         default = "3",
                         documentation = "Defined in Auto")

        # Property: defined_in_base
        # Defined in PropertyOversTypedPrimBase
        #   value = "base_def"
        #   allowedTokens = "base_def", "def_base"
        #   doc = "Defined in Base"
        # Property also defined in PropertyOversTypedPrimDerived setting just
        #   the default value to "derived_over"
        propName = "defined_in_base"
        # These are Typed schemas with inheritance, no API schema overrides on 
        # the property. This is just to verify that standard property 
        # composition happens for Typed schema inheritance.
        _VerifyAttribute(primBase, propName, 'token',
                         allowedTokens = Vt.TokenArray(["base_def", "def_base"]),
                         default = "base_def",
                         documentation = "Defined in Base")
        # The derived schema's default value composes with the property 
        # defintion from the base class.
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["base_def", "def_base"]),
                         default = "derived_over",
                         documentation = "Defined in Base")
        
        # Property: over_in_base
        # Defined in PropertyOversTypedPrimBase
        #   value = "base_over"
        #   allowedTokens = "base_over", "over_base"
        #   doc = "Override in Base"
        #   apiSchemaOverride = true
        #
        # Defined in PropertyOversTypedPrimDerived
        #   value = "derived_def"
        #   apiSchemaOverride = false
        propName = "over_in_base"
        # This is testing inheritance composition of a property in Typed schemas
        # where a derived class turns an API schema override property into a
        # defined property.
        #
        # TypedPrimBase defines this as an API schema override with but there's
        # no property defined in its built-in API schemas to make it exist in 
        # the base prim.
        self.assertFalse(primBase.GetAttribute(propName))
        # TypedPrimDerived changes the property to no longer be an API schema
        # override so it is defined in the derived prim. The derived property
        # is still composed with the base property to create the derived 
        # schema's property even though the base was declared as an API schema 
        # override.
        _VerifyAttribute(primDerived, propName, 'token',
                         allowedTokens = Vt.TokenArray(["base_over", "over_base"]),
                         default = "derived_def",
                         documentation = "Override in Base")

if __name__ == "__main__":
    unittest.main()
