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

# Helper for verifying change procesing related to fallback prim type metadata
# changes. This is used to wrap a call that will change the layer metadata and
# verifies that the ObjectsChanged notice is sent and whether the change caused
# all prims to be resynced or not. 
class ChangeNoticeVerifier(object):
    def __init__(self, testRunner, expectedResyncAll=False):
        self.testRunner = testRunner
        self.expectedResyncAll = expectedResyncAll
        self.receivedNotice = False
    def __enter__(self):
        self._listener = Tf.Notice.RegisterGlobally(
            'UsdNotice::ObjectsChanged', self._OnNotice)
        return self
    def __exit__(self, exc_type, exc_val, exc_tb):
        self._listener.Revoke()
        # Verify that we actually did receive a notice on exit, otherwise we'd
        # end up with a false positive if no notice got sent
        self.testRunner.assertTrue(self.receivedNotice)

    def _OnNotice(self, notice, sender):
        self.receivedNotice = True
        # If we expected a resync all the root path will be in the notices
        # resynced paths, otherwise it will be its changed info only paths.
        if self.expectedResyncAll:
            self.testRunner.assertEqual(notice.GetResyncedPaths(), 
                                        [Sdf.Path('/')])
            self.testRunner.assertEqual(notice.GetChangedInfoOnlyPaths(), [])
        else :
            self.testRunner.assertEqual(notice.GetResyncedPaths(), [])
            self.testRunner.assertEqual(notice.GetChangedInfoOnlyPaths(), 
                                        [Sdf.Path('/')])

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

        def _VerifyFallbacksInStageMetdata(typeName, expectedFallbacksList):
            fallbacks = stage.GetMetadataByDictKey("fallbackPrimTypes", typeName)
            if expectedFallbacksList is None:
                self.assertIsNone(fallbacks)
            else:
                self.assertEqual(fallbacks, Vt.TokenArray(expectedFallbacksList))

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
        _VerifyFallbacksInStageMetdata("ValidType_1", None)
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
        _VerifyFallbacksInStageMetdata("ValidType_2", ["ValidType_1"])
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
        _VerifyFallbacksInStageMetdata("InvalidType_1", None)
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_1")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_1")
        self.assertEqual(primTypeInfo.GetSchemaType(), Tf.Type.Unknown)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "")
        self.assertFalse(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), emptyPrimDef)

        # InvalidPrim_2 : This prim's type is not a valid schema type, but the
        # type has a single fallback defined in metadata which is a valid type.
        # This prim's schema type will be this fallback type.
        prim = stage.GetPrimAtPath("/InvalidPrim_2")
        _VerifyFallbacksInStageMetdata("InvalidType_2", ["ValidType_2"])
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_2")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_2")
        self.assertEqual(primTypeInfo.GetSchemaType(), self.validType2)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "ValidType_2")
        self.assertTrue(prim.IsA(self.validType2))
        self.assertTrue(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), validType2PrimDef)

        # InvalidPrim_3 : This prim's type is not a valid schema type, but the
        # type has two fallbacks defined in metadata which are both valid types.
        # This prim's schema type will be the first fallback type in the list.
        prim = stage.GetPrimAtPath("/InvalidPrim_3")
        _VerifyFallbacksInStageMetdata("InvalidType_3", 
                                       ["ValidType_1", "ValidType_2"])
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_3")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_3")
        self.assertEqual(primTypeInfo.GetSchemaType(), self.validType1)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "ValidType_1")
        self.assertTrue(prim.IsA(self.validType1))
        self.assertTrue(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), validType1PrimDef)

        # InvalidPrim_4 : This prim's type is not a valid schema type, but the
        # type has two fallbacks defined in metadata. The first fallback type
        # is itself invalid, but second is a valid type. This prim's schema type
        # will be the second fallback type in the list.
        prim = stage.GetPrimAtPath("/InvalidPrim_4")
        _VerifyFallbacksInStageMetdata("InvalidType_4", 
                                       ["InvalidType_3", "ValidType_2"])
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_4")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_4")
        self.assertEqual(primTypeInfo.GetSchemaType(), self.validType2)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "ValidType_2")
        self.assertTrue(prim.IsA(self.validType2))
        self.assertTrue(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), validType2PrimDef)

        # InvalidPrim_5 : This prim's type is not a valid schema type, but the
        # type has two fallbacks defined in metadata. However, both of these 
        # types are also invalid types. This will have an invalid schema type.
        prim = stage.GetPrimAtPath("/InvalidPrim_5")
        _VerifyFallbacksInStageMetdata("InvalidType_5", 
                                       ["InvalidType_3", "InvalidType_2"])
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_5")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_5")
        self.assertEqual(primTypeInfo.GetSchemaType(), Tf.Type.Unknown)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "")
        self.assertFalse(prim.IsA(Usd.Typed))
        _VerifyPrimDefsSame(prim.GetPrimDefinition(), emptyPrimDef)

        # InvalidPrim_WithAPISchemas_1 : Has no fallbacks defined in metadata 
        # and its type name is not a valid schema. This will have an invalid 
        # schema type, but the API schemas will still be applied
        prim = stage.GetPrimAtPath("/InvalidPrim_WithAPISchemas_1")
        _VerifyFallbacksInStageMetdata("InvalidType_1", None)
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
        Usd.CollectionAPI.Apply(otherPrim, "foo")
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

        # InvalidPrim_2 : This prim's type is not a valid schema type, but the
        # type has a single fallback defined in metadata which is a valid type.
        # This prim's schema type will be this fallback type and the API schemas
        # will be applied over the fallbacktype.
        prim = stage.GetPrimAtPath("/InvalidPrim_WithAPISchemas_2")
        _VerifyFallbacksInStageMetdata("InvalidType_2", ["ValidType_2"])
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_2")
        primTypeInfo = prim.GetPrimTypeInfo()
        self.assertEqual(primTypeInfo.GetTypeName(), "InvalidType_2")
        self.assertEqual(primTypeInfo.GetSchemaType(), self.validType2)
        self.assertEqual(primTypeInfo.GetSchemaTypeName(), "ValidType_2")
        self.assertEqual(primTypeInfo.GetAppliedAPISchemas(), 
                         ["CollectionAPI:foo"])
        self.assertTrue(prim.IsA(self.validType2))
        self.assertTrue(prim.IsA(Usd.Typed))
        self.assertEqual(prim.GetAppliedSchemas(), ["CollectionAPI:foo"])

        # Create a new prim using the fallback typename and the same API schemas
        # as the above. Verify that because these two prims have the same 
        # effective schema type and applied schemas, that they use the same
        # prim definition even though their type names differ.
        otherPrim = stage.DefinePrim("/Valid2WithCollection", "ValidType_2")
        Usd.CollectionAPI.Apply(otherPrim, "foo")
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

    def test_Sublayer(self):
        # Create a new layer with same layer above as a sublayer. We don't 
        # compose sublayer metadata in stage level metadata so we don't get
        # the fallback types like in the above test case.
        layer = Sdf.Layer.CreateAnonymous()
        layer.subLayerPaths.append('WithFallback.usda')
        stage = Usd.Stage.Open(layer)

        # The two prims with valid schema type names are still valid types.
        prim = stage.GetPrimAtPath("/ValidPrim_1")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "ValidType_1")
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaType(), 
                         self.validType1)
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaTypeName(), 
                         "ValidType_1")

        prim = stage.GetPrimAtPath("/ValidPrim_2")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "ValidType_2")
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaType(), 
                         self.validType2)
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaTypeName(), 
                         "ValidType_2")

        # All of the prims with invalid types have no schema type since we 
        # have no fallbacks from the sublayer.
        for primPath, typeName in [("/InvalidPrim_1", "InvalidType_1"),
                                   ("/InvalidPrim_2", "InvalidType_2"),
                                   ("/InvalidPrim_3", "InvalidType_3"),
                                   ("/InvalidPrim_4", "InvalidType_4"),
                                   ("/InvalidPrim_5", "InvalidType_5")] :
            prim = stage.GetPrimAtPath(primPath)
            self.assertTrue(prim)
            self.assertEqual(prim.GetTypeName(), typeName)
            self.assertEqual(prim.GetPrimTypeInfo().GetSchemaType(), 
                             Tf.Type.Unknown)
            self.assertEqual(prim.GetPrimTypeInfo().GetSchemaTypeName(), "")

    def test_FallbackAuthoring(self):
        # Test the manual authoring of fallback type metadata on the stage.
        stage = Usd.Stage.Open("WithFallback.usda")
        self.assertTrue(stage)

        # InvalidPrim_1 : Has no fallbacks defined in metadata and its type name
        # is not a valid schema. This will have an invalid schema type.
        prim = stage.GetPrimAtPath("/InvalidPrim_1")
        self.assertTrue(prim)
        self.assertEqual(prim.GetTypeName(), "InvalidType_1")
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaType(), 
                         Tf.Type.Unknown)
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaTypeName(), "")

        # Author a fallback value for InvalidType_1 in the stage's 
        # fallbackPrimTypes metadata dictionary. This will resync all prims
        # on the stage.
        with ChangeNoticeVerifier(self, expectedResyncAll=True):
            stage.SetMetadataByDictKey("fallbackPrimTypes", "InvalidType_1",
                                       Vt.TokenArray(["ValidType_2"]))

        # InvalidPrim_1 now has a schema type from the fallback type.
        self.assertEqual(prim.GetTypeName(), "InvalidType_1")
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaType(), 
                         self.validType2)
        self.assertEqual(prim.GetPrimTypeInfo().GetSchemaTypeName(), 
                         "ValidType_2")

    def test_WritingSchemaFallbacks(self):
        # Test the writing of fallback types defined in the schema through the
        # stage API.

        # Our test schema plugin has some fallbacks defined. Verify that we
        # can get these as dictionary from the schema registry
        schemaFallbacks = {
            "ValidType_1" : Vt.TokenArray(["FallbackType_1"]),
            "ValidType_2" : Vt.TokenArray(["FallbackType_2", "FallbackType_1"])
        }
        self.assertEqual(Usd.SchemaRegistry().GetFallbackPrimTypes(),
                         schemaFallbacks)

        # Create a new empty layer and stage from that layer. There will be
        # no fallbackPrimTypes set on the newly opened stage
        layer = Sdf.Layer.CreateNew("tempNew.usda")
        self.assertTrue(layer)
        stage = Usd.Stage.Open(layer)
        self.assertTrue(stage)
        self.assertFalse(stage.GetMetadata("fallbackPrimTypes"))
        self.assertFalse(layer.GetPrimAtPath('/').GetInfo('fallbackPrimTypes'))

        # Create a prim spec on the layer and save the layer itself. There will
        # be no fallback metadata on the stage since we saved through the
        # layer's API.
        spec = Sdf.PrimSpec(layer, "ValidPrim", Sdf.SpecifierDef, "ValidType_2")
        self.assertTrue(stage.GetPrimAtPath("/ValidPrim"))
        layer.Save()
        self.assertFalse(stage.GetMetadata("fallbackPrimTypes"))
        self.assertFalse(layer.GetPrimAtPath('/').GetInfo('fallbackPrimTypes'))

        # Now write the schema fallbacks to the layer using the stage API. This 
        # will write the schema fallbacks dictionary to the root layer. Verify 
        # that this layer change does not a cause a full stage resync.
        with ChangeNoticeVerifier(self, expectedResyncAll=False):
            stage.WriteFallbackPrimTypes()
        self.assertEqual(stage.GetMetadata("fallbackPrimTypes"), 
                         schemaFallbacks)
        self.assertEqual(layer.GetPrimAtPath('/').GetInfo('fallbackPrimTypes'),
                         schemaFallbacks)

        # Now manually author a different dictionary of fallback to the root
        # layer. This will trigger a full stage resync and changes the stage's
        # fallback types.
        newFallbacks = {
            "ValidType_1" : Vt.TokenArray(["ValidType_2, FallbackType_2"]),
            "InValidType_1" : Vt.TokenArray(["ValidType_1"])
        }
        with ChangeNoticeVerifier(self, expectedResyncAll=True):
            layer.GetPrimAtPath('/').SetInfo('fallbackPrimTypes', 
                                             newFallbacks)
        self.assertEqual(stage.GetMetadata("fallbackPrimTypes"), 
                         newFallbacks)
        self.assertEqual(layer.GetPrimAtPath('/').GetInfo('fallbackPrimTypes'),
                         newFallbacks)

        # Write the schema fallbacks again and verify that it will add fallback 
        # types to the metadata dictionary from the schema registry but only for
        # types that aren't already in the dictionary.
        with ChangeNoticeVerifier(self, expectedResyncAll=False):
            stage.WriteFallbackPrimTypes()
        postSaveFallbacks = {
            "ValidType_1" : Vt.TokenArray(["ValidType_2, FallbackType_2"]),
            "ValidType_2" : Vt.TokenArray(["FallbackType_2", "FallbackType_1"]),
            "InValidType_1" : Vt.TokenArray(["ValidType_1"])
        }
        self.assertEqual(stage.GetMetadata("fallbackPrimTypes"), 
                         postSaveFallbacks)
        self.assertEqual(layer.GetPrimAtPath('/').GetInfo('fallbackPrimTypes'),
                         postSaveFallbacks)


if __name__ == "__main__":
    unittest.main()
