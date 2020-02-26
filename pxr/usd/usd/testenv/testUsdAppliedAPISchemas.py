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

class TestUsdSchemaRegistry(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdAppliedAPISchemas", \
            "Failed to load expected test plugin"
    
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

        # Find the prim definition for the test multi apply schema. It has
        # some properties defined. Note that the properties in the multi apply
        # definition are not prefixed yet.
        multiApplyAPIDef = Usd.SchemaRegistry().FindAppliedAPIPrimDefinition(
            "TestMultiApplyAPI")
        self.assertTrue(multiApplyAPIDef)
        self.assertEqual(multiApplyAPIDef.GetPropertyNames(), [
            "bool_attr", "token_attr", "relationship"])

        # Find the prim definition for the concrete prim type with fallback
        # API schemas. You can query its API schemas and it will have properties
        # from those schemas already.
        primDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "TestWithFallbackAppliedSchema")
        self.assertTrue(primDef)
        self.assertEqual(primDef.GetAppliedAPISchemas(), [
            "TestMultiApplyAPI:fallback", "TestSingleApplyAPI"])
        self.assertEqual(sorted(primDef.GetPropertyNames()), [
            "multi:fallback:bool_attr", 
            "multi:fallback:relationship",
            "multi:fallback:token_attr", 
            "single:bool_attr",
            "single:relationship", 
            "single:token_attr", 
            "testAttr", 
            "testRel"])
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
        self.assertEqual(untypedPrim.GetPropertyNames(), [])

        # Add an api schema to the prim's metadata.
        l = Sdf.TokenListOp()
        l.explicitItems = ["TestSingleApplyAPI"]
        untypedPrim.SetMetadata("apiSchemas", l)

        # Prim still has no type but does have applied schemas
        self.assertEqual(untypedPrim.GetTypeName(), '')
        self.assertEqual(untypedPrim.GetAppliedSchemas(), ["TestSingleApplyAPI"])
        self.assertTrue(untypedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")))

        # The prim has properties from the applied schema and value resolution
        # returns the applied schema's property fallback value.
        self.assertEqual(untypedPrim.GetPropertyNames(), [
            "single:bool_attr", "single:relationship", "single:token_attr"])
        self.assertEqual(untypedPrim.GetAttribute("single:token_attr").Get(), 
                         "bar")

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
        self.assertEqual(typedPrim.GetPropertyNames(), ["testAttr", "testRel"])

        # Add an api schemas to the prim's metadata.
        l = Sdf.TokenListOp()
        l.explicitItems = ["TestSingleApplyAPI", "TestMultiApplyAPI:garply"]
        typedPrim.SetMetadata("apiSchemas", l)

        # Prim has the same type and now has API schemas. The properties have
        # been expanded to include properties from the API schemas
        self.assertEqual(typedPrim.GetTypeName(), 'TestTypedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
                         ["TestSingleApplyAPI", "TestMultiApplyAPI:garply"])
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")))
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiApplyAPI")))
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
                         ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI"])
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")))
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiApplyAPI")))
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
        l = Sdf.TokenListOp()
        l.explicitItems = ["TestMultiApplyAPI:garply"]
        typedPrim.SetMetadata("apiSchemas", l)

        # Prim has the same type and now has both its original API schemas and
        # the new one. Note that the new schema was added using an explicit 
        # list op but was still appended to the original list. Fallback API 
        # schemas cannot be deleted and any authored API schemas will always be
        # appended to the fallbacks.
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithFallbackAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
            ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI", 
             "TestMultiApplyAPI:garply"])
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")))
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiApplyAPI")))

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
                         ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI"])
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")))
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiApplyAPI")))
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
        l = Sdf.TokenListOp()
        l.explicitItems = ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI"]
        typedPrim.SetMetadata("apiSchemas", l)

        # Prim has the same type and now has both its original API schemas and
        # plus the same schemas again appended to the list (i.e. both schemas
        # now show up twice). 
        self.assertEqual(typedPrim.GetTypeName(), 'TestWithFallbackAppliedSchema')
        self.assertEqual(typedPrim.GetAppliedSchemas(), 
            ["TestMultiApplyAPI:fallback", "TestSingleApplyAPI", 
             "TestMultiApplyAPI:fallback", "TestSingleApplyAPI"])
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestSingleApplyAPI")))
        self.assertTrue(typedPrim.HasAPI(
            Tf.Type(Usd.SchemaBase).FindDerivedByName("TestMultiApplyAPI")))

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

if __name__ == "__main__":
    unittest.main()
