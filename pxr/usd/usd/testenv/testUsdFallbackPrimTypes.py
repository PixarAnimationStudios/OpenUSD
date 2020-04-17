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

class TestUsdFallbackPrimTypes(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdFallbackPrimTypes", \
            "Failed to load expected test plugin"
    
        cls.validType1 = Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName(
            "ValidType_1")
        assert cls.validType1
        cls.validType2 = Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName(
            "ValidType_2")
        assert cls.validType2
                
    def test_OpenLayerWithFallbackTypes(self):
        stage = Usd.Stage.Open("WithFallback.usda")
        self.assertTrue(stage)

        emptyPrimDef = Usd.SchemaRegistry().GetEmptyPrimDefinition()
        validType1PrimDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "ValidType_1")
        validType2PrimDef = Usd.SchemaRegistry().FindConcretePrimDefinition(
            "ValidType_2")

        def _VerifyPrimDefsSame(primDef1, primDef2):
            self.assertEqual(primDef1.GetAppliedAPISchemas(), 
                             primDef2.GetAppliedAPISchemas())
            self.assertEqual(primDef1.GetPropertyNames(), 
                             primDef2.GetPropertyNames())
            for propName in primDef1.GetPropertyNames():
                self.assertEqual(primDef1.GetSchemaPropertySpec(propName), 
                                 primDef2.GetSchemaPropertySpec(propName))
            self.assertEqual(primDef1.ListMetadataFields(), 
                             primDef2.ListMetadataFields())
            for fieldName in primDef1.ListMetadataFields():
                self.assertEqual(primDef1.GetMetadata(fieldName), 
                                 primDef2.GetMetadata(fieldName))

        # ValidPrim_1 : Has no fallbacks defined in metadata but its type name
        # is a valid schema so it doesn't matter. This is the most typical case.
        prim = stage.GetPrimAtPath("/ValidPrim_1")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "ValidType_1")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "ValidType_1")
        self.assertEqual(primTypeInfo.GetSchemaType(), self.validType1)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "ValidType_1")
        self.assertTrue(prim.IsA(self.validType1))
        self.assertTrue(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), validType1PrimDef)

        # ValidPrim_2 : Has fallbacks defined in metadata but its type name
        # is a valid schema, so it ignores fallbacks.
        prim = stage.GetPrimAtPath("/ValidPrim_2")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "ValidType_2")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "ValidType_2")
        self.assertEqual(primTypeInfo.GetSchemaType(), self.validType2)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "ValidType_2")
        self.assertTrue(prim.IsA(self.validType2))
        self.assertTrue(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), validType2PrimDef)

        # InvalidPrim_1 : Has no fallbacks defined in metadata and its type name
        # is not a valid schema. This will have an invalid schema type.
        prim = stage.GetPrimAtPath("/InvalidPrim_1")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_1")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_1")
        self.assertEqual(primTypeInfo.GetSchemaType(), Tf.Type.Unknown)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "")
        self.assertFalse(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), emptyPrimDef)

        # InvalidPrim_WithAPISchemas_1 : Has no fallbacks defined in metadata 
        # and its type name is not a valid schema. This will have an invalid 
        # schema type, but the API schemas will still be applied
        prim = stage.GetPrimAtPath("/InvalidPrim_WithAPISchemas_1")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_1")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_1")
        self.assertEqual(primTypeInfo.GetSchemaType(), Tf.Type.Unknown)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "")
        self.assertEqual(primTypeInfo.GetAppliedAPISchemas(), 
                         ["CollectionAPI:foo"])
        self.assertFalse(prim.IsA(Usd.Typed))
        self.assertEqual(prim.GetAppliedSchemas(), ["CollectionAPI:foo"])

        # Create a new prim with an empty typename and the same API schemas
        # as the above. Verify that because these two prims have the same 
        # effective schema type and applied schemas, that they use the same
        # prim definition even though their type names differ.
        otherPrim = stage.DefinePrim("/EmptyWithCollection", "")
        Usd.CollectionAPI.ApplyCollection(otherPrim, "foo")
        otherPrimTypeInfo = otherPrim.GetPrimTypeInfo()
        self.assertNotEqual(otherPrim.GetTypeName(), prim.GetTypeName())
        self.assertNotEqual(otherPrimTypeInfo, primTypeInfo)
        self.assertEqual(otherPrimTypeInfo.GetSchemaTypeName(), 
                         primTypeInfo.GetSchemaTypeName())
        self.assertEqual(otherPrimTypeInfo.GetSchemaType(), 
                         primTypeInfo.GetSchemaType())
        self.assertEqual(otherPrim.GetAppliedSchemas(), 
                         prim.GetAppliedSchemas())
        _VerifyPrimDefsSame(otherPrim.GetPrimDefinition(), 
                             prim.GetPrimDefinition())

if __name__ == "__main__":
    unittest.main()
