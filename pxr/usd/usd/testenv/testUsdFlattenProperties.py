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
        self.srcStage = Usd.Stage.Open("root.usda")
        # setup a destination stage different from the src stage
        self.stage2 = Usd.Stage.Open("root2.usda")
        # setup expected states for offset tests 
        self.expectedTimeSampleOffsetSublayer = {
                self.srcStage: { 0: 100, 10: 1000 },
                self.stage2: { -10: 100, 0: 1000 }
        }
        self.expectedTimeCodeDefaultValueOffsetSubLayer = {
                self.srcStage: Sdf.TimeCode(0),
                self.stage2: Sdf.TimeCode(-10)
        }
        self.expectedTimeCodeTimeSampleOffsetSubLayer = {
                self.srcStage: { 0: Sdf.TimeCode(100), 10: Sdf.TimeCode(1000) },

                self.stage2: { -10.0: Sdf.TimeCode(90), 0.0: Sdf.TimeCode(990) }
        }

    def tearDown(self):
        for stage in [self.srcStage, self.stage2]:
            stage.Reload()

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

        srcAttr = self.srcStage.GetPrimAtPath("/Basic").GetAttribute("a")

        for dstStage in [self.srcStage, self.stage2]:
            dstPrim = dstStage.DefinePrim("/BasicCopy")

            # Flatten to property beneath dstPrim with the same name
            dstAttr = srcAttr.FlattenTo(dstPrim)
            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
            self.assertEqual(dstAttr.GetPath(), "/BasicCopy.a")
            self.assertEqual(dstAttrSpec.path, "/BasicCopy.a")
            self._VerifyExpectedFields(dstAttrSpec, expectedFields)

            # Flatten to property beneath dstPrim with different name
            dstAttr = srcAttr.FlattenTo(dstPrim, "b")
            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
            self.assertEqual(dstAttr.GetPath(), "/BasicCopy.b")
            self.assertEqual(dstAttrSpec.path, "/BasicCopy.b")
            self._VerifyExpectedFields(dstAttrSpec, expectedFields)

            # Flatten to given property. Note that this clears out the
            # pre-existing property spec.
            dstAttr = dstPrim.CreateAttribute("c", Sdf.ValueTypeNames.String, 
                                              True, Sdf.VariabilityUniform)
            dstAttr = srcAttr.FlattenTo(dstAttr)
            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
            self.assertEqual(dstAttr.GetPath(), "/BasicCopy.c")
            self.assertEqual(dstAttrSpec.path, "/BasicCopy.c")
            self._VerifyExpectedFields(dstAttrSpec, expectedFields)

    def test_Errors(self):
        """Tests error cases."""

        # Flattening to an invalid prim is an error
        for dstStage in [self.srcStage, self.stage2]:
            with self.assertRaises(Tf.ErrorException):
                srcAttr = self.srcStage.GetPrimAtPath("/Basic").GetAttribute(
                        "a")
                srcAttr.FlattenTo(dstStage.GetPrimAtPath("/Bogus"))

            # Flattening an invalid property is an error
            with self.assertRaises(Tf.ErrorException):
                srcAttr = self.srcStage.GetPrimAtPath("/Basic").GetAttribute(
                        "foo")
                srcAttr.FlattenTo(dstStage.GetPrimAtPath("/Basic"))

            # Flattening an attribute to an existing relationship or
            # a relationship to an existing attribute is an error.
            srcRel = self.srcStage.OverridePrim(
                    "/TestErrors").CreateRelationship("rel")
            dstRel = dstStage.OverridePrim(
                    "/TestErrors").CreateRelationship("rel")
            srcAttr = self.srcStage.GetPrimAtPath("/Basic").GetAttribute("a")
            dstAttr = dstStage.GetPrimAtPath("/Basic").GetAttribute("a")
            with self.assertRaises(Tf.ErrorException):
                srcAttr.FlattenTo(dstRel)
            with self.assertRaises(Tf.ErrorException):
                srcRel.FlattenTo(dstAttr)

    def test_FlattenWithOffsets(self):
        """Tests that layer offsets are taken into account when flattening
        attribute time samples."""
        srcAttr = self.srcStage.GetPrimAtPath("/OffsetTimeSamples") \
                            .GetAttribute("a")

        self.assertEqual(self._GetTimeSamples(srcAttr), 
                         { 10: 100, 20: 1000 })

        for dstStage in [self.srcStage, self.stage2]:
            dstAttr = srcAttr.FlattenTo(
                dstStage.OverridePrim("/OffsetTimeSamplesRoot"))

            self.assertEqual(
                self._GetTimeSamples(dstAttr), { 10: 100, 20: 1000 })
            self.assertEqual(
                self._GetTimeSamplesInLayer(
                    dstStage.GetRootLayer(), dstAttr.GetPath()),
                { 10: 100, 20: 1000 })
            
            dstSubLayer = dstStage.GetLayerStack()[-1]
            with Usd.EditContext(
                dstStage, dstStage.GetEditTargetForLocalLayer(dstSubLayer)):
                dstAttr = srcAttr.FlattenTo(
                    dstStage.OverridePrim("/OffsetTimeSamplesSublayer"))
                
                # flattened time samples
                self.assertEqual(
                    self._GetTimeSamples(dstAttr), { 10: 100, 20: 1000 })
                # time samples from the offset sublayer
                self.assertEqual(
                    self._GetTimeSamplesInLayer(dstSubLayer, dstAttr.GetPath()),
                    self.expectedTimeSampleOffsetSublayer[dstStage])

    def test_FlattenTimeCodeWithOffsets(self):
        """Tests that layer offsets are taken into account when flattening
        attribute time samples and defaults of time code value attributes."""

        propName = "a"
        srcAttr = self.srcStage.GetPrimAtPath("/OffsetTimeCodeTimeSamples") \
                            .GetAttribute(propName)

        self.assertEqual(self._GetDefault(srcAttr), Sdf.TimeCode(10))
        self.assertEqual(self._GetTimeSamples(srcAttr), 
                         { 10: Sdf.TimeCode(110), 20: Sdf.TimeCode(1010) })

        rootPrimPath = Sdf.Path("/OffsetTimeCodeTimeSamplesRoot")

        for dstStage in [self.srcStage, self.stage2]:
            dstAttr = srcAttr.FlattenTo(
                dstStage.OverridePrim(rootPrimPath))

            self.assertEqual(self._GetDefault(dstAttr), Sdf.TimeCode(10))
            self.assertEqual(
                self._GetTimeSamples(dstAttr), { 10: Sdf.TimeCode(110), 
                                                 20: Sdf.TimeCode(1010) })
            self.assertEqual(
                self._GetDefaultInLayer(
                    dstStage.GetRootLayer(), rootPrimPath.AppendProperty(
                        propName)), 
                Sdf.TimeCode(10))
            self.assertEqual(
                self._GetTimeSamplesInLayer(
                    dstStage.GetRootLayer(), rootPrimPath.AppendProperty(
                        propName)),
                { 10: Sdf.TimeCode(110), 20: Sdf.TimeCode(1010) })

            subPrimPath = Sdf.Path("/OffsetTimeCodeTimeSamplesSublayer")
            dstSubLayer = dstStage.GetLayerStack()[-1]
            with Usd.EditContext(
                dstStage, dstStage.GetEditTargetForLocalLayer(dstSubLayer)):
                dstAttr = srcAttr.FlattenTo(
                    dstStage.OverridePrim(subPrimPath))

                self.assertEqual(self._GetDefault(dstAttr), Sdf.TimeCode(10))
                self.assertEqual(
                    self._GetTimeSamples(dstAttr), { 10: Sdf.TimeCode(110), 
                                                     20: Sdf.TimeCode(1010) })
                self.assertEqual(
                    self._GetDefaultInLayer(
                        dstSubLayer, subPrimPath.AppendProperty(propName)), 
                    self.expectedTimeCodeDefaultValueOffsetSubLayer[dstStage])
                self.assertEqual(
                    self._GetTimeSamplesInLayer(
                        dstSubLayer, subPrimPath.AppendProperty(propName)),
                    self.expectedTimeCodeTimeSampleOffsetSubLayer[dstStage])

    def test_DefaultAndTimeSamples(self):
        """Tests that properties with both a default value and time samples
        in different sublayers are flattened so that the result has the
        same resolved values as the source."""

        for dstStage in [self.srcStage, self.stage2]:
            srcAttr = self.srcStage.GetPrimAtPath("/WeakerTimeSamples") \
                                .GetAttribute("a")

            self.assertEqual(self._GetDefault(srcAttr), 1)
            self.assertEqual(self._GetTimeSamples(srcAttr), {})
            dstAttr = srcAttr.FlattenTo(
                dstStage.GetPrimAtPath("/WeakerTimeSamples"), "b")
            self.assertEqual(self._GetDefault(dstAttr), 1)
            self.assertEqual(self._GetTimeSamples(dstAttr), {})
            self.assertEqual(
                self._GetDefaultInLayer(dstStage.GetRootLayer(), 
                    "/WeakerTimeSamples.b"), 1.0)
            self.assertEqual(
                self._GetTimeSamplesInLayer(dstStage.GetRootLayer(), 
                    "/WeakerTimeSamples.b"), {})

            srcAttr = self.srcStage.GetPrimAtPath("/StrongerTimeSamples") \
                                .GetAttribute("a")

            self.assertEqual(self._GetDefault(srcAttr), 1)
            self.assertEqual(self._GetTimeSamples(srcAttr), {0: 100, 10: 1000})

            dstAttr = srcAttr.FlattenTo(
                dstStage.GetPrimAtPath("/StrongerTimeSamples"), "b")
            self.assertEqual(self._GetDefault(dstAttr), 1)
            self.assertEqual(self._GetTimeSamples(dstAttr), {0: 100, 10: 1000})
            self.assertEqual(
                self._GetDefaultInLayer(dstStage.GetRootLayer(), 
                    "/StrongerTimeSamples.b"), 1.0)
            self.assertEqual(
                self._GetTimeSamplesInLayer(dstStage.GetRootLayer(), 
                    "/StrongerTimeSamples.b"), 
                {0: 100, 10: 1000})

    def test_Builtins(self):
        """Tests flattening builtin properties."""
        srcPrim = self.srcStage.GetPrimAtPath("/FlattenPropertyTest")
        srcAttr = srcPrim.GetAttribute("builtinAttr")

        for dstStage in [self.srcStage, self.stage2]:
            # Flatten a builtin attribute with no authored opinions to a prim 
            # with the same type. Since the fallback values are the same for the
            # source and destination attributes, no fallbacks should be written;
            # just an attribute spec with the same variability and typename.
            dstPrim = dstStage.DefinePrim("/FlattenPropertyTestCopy", 
                                            srcPrim.GetTypeName())

            dstAttr = srcAttr.FlattenTo(dstPrim)
            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
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

            with Usd.EditContext(dstStage, dstStage.GetSessionLayer()):
                dstAttr = srcAttr.FlattenTo(dstPrim)

            dstAttrSpec = dstStage.GetSessionLayer() \
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
            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
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
        loft composed opinions into the current edit target.
        Noting same property path across different stages are considered
        different."""
        srcAttr = self.srcStage.GetPrimAtPath("/FlattenOverSelf") \
                            .GetAttribute("builtinAttr")

        dstAttr = srcAttr.FlattenTo(srcAttr)
        dstAttrSpec = \
            self.srcStage.GetRootLayer().GetAttributeAtPath(
                    "/FlattenOverSelf.builtinAttr")
        self.assertTrue(dstAttrSpec)
        self._VerifyExpectedFields(
            dstAttrSpec,
            { "custom": False, 
              "default": "sub authored value",
              "testCustomMetadata": "root authored metadata",
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })

        # flattenTo on an attribute of same path across different stage
        dstAttrBeforeFlatten = self.stage2.GetAttributeAtPath(
                '/FlattenOverSelf.builtinAttr')
        dstAttrSpecBeforeFlatten = self.stage2.GetRootLayer(). \
                GetAttributeAtPath('/FlattenOverSelf.builtinAttr')
        self.assertEqual(dstAttrBeforeFlatten.Get(), "before flatten")
        self._VerifyExpectedFields(
            dstAttrSpecBeforeFlatten,
            { "custom": False, 
              "default": "before flatten",
              "testCustomMetadata": "other stage authored metadata",
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })
        dstAttr = srcAttr.FlattenTo(dstAttrBeforeFlatten)
        dstAttrSpec = self.stage2.GetRootLayer(). \
                GetAttributeAtPath('/FlattenOverSelf.builtinAttr')
        self.assertEqual(dstAttr.Get(), "sub authored value")
        self._VerifyExpectedFields(
            dstAttrSpecBeforeFlatten,
            { "custom": False, 
              "default": "sub authored value",
              "testCustomMetadata": "root authored metadata",
              "typeName" : "string",
              "variability": Sdf.VariabilityVarying })
            

    def test_RemapTargetPaths(self):
        """Tests that relationship target and attribute connection
        paths that point to an object within the source prim are
        remapped when flattened."""
        srcRel = self.srcStage.GetPrimAtPath("/RemapTargetPaths") \
                           .GetRelationship("a")
        
        for dstStage in [self.srcStage, self.stage2]:
            dstPrim = dstStage.DefinePrim("/RemapTargetPathsCopy")
            dstRel = srcRel.FlattenTo(dstPrim)
            self.assertEqual(dstRel.GetPath(), "/RemapTargetPathsCopy.a")

            dstRelSpec = \
                dstStage.GetRootLayer().GetRelationshipAtPath(
                        "/RemapTargetPathsCopy.a")
            self.assertEqual(list(dstRelSpec.targetPathList.explicitItems), 
                             ["/OutsideTarget",
                              "/RemapTargetPathsCopy/A", 
                              "/RemapTargetPathsCopy/A.b", 
                          "/RemapTargetPathsCopy.b"])

    def test_FlattenInstanceProperty(self):
        """Tests flattening properties from instancing prototypes"""
        def ExplicitPathListOp(paths):
            listOp = Sdf.PathListOp()
            listOp.explicitItems = paths
            return listOp

        for dstStage in [self.srcStage, self.stage2]:
            srcAttr = self.srcStage.GetPrimAtPath("/FlattenInstanceProperty") \
                                .GetPrototype().GetChild("Instance") \
                                .GetAttribute("builtinAttr")
            # Flattening a property from a prim in a prototype should behave
            # the same as with any other property for the most part.
            dstPrim = dstStage.DefinePrim("/FlattenInstanceProperty_1",
                                            "FlattenPropertyTest")
            dstAttr = srcAttr.FlattenTo(dstPrim)
            self.assertEqual(dstAttr.GetPath(),
                             "/FlattenInstanceProperty_1.builtinAttr")

            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
            self.assertTrue(dstAttrSpec)
            self._VerifyExpectedFields(
                dstAttrSpec,
                { "connectionPaths": ExplicitPathListOp([
                    "/FlattenInstanceProperty_1.builtinAttr"]),
                  "custom": False,
                  "default": "instance authored value",
                  "typeName": "string",
                  "variability": Sdf.VariabilityVarying })

            # However, if the property has an attribute connection or
            # relationship target that points to an object in a prototype that
            # can't be remapped, a warning is issued and that path is ignored.
            srcAttr = self.srcStage.GetPrimAtPath("/FlattenInstanceProperty") \
                                .GetPrototype().GetChild("Instance2") \
                                .GetAttribute("builtinAttr")
            dstPrim = dstStage.DefinePrim("/FlattenInstanceProperty_2",
                                            "FlattenPropertyTest")
            dstAttr = srcAttr.FlattenTo(dstPrim)
            self.assertEqual(dstAttr.GetPath(),
                             "/FlattenInstanceProperty_2.builtinAttr")

            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
            self.assertTrue(dstAttrSpec)
            self._VerifyExpectedFields(
                dstAttrSpec,
                { "custom": False,
                  "default": "instance authored value 2",
                  "typeName": "string",
                  "variability": Sdf.VariabilityVarying })

            # Using an instance proxy avoids this problem.
            srcAttr = self.srcStage.GetPrimAtPath(
                "/FlattenInstanceProperty/Instance2").GetAttribute("builtinAttr")
            dstPrim = dstStage.DefinePrim("/FlattenInstanceProperty_3",
                                            "FlattenPropertyTest")
            dstAttr = srcAttr.FlattenTo(dstPrim)
            self.assertEqual(dstAttr.GetPath(),
                             "/FlattenInstanceProperty_3.builtinAttr")

            dstAttrSpec = dstStage.GetRootLayer().GetAttributeAtPath(
                    dstAttr.GetPath())
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
