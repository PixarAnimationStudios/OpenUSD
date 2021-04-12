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
    
    def test_SimpleTypedSchemaPrimDefinition(self):
        """
        Tests the prim definition for a simple typed schema that has no
        fallback API schemas
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

    def test_TypedSchemaWithFallbackAPISchemas(self):
        """
        Tests the prim definition for schema prim type that has API schemas
        applied to it in its generated schema.
        """

        # Find the prim definition for the test single apply schema. It has
        # some properties defined.
        singleApplyAPIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestSingleApplyAPI")
        self.assertTrue(singleApplyAPIDef)
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
        self.assertEqual(multiApplyAPIDef.GetPropertyNames(), [
            "bool_attr", "token_attr", "relationship"])
        self.assertEqual(multiApplyAPIDef.GetDocumentation(),
            "Test multi-apply API schema")

        # Find the prim definition for the concrete prim type with fallback
        # API schemas. You can query its API schemas and it will have properties
        # from those schemas already.
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestWithFallbackAppliedSchema")
        self.assertTrue(primDef)
        self.assertEqual(primDef.GetAppliedAPISchemas(), [
            "TestSingleApplyAPI", "TestMultiApplyAPI:fallback"])
        self.assertEqual(sorted(primDef.GetPropertyNames()), [
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])
        # Note that prim def documentation does not come from the fallback API
        # schemas.
        self.assertEqual(primDef.GetDocumentation(), 
                         "Test with fallback API schemas")

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
            "multi:fallback:token_attr")
        self.assertEqual(multiTokenAttr, 
            multiApplyAPIDef.GetSchemaAttributeSpec("token_attr"))
        self.assertEqual(multiTokenAttr.default, "foo")
        self.assertEqual(multiTokenAttr.typeName.cppTypeName, "TfToken")

        multiRelationship = primDef.GetSchemaRelationshipSpec(
            "multi:fallback:relationship")
        self.assertTrue(multiRelationship)
        self.assertEqual(multiRelationship, 
            multiApplyAPIDef.GetSchemaRelationshipSpec("relationship"))

        # Verify the case where the concrete type overrides a property from 
        # one of its applied API schemas. In this case the property spec from
        # the concrete prim is returned instead of the property spec from the
        # API schema.
        multiBoolAttr = primDef.GetSchemaAttributeSpec(
            "multi:fallback:bool_attr")
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

    def test_TypedPrimsOnStageWithFallback(self):
        """
        Tests the fallback properties of typed prims on a stage when new API
        schemas are applied to a prim whose type already has applied API 
        schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim. It has API schemas already from its prim definition
        # and has properties from both its type and its APIs.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestWithFallbackAppliedSchema")
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithFallbackAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:fallback"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithFallbackAppliedSchema')
        # Note that prim type info does NOT contain the fallback applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])

        # Add a new api schemas to the prim's metadata.
        typedPrim.ApplyAPI(self.MultiApplyAPIType, "garply")

        # Prim has the same type and now has both its original API schemas and
        # the new one. Note that the new schema was added using an explicit 
        # list op but was still prepended to the original list. Fallback API 
        # schemas cannot be deleted and any authored API schemas will always be
        # prepended to the fallbacks.
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithFallbackAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
            ["TestMultiApplyAPI:garply", 
             "TestSingleApplyAPI", "TestMultiApplyAPI:fallback"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithFallbackAppliedSchema')
        # Note that prim type info does NOT contain the fallback applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestMultiApplyAPI:garply"])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))

        # Properties have been expanded to include the new API schema
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
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
        # Property fallback actually comes from TestWithFallbackAppliedSchema as
        # the typed schema overrides this property from its fallback API schema.
        attr = typedPrim.GetAttribute("multi:fallback:bool_attr")
        self.assertEqual(attr.Get(), False)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestWithFallbackAppliedSchema
        attr = typedPrim.GetAttribute("testAttr")
        self.assertEqual(attr.Get(), "foo")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)

        # Metadata "hidden" has a fallback value defined in 
        # TestWithFallbackAppliedSchema. It will be returned by GetMetadata and 
        # GetAllMetadata but will return false for queries about whether it's 
        # authored
        self.assertEqual(typedPrim.GetAllMetadata()["hidden"], False)
        self.assertEqual(typedPrim.GetMetadata("hidden"), False)
        self.assertFalse(typedPrim.HasAuthoredMetadata("hidden"))
        self.assertFalse("hidden" in typedPrim.GetAllAuthoredMetadata())

        # Documentation metadata comes from prim type definition even with API
        # schemas applied.
        self.assertEqual(typedPrim.GetMetadata("documentation"), 
                         "Test with fallback API schemas")

    def test_TypedPrimsOnStageWithFallbackReapply(self):
        """
        Tests the fallback properties of typed prims on a stage when the same 
        API schemas are applied again to a prim whose type already has applied 
        API schemas.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim. It has API schemas already from its prim definition
        # and has properties from both its type and its APIs.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestWithFallbackAppliedSchema")
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithFallbackAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:fallback"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithFallbackAppliedSchema')
        # Note that prim type info does NOT contain the fallback applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), [])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
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
        # schema overrides this property from its fallback API schema.
        attr = typedPrim.GetAttribute("multi:fallback:bool_attr")
        self.assertEqual(attr.Get(), False)
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)
        # Property fallback comes from TestTypedSchema
        attr = typedPrim.GetAttribute("testAttr")
        self.assertEqual(attr.Get(), "foo")
        self.assertEqual(attr.GetResolveInfo().GetSource(), 
                         Usd.ResolveInfoSourceFallback)

        # Add the fallback api schemas again to the prim's metadata.
        typedPrim.ApplyAPI(self.MultiApplyAPIType, "fallback")
        typedPrim.ApplyAPI(self.SingleApplyAPIType)

        # Prim has the same type and now has both its original API schemas and
        # plus the same schemas again appended to the list (i.e. both schemas
        # now show up twice). 
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithFallbackAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
            ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI", 
             "TestSingleApplyAPI", "TestMultiApplyAPI:fallback"])
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetTypeName(), 
                         'TestWithFallbackAppliedSchema')
        # Note that prim type info does NOT contain the fallback applied API
        # schemas from the concrete type's prim definition as these are not part
        # of the type identity.
        self.assertEqual(typedPrim.GetPrimTypeInfo().GetAppliedAPISchemas(), 
                         ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI"])

        self.assertTrue(typedPrim.HasAPI(self.SingleApplyAPIType))
        self.assertTrue(typedPrim.HasAPI(self.MultiApplyAPIType))

        # The list of properties hasn't changed as there are no "new" schemas,
        # however the defaults may have changed.
        self.assertEqual(typedPrim.GetPropertyNames(), [
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
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
        # Property fallback has now change to True to False as the 
        # TestTypedSchema originally overrode the fallback from 
        # TestMultiApplyAPI. But by applying TestMultiApplyAPI again with the 
        # same instance, we've re-overridden the attribute getting the default
        # from the applied schema.
        attr = typedPrim.GetAttribute("multi:fallback:bool_attr")
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
                         "Test with fallback API schemas")

    def test_TypedPrimsOnStageWithAutoAppliedAPIs(self):
        """
        Tests the fallback properties of typed prims on a stage where API
        schemas are auto applied.
        """
        stage = Usd.Stage.CreateInMemory()

        # Add a typed prim that has two types of builtin applied schemas. 
        # TestMultiApplyAPI:fallback comes from the apiSchemas metadata defined
        # in TestTypedSchemaForAutoApply's schema definition.
        # TestSingleApplyAPI and TestMultiApplyAPI:autoFoo come from 
        # TestTypedSchemaForAutoApply being listed in the "AutoApplyAPISchemas"
        # plugInfo metadata for both API schemas.
        # The builtin applied schemas that come from the apiSchemas metadata 
        # will always be listed before (and be stronger than) any applied 
        # schemas that come from apiSchemaAutoApplyTo.
        typedPrim = stage.DefinePrim("/TypedPrim", "TestTypedSchemaForAutoApply")
        self.assertEqual(typedPrim.GetTypeName(), 'TestTypedSchemaForAutoApply')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestMultiApplyAPI:fallback", 
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
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
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
             'TestTypedSchemaForAutoApply'])

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
        self.assertEqual(prim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:bar",
                          "BogusTypeName", "TestMultiApplyAPI"])
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


if __name__ == "__main__":
    unittest.main()
