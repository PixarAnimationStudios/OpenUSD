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
from pxr import Usd, Sdf, Tf, Plug

class TestUsdFlattenProperties(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        pr = Plug.Registry()
        testPlugins = pr.RegisterPlugins(os.path.abspath("resources"))
        assert len(testPlugins) == 1, \
            "Failed to load expected test plugin"
        assert testPlugins[0].name == "testUsdFlattenProperties", \
            "Failed to load expected test plugin"

    def setUp(self):
        self.stage = Usd.Stage.Open("root.usda")
        self.rootLayer = self.stage.GetRootLayer()
        self.subLayer = self.stage.GetLayerStack()[-1]

    def tearDown(self):
        self.stage.Reload()

    def _GetDefault(self, attr):
        return attr.Get()

    def _GetTimeSamples(self, attr):
        return dict([(s, attr.Get(s)) for s in attr.GetTimeSamples()])

    def _GetDefaultInLayer(self, layer, path):
        return layer.GetAttributeAtPath(path).default
            
    def _GetTimeSamplesInLayer(self, layer, path):
        return dict([(s, layer.QueryTimeSample(path, s))
                     for s in layer.ListTimeSamplesForPath(path)])

    def _VerifyExpectedFields(self, attrSpec, expectedFields):
        for field in attrSpec.ListInfoKeys():
            self.assertTrue(field in expectedFields, 
                            "{0} not in expected fields".format(field))

        for (field, value) in expectedFields.items():
            self.assertTrue(field in attrSpec.ListInfoKeys(),
                            "{0} not in property fields".format(field))
            self.assertEqual(attrSpec.GetInfo(field), value,
                             "Got {0}, expected {1} for {2}"
                             .format(attrSpec.GetInfo(field), value, field))

    def test_Basic(self):
        """Tests basic functionality."""
        expectedFields = {
            "custom": False,
            "default": 1,
            "displayName": "foo",
            "timeSamples": { 0: 100, 10: 1000 },
            "typeName": "double",
            "variability": Sdf.VariabilityVarying,
        }

        srcAttr = self.stage.GetPrimAtPath("/Basic").GetAttribute("a")
        dstPrim = self.stage.DefinePrim("/BasicCopy")

        # Flatten to property beneath dstPrim with the same name
        dstAttr = srcAttr.FlattenTo(dstPrim)
        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertEqual(dstAttr.GetPath(), "/BasicCopy.a")
        self.assertEqual(dstAttrSpec.path, "/BasicCopy.a")
        self._VerifyExpectedFields(dstAttrSpec, expectedFields)

        # Flatten to property beneath dstPrim with different name
        dstAttr = srcAttr.FlattenTo(dstPrim, "b")
        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertEqual(dstAttr.GetPath(), "/BasicCopy.b")
        self.assertEqual(dstAttrSpec.path, "/BasicCopy.b")
        self._VerifyExpectedFields(dstAttrSpec, expectedFields)

        # Flatten to given property. Note that this clears out the
        # pre-existing property spec.
        dstAttr = dstPrim.CreateAttribute("c", Sdf.ValueTypeNames.String, 
                                          True, Sdf.VariabilityUniform)
        dstAttr = srcAttr.FlattenTo(dstAttr)
        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertEqual(dstAttr.GetPath(), "/BasicCopy.c")
        self.assertEqual(dstAttrSpec.path, "/BasicCopy.c")
        self._VerifyExpectedFields(dstAttrSpec, expectedFields)

    def test_Errors(self):
        """Tests error cases."""
        
        # Flattening to an invalid prim is an error
        with self.assertRaises(Tf.ErrorException):
            srcAttr = self.stage.GetPrimAtPath("/Basic").GetAttribute("a")
            srcAttr.FlattenTo(self.stage.GetPrimAtPath("/Bogus"))

        # Flattening an invalid property is an error
        with self.assertRaises(Tf.ErrorException):
            srcAttr = self.stage.GetPrimAtPath("/Basic").GetAttribute("foo")
            srcAttr.FlattenTo(self.stage.GetPrimAtPath("/Basic"))

        # Flattening an attribute to an existing relationship or
        # a relationship to an existing attribute is an error.
        srcRel = self.stage.OverridePrim("/TestErrors").CreateRelationship("rel")
        with self.assertRaises(Tf.ErrorException):
            srcAttr = self.stage.GetPrimAtPath("/Basic").GetAttribute("a")
            srcAttr.FlattenTo(srcRel)
        with self.assertRaises(Tf.ErrorException):
            srcAttr = self.stage.GetPrimAtPath("/Basic").GetAttribute("a")
            srcRel.FlattenTo(srcAttr)

    def test_FlattenWithOffsets(self):
        """Tests that layer offsets are taken into account when flattening
        attribute time samples."""
        srcAttr = self.stage.GetPrimAtPath("/OffsetTimeSamples") \
                            .GetAttribute("a")

        if Usd.UsesInverseLayerOffset():
            self.assertEqual(self._GetTimeSamples(srcAttr), 
                             { -10: 100, 0: 1000 })
        else:
            self.assertEqual(self._GetTimeSamples(srcAttr), 
                             { 10: 100, 20: 1000 })

        dstAttr = srcAttr.FlattenTo(
            self.stage.OverridePrim("/OffsetTimeSamplesRoot"))

        if Usd.UsesInverseLayerOffset():
            self.assertEqual(
                self._GetTimeSamples(dstAttr), { -10: 100, 0: 1000 })
            self.assertEqual(
                self._GetTimeSamplesInLayer(
                    self.rootLayer, "/OffsetTimeSamplesRoot.a"),
                { -10: 100, 0: 1000 })
        else:
            self.assertEqual(
                self._GetTimeSamples(dstAttr), { 10: 100, 20: 1000 })
            self.assertEqual(
                self._GetTimeSamplesInLayer(
                    self.rootLayer, "/OffsetTimeSamplesRoot.a"),
                { 10: 100, 20: 1000 })
            
        with Usd.EditContext(
            self.stage, self.stage.GetEditTargetForLocalLayer(self.subLayer)):
            dstAttr = srcAttr.FlattenTo(
                self.stage.OverridePrim("/OffsetTimeSamplesSublayer"))
            
            if Usd.UsesInverseLayerOffset():
                self.assertEqual(
                    self._GetTimeSamples(dstAttr), { -10: 100, 0: 1000 })
                self.assertEqual(
                    self._GetTimeSamplesInLayer(
                        self.subLayer, "/OffsetTimeSamplesSublayer.a"),
                    { 0: 100, 10: 1000 })
            else:
                self.assertEqual(
                    self._GetTimeSamples(dstAttr), { 10: 100, 20: 1000 })
                self.assertEqual(
                    self._GetTimeSamplesInLayer(
                        self.subLayer, "/OffsetTimeSamplesSublayer.a"),
                    { 0: 100, 10: 1000 })

    def test_DefaultAndTimeSamples(self):
        """Tests that properties with both a default value and time samples
        in different sublayers are flattened so that the result has the
        same resolved values as the source."""
        srcAttr = self.stage.GetPrimAtPath("/WeakerTimeSamples") \
                            .GetAttribute("a")

        self.assertEqual(self._GetDefault(srcAttr), 1)
        self.assertEqual(self._GetTimeSamples(srcAttr), {})

        dstAttr = srcAttr.FlattenTo(
            self.stage.GetPrimAtPath("/WeakerTimeSamples"), "b")
        self.assertEqual(self._GetDefault(dstAttr), 1)
        self.assertEqual(self._GetTimeSamples(dstAttr), {})
        self.assertEqual(
            self._GetDefaultInLayer(self.rootLayer, "/WeakerTimeSamples.b"), 1.0)
        self.assertEqual(
            self._GetTimeSamplesInLayer(self.rootLayer, "/WeakerTimeSamples.b"), {})

        srcAttr = self.stage.GetPrimAtPath("/StrongerTimeSamples") \
                            .GetAttribute("a")

        self.assertEqual(self._GetDefault(srcAttr), 1)
        self.assertEqual(self._GetTimeSamples(srcAttr), {0: 100, 10: 1000})

        dstAttr = srcAttr.FlattenTo(
            self.stage.GetPrimAtPath("/StrongerTimeSamples"), "b")
        self.assertEqual(self._GetDefault(dstAttr), 1)
        self.assertEqual(self._GetTimeSamples(dstAttr), {0: 100, 10: 1000})
        self.assertEqual(
            self._GetDefaultInLayer(self.rootLayer, "/StrongerTimeSamples.b"), 1.0)
        self.assertEqual(
            self._GetTimeSamplesInLayer(self.rootLayer, "/StrongerTimeSamples.b"), 
            {0: 100, 10: 1000})

    def test_Builtins(self):
        """Tests flattening builtin properties."""
        srcPrim = self.stage.GetPrimAtPath("/FlattenPropertyTest")
        srcAttr = srcPrim.GetAttribute("builtinAttr")

        # Flatten a builtin attribute with no authored opinions to a prim 
        # with the same type. Since the fallback values are the same for the
        # source and destination attributes, no fallbacks should be written;
        # just an attribute spec with the same variability and typename.
        dstPrim = self.stage.DefinePrim("/FlattenPropertyTestCopy", 
                                        srcPrim.GetTypeName())

        dstAttr = srcAttr.FlattenTo(dstPrim)
        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec, 
            { "custom": False, 
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })

        # Author some values on the newly-created attribute, then flatten
        # again -- this time, into the stronger session layer. This should
        # cause fallback values to be baked out to ensure they win over the
        # authored values in the weaker layer.
        dstAttrSpec.SetInfo("default", "default_2")
        dstAttrSpec.SetInfo("testCustomMetadata", "foo")

        with Usd.EditContext(self.stage, self.stage.GetSessionLayer()):
            dstAttr = srcAttr.FlattenTo(dstPrim)

        dstAttrSpec = self.stage.GetSessionLayer() \
                                .GetAttributeAtPath(dstAttr.GetPath())
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec, 
            { "custom": False, 
              "default": "default value",
              "testCustomMetadata": "garply",
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })

        # Flatten builtin attribute to non-builtin attribute. This should
        # cause all fallbacks to be authored, since the destination attribute
        # has no fallback opinions.
        dstAttr = srcAttr.FlattenTo(dstPrim, "nonbuiltinAttr")
        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec, 
            { "custom": False, 
              "default": "default value",
              "displayName": "display name",
              "testCustomMetadata": "garply",
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })

    def test_FlattenOverSelf(self):
        """Tests that a property can be flattened over itself to
        loft composed opinions into the current edit target."""
        srcAttr = self.stage.GetPrimAtPath("/FlattenOverSelf") \
                            .GetAttribute("builtinAttr")

        dstAttr = srcAttr.FlattenTo(srcAttr)
        dstAttrSpec = \
            self.rootLayer.GetAttributeAtPath("/FlattenOverSelf.builtinAttr")
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec,
            { "custom": False, 
              "default": "sub authored value",
              "testCustomMetadata": "root authored metadata",
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })

    def test_RemapTargetPaths(self):
        """Tests that relationship target and attribute connection
        paths that point to an object within the source prim are
        remapped when flattened."""
        srcRel = self.stage.GetPrimAtPath("/RemapTargetPaths") \
                           .GetRelationship("a")
        
        dstPrim = self.stage.DefinePrim("/RemapTargetPathsCopy")
        dstRel = srcRel.FlattenTo(dstPrim)
        self.assertEqual(dstRel.GetPath(), "/RemapTargetPathsCopy.a")

        dstRelSpec = \
            self.rootLayer.GetRelationshipAtPath("/RemapTargetPathsCopy.a")
        self.assertEqual(list(dstRelSpec.targetPathList.explicitItems), 
                         ["/OutsideTarget",
                          "/RemapTargetPathsCopy/A", 
                          "/RemapTargetPathsCopy/A.b", 
                          "/RemapTargetPathsCopy.b"])

    def test_FlattenInstanceProperty(self):
        """Tests flattening properties from instancing masters"""
        def ExplicitPathListOp(paths):
            listOp = Sdf.PathListOp()
            listOp.explicitItems = paths
            return listOp

        srcAttr = self.stage.GetPrimAtPath("/FlattenInstanceProperty") \
                            .GetMaster().GetChild("Instance") \
                            .GetAttribute("builtinAttr")

        # Flattening a property from a prim in a master should behave
        # the same as with any other property for the most part.
        dstPrim = self.stage.DefinePrim("/FlattenInstanceProperty_1",
                                        "FlattenPropertyTest")
        dstAttr = srcAttr.FlattenTo(dstPrim)
        self.assertEqual(dstAttr.GetPath(),
                         "/FlattenInstanceProperty_1.builtinAttr")

        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec,
            { "connectionPaths": ExplicitPathListOp([
                "/FlattenInstanceProperty_1.builtinAttr"]),
              "custom": False,
              "default": "instance authored value",
              "typeName": "string",
              "variability": Sdf.VariabilityVarying })

        # However, if the property has an attribute connection or relationship
        # target that points to an object in a master that can't be remapped,
        # a warning is issued and that path is ignored.
        srcAttr = self.stage.GetPrimAtPath("/FlattenInstanceProperty") \
                            .GetMaster().GetChild("Instance2") \
                            .GetAttribute("builtinAttr")
        dstPrim = self.stage.DefinePrim("/FlattenInstanceProperty_2",
                                        "FlattenPropertyTest")
        dstAttr = srcAttr.FlattenTo(dstPrim)
        self.assertEqual(dstAttr.GetPath(),
                         "/FlattenInstanceProperty_2.builtinAttr")

        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec,
            { "custom": False,
              "default": "instance authored value 2",
              "typeName": "string",
              "variability": Sdf.VariabilityVarying })

        # Using an instance proxy avoids this problem.
        srcAttr = self.stage.GetPrimAtPath(
            "/FlattenInstanceProperty/Instance2").GetAttribute("builtinAttr")
        dstPrim = self.stage.DefinePrim("/FlattenInstanceProperty_3",
                                        "FlattenPropertyTest")
        dstAttr = srcAttr.FlattenTo(dstPrim)
        self.assertEqual(dstAttr.GetPath(),
                         "/FlattenInstanceProperty_3.builtinAttr")

        dstAttrSpec = self.rootLayer.GetAttributeAtPath(dstAttr.GetPath())
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec,
            { "connectionPaths": ExplicitPathListOp([
                "/FlattenInstanceProperty/Instance.builtinAttr"]),
              "custom": False,
              "default": "instance authored value 2",
              "typeName": "string",
              "variability": Sdf.VariabilityVarying })
        
if __name__ == "__main__":
    unittest.main()
