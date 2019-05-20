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

from pxr import Sdf
import textwrap, unittest

class TestSdfCopyUtils(unittest.TestCase):
    def _VerifyExpectedData(self, layer, expected):
        for (path, fieldValues) in expected.items():
            spec = layer.GetObjectAtPath(path)
            self.assertTrue(spec)

            for (field, value) in fieldValues.items():
                self.assertEqual(spec.GetInfo(field), value)
                
            for field in spec.ListInfoKeys():
                self.assertIn(field, fieldValues)

    def test_Basic(self):
        """Tests basic spec copying functionality"""
        srcLayer = Sdf.Layer.CreateAnonymous()
        srcLayerStr = '''\
        #sdf 1.4.32

        def Scope "Root"
        {
            custom string attr = "root_attr"
            custom rel rel

            over "Child"
            {
            }
            variantSet "vset" = {
                "x" (documentation = "testing") {
                    over "VariantChild"
                    {
                    }
                }
            }
        }
        '''
        srcLayer.ImportFromString(textwrap.dedent(srcLayerStr))

        dstLayer = Sdf.Layer.CreateAnonymous()

        # Copy the entire /Root prim spec.
        self.assertTrue(Sdf.CopySpec(srcLayer, "/Root", dstLayer, "/RootCopy"))
        self._VerifyExpectedData(
            dstLayer, expected = {
                "/RootCopy" : {
                    "specifier" : Sdf.SpecifierDef,
                    "typeName" : "Scope"
                },
                "/RootCopy.attr" : {
                    "custom" : True,
                    "typeName" : Sdf.ValueTypeNames.String,
                    "default" : "root_attr",
                    "variability" : Sdf.VariabilityVarying
                },
                "/RootCopy.rel" : {
                    "custom" : True,
                    "variability" : Sdf.VariabilityUniform
                },
                "/RootCopy/Child" : {
                    "specifier" : Sdf.SpecifierOver
                },
                "/RootCopy{vset=}" : { },
                "/RootCopy{vset=x}" : {
                    "specifier" : Sdf.SpecifierOver,
                    "documentation" : "testing"
                },
                "/RootCopy{vset=x}VariantChild" : {
                    "specifier" : Sdf.SpecifierOver
                },
            })

        # Create a parent prim spec to test copying property and variant specs.
        Sdf.CreatePrimInLayer(dstLayer, "/NewRoot")

        # Copy the /Root.attr attribute spec.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root.attr", dstLayer, "/NewRoot.attr2"))
        self._VerifyExpectedData(
            dstLayer, expected = {
                "/NewRoot.attr2" : {
                    "custom" : True,
                    "typeName" : Sdf.ValueTypeNames.String,
                    "default" : "root_attr",
                    "variability" : Sdf.VariabilityVarying
                }
            })

        # Copy the /Root.rel relationship spec.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root.rel", dstLayer, "/NewRoot.rel2"))
        self._VerifyExpectedData(
            dstLayer, expected = {
                "/NewRoot.rel2" : {
                    "custom" : True,
                    "variability" : Sdf.VariabilityUniform
                }
            })

        # Copy the /Root{vset=} variant set spec.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root{vset=}", dstLayer, "/NewRoot{zset=}"))
        self._VerifyExpectedData(
            dstLayer, expected = {
                "/NewRoot{zset=}" : { },
                "/NewRoot{zset=x}" : {
                    "specifier" : Sdf.SpecifierOver,
                    "documentation" : "testing"
                },
                "/NewRoot{zset=x}VariantChild" : {
                    "specifier" : Sdf.SpecifierOver
                },
            })

        # Copy the /Root{vset=x} variant spec.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root{vset=x}", dstLayer, "/NewRoot{zset=y}"))
        self._VerifyExpectedData(
            dstLayer, expected = {
                "/NewRoot{zset=y}" : {
                    "specifier" : Sdf.SpecifierOver,
                    "documentation" : "testing"
                },
                "/NewRoot{zset=y}VariantChild" : {
                    "specifier" : Sdf.SpecifierOver
                },
            })

    def test_Overwrite(self):
        """Tests that copying a spec will overwrite a pre-existing spec
        completely."""
        srcLayer = Sdf.Layer.CreateAnonymous()
        srcLayerStr = '''\
        #sdf 1.4.32

        def "Empty"
        {
        }

        def Scope "Root"
        {
            custom string attr = "root_attr"
            custom rel rel

            over "Child"
            {
            }
            variantSet "vset" = {
                "x" (documentation = "root") {
                    over "RootVariantChild"
                    {
                    }
                }
            }
        }

        def Scope "Copy"
        {
            double attr = 1.0
            rel rel

            over "Child"
            {
            }
            variantSet "vset" = {
                "y" (kind = "model") {
                    over "CopyVariantChild"
                    {
                    }
                }
            }
        }
        '''
        
        srcLayer.ImportFromString(textwrap.dedent(srcLayerStr))

        # Copy the /Root{vset=x} variant spec over /Copy{vset=y}
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root{vset=x}", srcLayer, "/Copy{vset=y}"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Copy{vset=y}" : {
                    "specifier" : Sdf.SpecifierOver,
                    "documentation" : "root"
                },
                "/Copy{vset=y}RootVariantChild" : {
                    "specifier" : Sdf.SpecifierOver
                }
            })
        self.assertFalse(
            srcLayer.GetObjectAtPath("/Copy{vset=y}CopyVariantChild"))

        # Copy the /Root{vset=} variant set spec over /Copy{vset=}
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root{vset=}", srcLayer, "/Copy{vset=}"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Copy{vset=}" : { },
                "/Copy{vset=x}" : { 
                    "specifier" : Sdf.SpecifierOver,
                    "documentation" : "root"
                },
                "/Copy{vset=x}RootVariantChild" : {
                    "specifier" : Sdf.SpecifierOver
                }
            })
        self.assertFalse(srcLayer.GetObjectAtPath("/Copy{vset=y}"))

        # Copy the /Root.attr attribute spec over /Copy.attr
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root.attr", srcLayer, "/Copy.attr"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Copy.attr" : {
                    "custom" : True,
                    "typeName" : Sdf.ValueTypeNames.String,
                    "default" : "root_attr",
                    "variability" : Sdf.VariabilityVarying
                }
            })
        
        # Copy the /Root.rel relationship spec over /Copy.rel
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Root.rel", srcLayer, "/Copy.rel"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Copy.rel" : {
                    "custom" : True,
                    "variability" : Sdf.VariabilityUniform
                }
            })

        # Copy the /Empty relationship spec over /Copy
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Empty", srcLayer, "/Copy"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Copy" : {
                    "specifier" : Sdf.SpecifierDef
                }
            })
        dstSpec = srcLayer.GetPrimAtPath("/Copy")
        self.assertEqual(list(dstSpec.nameChildren), [])
        self.assertEqual(list(dstSpec.properties), [])

    def test_CopyPrimsAndVariant(self):
        """Tests that a prim spec can be copied to a variant and vice-versa."""
        srcLayer = Sdf.Layer.CreateAnonymous()
        srcLayerStr = '''\
        #sdf 1.4.32

        def SourceType "Source"
        {
            double attr = 1.0
            def "Child"
            {
            }

            variantSet "y" = {
                "a" {
                    double attr = 1.0
                    def "Child"
                    {
                    }
                }
            }
        }

        over "OverSource"
        {
            variantSet "x" = {
                "a" {
                    def "Child"
                    {
                    }
                }
            }
        }

        def "Dest"
        {
            variantSet "x" = {
                "a" {
                }
            }
        }

        def "Dest2"
        {
        }
        '''

        srcLayer.ImportFromString(textwrap.dedent(srcLayerStr))

        # Copy the /Source prim over /Dest{x=a}
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Source", srcLayer, "/Dest{x=a}"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Dest{x=a}" : {
                    "specifier" : Sdf.SpecifierOver,
                },
                "/Dest{x=a}.attr" : {
                    "typeName" : Sdf.ValueTypeNames.Double,
                    "default": 1.0,
                    "variability": Sdf.VariabilityVarying,
                    "custom": False
                },
                "/Dest{x=a}Child" : {
                    "specifier" : Sdf.SpecifierDef
                },
                "/Dest{x=a}{y=a}" : {
                    "specifier" : Sdf.SpecifierOver
                },
                "/Dest{x=a}{y=a}.attr" : {
                    "typeName" : Sdf.ValueTypeNames.Double,
                    "default": 1.0,
                    "variability": Sdf.VariabilityVarying,
                    "custom": False
                },
                "/Dest{x=a}{y=a}Child" : {
                    "specifier" : Sdf.SpecifierDef
                }
            })

        # Copy the /Source prim into a new variant /Dest{x=b}
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Source", srcLayer, "/Dest{x=b}"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Dest{x=b}" : {
                    "specifier" : Sdf.SpecifierOver,
                },
                "/Dest{x=b}.attr" : {
                    "typeName" : Sdf.ValueTypeNames.Double,
                    "default": 1.0,
                    "variability": Sdf.VariabilityVarying,
                    "custom": False
                },
                "/Dest{x=b}Child" : {
                    "specifier" : Sdf.SpecifierDef
                },
                "/Dest{x=b}{y=a}" : {
                    "specifier" : Sdf.SpecifierOver
                },
                "/Dest{x=b}{y=a}.attr" : {
                    "typeName" : Sdf.ValueTypeNames.Double,
                    "default": 1.0,
                    "variability": Sdf.VariabilityVarying,
                    "custom": False
                },
                "/Dest{x=b}{y=a}Child" : {
                    "specifier" : Sdf.SpecifierDef
                }
            })

        # Copy the /Source{y=a} variant over /Dest2
        # Note the specifier and typename from the source variant's owning
        # prim are copied over to the destination prim.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Source{y=a}", srcLayer, "/Dest2"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Dest2" : {
                    "specifier" : Sdf.SpecifierDef,
                    "typeName" : "SourceType"
                },
                "/Dest2.attr" : {
                    "typeName" : Sdf.ValueTypeNames.Double,
                    "default": 1.0,
                    "variability": Sdf.VariabilityVarying,
                    "custom": False
                },
                "/Dest2/Child" : {
                    "specifier" : Sdf.SpecifierDef
                }
            })

        # Copy the /Source{y=a} variant into a new prim /Dest3
        # Note the specifier and typename from the source variant's owning
        # prim are copied over to the to the destination prim.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/Source{y=a}", srcLayer, "/Dest3"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Dest3" : {
                    "specifier" : Sdf.SpecifierDef,
                    "typeName" : "SourceType"
                },
                "/Dest3.attr" : {
                    "typeName" : Sdf.ValueTypeNames.Double,
                    "default": 1.0,
                    "variability": Sdf.VariabilityVarying,
                    "custom": False
                },
                "/Dest3/Child" : {
                    "specifier" : Sdf.SpecifierDef
                }
            })

        # Copy the /OverSource variant into a new prim /Dest4. 
        # Note the specifier and typename from the source variant's owning
        # prim are copied over to the to the destination prim.
        self.assertTrue(
            Sdf.CopySpec(srcLayer, "/OverSource{x=a}", srcLayer, "/Dest4"))
        self._VerifyExpectedData(
            srcLayer, expected = {
                "/Dest4" : {
                    "specifier" : Sdf.SpecifierOver,
                },
                "/Dest4/Child" : {
                    "specifier" : Sdf.SpecifierDef
                }
            })

    def test_AttributeConnectionRemapping(self):
        """Tests that attribute connections that point to a child of the 
        source path are remapped to the destination path."""
        layer = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.CreatePrimInLayer(layer, "/Test/Child")
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Float)
        attrSpec.connectionPathList.explicitItems = \
            [ "/Test/Child.attr2", "/Test/Child/Subchild.attr3", 
              "/Test/Sibling.attr" ]
        Sdf.MapperSpec(attrSpec, "/Test/Child.attr2", "mapper")

        # Copy root prim and verify that connections on the child prim that
        # point to objects beneath /Test are remapped to /TestCopy.
        self.assertTrue(Sdf.CopySpec(layer, "/Test", layer, "/TestCopy"))

        dstAttrSpec = layer.GetAttributeAtPath("/TestCopy/Child.attr")
        self.assertTrue(dstAttrSpec)
        
        expectedListOp = Sdf.PathListOp()
        expectedListOp.explicitItems = \
            [ "/TestCopy/Child.attr2", "/TestCopy/Child/Subchild.attr3", 
              "/TestCopy/Sibling.attr" ]
        self.assertEqual(
            dstAttrSpec.GetInfo("connectionPaths"), expectedListOp)

        # Copy prim with attribute and verify that connections that point
        # to objects beneath the source /Test/Child prim are remapped to
        # /Dest.
        self.assertTrue(Sdf.CopySpec(layer, "/Test/Child", layer, "/Dest"))

        dstAttrSpec = layer.GetAttributeAtPath("/Dest.attr")
        self.assertTrue(dstAttrSpec)

        expectedListOp = Sdf.PathListOp()
        expectedListOp.explicitItems = \
            [ "/Dest.attr2", "/Dest/Subchild.attr3", "/Test/Sibling.attr" ]
        self.assertEqual(
            dstAttrSpec.GetInfo("connectionPaths"), expectedListOp)
        self.assertTrue(
            dstAttrSpec.connectionMappers["/Dest.attr2"].typeName, "mapper")

        # Same as above, but copy to variant to ensure that variant selection
        # paths aren't authored into the connection list.
        varSpec = Sdf.CreateVariantInLayer(layer, "/Variant", "version", "a")
        self.assertTrue(
            Sdf.CopySpec(layer, "/Test/Child", layer, "/Variant{version=a}"))

        dstAttrSpec = layer.GetAttributeAtPath("/Variant{version=a}.attr")
        self.assertTrue(dstAttrSpec)

        expectedListOp = Sdf.PathListOp()
        expectedListOp.explicitItems = \
            [ "/Variant.attr2", "/Variant/Subchild.attr3", "/Test/Sibling.attr" ]
        self.assertEqual(
            dstAttrSpec.GetInfo("connectionPaths"), expectedListOp)
        self.assertTrue(
            dstAttrSpec.connectionMappers["/Variant.attr2"].typeName, "mapper")

        # Copy from variant to variant, again to ensure that variant selection
        # paths aren't authored into the connection list.
        varSpec2 = Sdf.CreateVariantInLayer(layer, "/Variant", "version", "b")
        self.assertTrue(
            Sdf.CopySpec(layer, "/Variant{version=a}", 
                         layer, "/Variant{version=b}"))

        dstAttrSpec = layer.GetAttributeAtPath("/Variant{version=b}.attr")
        self.assertTrue(dstAttrSpec)

        self.assertEqual(
            dstAttrSpec.GetInfo("connectionPaths"), expectedListOp)
        self.assertTrue(
            dstAttrSpec.connectionMappers["/Variant.attr2"].typeName, "mapper")

    def test_RelationshipTargetRemapping(self):
        """Tests that relationship targets that point to a child of the 
        source path are remapped to the destination path."""
        layer = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.CreatePrimInLayer(layer, "/Test/Child")
        relSpec = Sdf.RelationshipSpec(primSpec, "rel")
        relSpec.targetPathList.explicitItems = \
            [ "/Test/Child.attr2", "/Test/Child/Subchild.attr3", 
              "/Test/Sibling.attr" ]

        # Copy root prim and verify that targets on the child prim that
        # point to objects beneath /Test are remapped to /TestCopy.
        self.assertTrue(Sdf.CopySpec(layer, "/Test", layer, "/TestCopy"))

        dstRelSpec = layer.GetRelationshipAtPath("/TestCopy/Child.rel")
        self.assertTrue(dstRelSpec)
        
        expectedListOp = Sdf.PathListOp()
        expectedListOp.explicitItems = \
            [ "/TestCopy/Child.attr2", "/TestCopy/Child/Subchild.attr3", 
              "/TestCopy/Sibling.attr" ]
        self.assertEqual(
            dstRelSpec.GetInfo("targetPaths"), expectedListOp)

        # Copy prim with relationship and verify that targets that point
        # to objects beneath the source /Test/Child prim are remapped to
        # /Dest.
        self.assertTrue(Sdf.CopySpec(layer, "/Test/Child", layer, "/Dest"))

        dstRelSpec = layer.GetRelationshipAtPath("/Dest.rel")
        self.assertTrue(dstRelSpec)

        expectedListOp = Sdf.PathListOp()
        expectedListOp.explicitItems = \
            [ "/Dest.attr2", "/Dest/Subchild.attr3", "/Test/Sibling.attr" ]
        self.assertEqual(
            dstRelSpec.GetInfo("targetPaths"), expectedListOp)

        # Same as above, but copy to variant to ensure that variant selection
        # paths aren't authored into the connection list.
        varSpec = Sdf.CreateVariantInLayer(layer, "/Variant", "version", "a")
        self.assertTrue(
            Sdf.CopySpec(layer, "/Test/Child", layer, "/Variant{version=a}"))

        dstRelSpec = layer.GetRelationshipAtPath("/Variant{version=a}.rel")
        self.assertTrue(dstRelSpec)

        expectedListOp = Sdf.PathListOp()
        expectedListOp.explicitItems = \
            [ "/Variant.attr2", "/Variant/Subchild.attr3", "/Test/Sibling.attr" ]
        self.assertEqual(
            dstRelSpec.GetInfo("targetPaths"), expectedListOp)

        # Copy from variant to variant, again to ensure that variant selection
        # paths aren't authored into the connection list.
        varSpec2 = Sdf.CreateVariantInLayer(layer, "/Variant", "version", "b")
        self.assertTrue(
            Sdf.CopySpec(layer, "/Variant{version=a}", 
                         layer, "/Variant{version=b}"))

        dstRelSpec = layer.GetRelationshipAtPath("/Variant{version=b}.rel")
        self.assertTrue(dstRelSpec)

        self.assertEqual(
            dstRelSpec.GetInfo("targetPaths"), expectedListOp)

    def test_InheritsAndSpecializesRemapping(self):
        def _TestRemapping(fieldName):
            layer = Sdf.Layer.CreateAnonymous()
            srcPrimSpec = Sdf.CreatePrimInLayer(layer, "/Root/Instance")
        
            listOp = Sdf.PathListOp()
            listOp.explicitItems = ["/GlobalClass", "/Root/LocalClass"]
            srcPrimSpec.SetInfo(fieldName, listOp)

            # Copy the root prim spec. The paths in the listOp on the child 
            # prim that are beneath the root will be remapped to the new
            # destination.
            self.assertTrue(Sdf.CopySpec(layer, "/Root", layer, "/RootCopy"))

            expectedListOp = Sdf.PathListOp()
            expectedListOp.explicitItems = \
                ["/GlobalClass", "/RootCopy/LocalClass"]
            self.assertEqual(
                layer.GetPrimAtPath("/RootCopy/Instance").GetInfo(fieldName), 
                expectedListOp)

            # Copy the child prim spec on which the inherit or specializes
            # list is authored. Since the paths in the listOp are outside the
            # root of the copy operation, none of them will be remapped.
            self.assertTrue(
                Sdf.CopySpec(layer, "/Root/Instance", layer, "/InstanceCopy"))

            expectedListOp = listOp
            self.assertEqual(
                layer.GetPrimAtPath("/InstanceCopy").GetInfo(fieldName), 
                expectedListOp)

        _TestRemapping("inheritPaths")
        _TestRemapping("specializes")

    def test_ReferenceRemapping(self):
        layer = Sdf.Layer.CreateAnonymous()
        srcPrimSpec = Sdf.CreatePrimInLayer(layer, "/Root/Child")
        srcPrimSpec.referenceList.explicitItems = [
            # External reference
            Sdf.Reference("./test.sdf", "/Ref"), 
            # External sub-root reference
            Sdf.Reference("./test.sdf", "/Root/Ref"), 
            # Internal reference
            Sdf.Reference("", "/Ref"),
            # Internal sub-root reference
            Sdf.Reference("", "/Root/Ref")
        ]
        
        # Copy the root prim spec and verify that the internal sub-root 
        # references on the child prim that target a prim beneath /Root are 
        # remapped to /RootCopy.
        self.assertTrue(Sdf.CopySpec(layer, "/Root", layer, "/RootCopy"))

        expectedListOp = Sdf.ReferenceListOp()
        expectedListOp.explicitItems = [
            Sdf.Reference("./test.sdf", "/Ref"), 
            Sdf.Reference("./test.sdf", "/Root/Ref"), 
            Sdf.Reference("", "/Ref"),
            Sdf.Reference("", "/RootCopy/Ref")
        ]
        
        self.assertEqual(
            layer.GetPrimAtPath("/RootCopy/Child").GetInfo("references"), 
            expectedListOp)

        # Copy the child prim spec. Since all of the internal sub-root
        # references point outside the root of the copy operation, none
        # of them will be remapped.
        self.assertTrue(Sdf.CopySpec(layer, "/Root/Child", layer, "/ChildCopy"))

        expectedListOp = Sdf.ReferenceListOp()
        expectedListOp.explicitItems = [
            Sdf.Reference("./test.sdf", "/Ref"), 
            Sdf.Reference("./test.sdf", "/Root/Ref"), 
            Sdf.Reference("", "/Ref"),
            Sdf.Reference("", "/Root/Ref")
        ]
        
        self.assertEqual(
            layer.GetPrimAtPath("/ChildCopy").GetInfo("references"), 
            expectedListOp)

    def test_PayloadRemapping(self):
        layer = Sdf.Layer.CreateAnonymous()
        srcPrimSpec = Sdf.CreatePrimInLayer(layer, "/Root/Child")
        srcPrimSpec.payloadList.explicitItems = [
            # External payload
            Sdf.Payload("./test.sdf", "/Ref"), 
            # External sub-root payload
            Sdf.Payload("./test.sdf", "/Root/Ref"), 
            # Internal payload
            Sdf.Payload("", "/Ref"),
            # Internal sub-root payload
            Sdf.Payload("", "/Root/Ref")
        ]

        # Copy the root prim spec and verify that the internal sub-root 
        # payloads on the child prim that target a prim beneath /Root are 
        # remapped to /RootCopy.
        self.assertTrue(Sdf.CopySpec(layer, "/Root", layer, "/RootCopy"))

        expectedListOp = Sdf.PayloadListOp()
        expectedListOp.explicitItems = [
            Sdf.Payload("./test.sdf", "/Ref"), 
            Sdf.Payload("./test.sdf", "/Root/Ref"), 
            Sdf.Payload("", "/Ref"),
            Sdf.Payload("", "/RootCopy/Ref")
        ]

        self.assertEqual(
            layer.GetPrimAtPath("/RootCopy/Child").GetInfo("payload"), 
            expectedListOp)

        # Copy the child prim spec. Since all of the internal sub-root
        # payloads point outside the root of the copy operation, none
        # of them will be remapped.
        self.assertTrue(Sdf.CopySpec(layer, "/Root/Child", layer, "/ChildCopy"))

        expectedListOp = Sdf.PayloadListOp()
        expectedListOp.explicitItems = [
            Sdf.Payload("./test.sdf", "/Ref"), 
            Sdf.Payload("./test.sdf", "/Root/Ref"), 
            Sdf.Payload("", "/Ref"),
            Sdf.Payload("", "/Root/Ref")
        ]

        self.assertEqual(
            layer.GetPrimAtPath("/ChildCopy").GetInfo("payload"), 
            expectedListOp)

    def test_Relocates(self):
        """Tests that relocates are remapped to destination prim on copy"""
        layer = Sdf.Layer.CreateAnonymous()
        srcPrimSpec = Sdf.PrimSpec(layer, "Root", Sdf.SpecifierOver)
        srcPrimSpec.relocates = { Sdf.Path("/Root/A") : Sdf.Path("/Root/B") }

        self.assertTrue(Sdf.CopySpec(layer, "/Root", layer, "/Copy"))
        self._VerifyExpectedData(
            layer, expected = {
                "/Copy" : {
                    "relocates" : { Sdf.Path("/Copy/A") : Sdf.Path("/Copy/B") },
                    "specifier" : Sdf.SpecifierOver
                }
            })

    def test_Advanced(self):
        """Test using callbacks to control spec copying via 
        advanced API"""
        l = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.PrimSpec(l, "Root", Sdf.SpecifierOver)
        primSpec.documentation = "prim docs"
        primSpec.kind = "model"

        # If callbacks just return True, all values and children are copied.
        self.assertTrue(
            Sdf.CopySpec(l, "/Root", l, "/Copy",
                         shouldCopyValueFn = lambda *args: True,
                         shouldCopyChildrenFn = lambda *args: True))
        dstPrimSpec = l.GetPrimAtPath("/Copy")
        self._VerifyExpectedData(
            l, expected = {
                "/Copy" : {
                    "specifier" : Sdf.SpecifierOver,
                    "documentation" : "prim docs",
                    "kind" : "model"
                }
            })

        # Set up a value callback that skips copying a field and replaces
        # the value of another field during the copy.
        def shouldCopyValue(*args):
            if args[1] == "documentation":
                return False
            elif args[1] == "kind":
                return (True, "prop")
            return True

        self.assertTrue(
            Sdf.CopySpec(l, "/Root", l, "/Copy2",
                         shouldCopyValueFn = shouldCopyValue,
                         shouldCopyChildrenFn = lambda *args: True))
        dstPrimSpec = l.GetPrimAtPath("/Copy2")
        self._VerifyExpectedData(
            l, expected = {
                "/Copy2" : {
                    "specifier" : Sdf.SpecifierOver,
                    "kind" : "prop"
                }
            })

        # Set up a children callback that skips copying properties.
        attrSpecA = Sdf.AttributeSpec(primSpec, "A", Sdf.ValueTypeNames.Float)
        attrSpecA.custom = True
        attrSpecA.default = 1.0

        attrSpecB = Sdf.AttributeSpec(primSpec, "B", Sdf.ValueTypeNames.Float)
        attrSpecB.custom = True
        attrSpecB.default = 2.0

        def shouldCopyChildren(*args):
            if args[0] == "properties":
                return False
            return True

        self.assertTrue(
            Sdf.CopySpec(l, "/Root", l, "/Copy3",
                         shouldCopyValueFn = lambda *args: True,
                         shouldCopyChildrenFn = shouldCopyChildren))

        dstPrimSpec = l.GetPrimAtPath("/Copy3")
        self.assertEqual(list(dstPrimSpec.properties), [])
        
        # Set up a children callback that copies the property named "A"
        # to a property named "C" under the destination spec.
        def shouldCopyChildren(*args):
            if args[0] == "properties":
                return (True, ["A"], ["C"])
            return True

        self.assertTrue(
            Sdf.CopySpec(l, "/Root", l, "/Copy4",
                         shouldCopyValueFn = lambda *args: True,
                         shouldCopyChildrenFn = shouldCopyChildren))
        dstPrimSpec = l.GetPrimAtPath("/Copy4")
        self.assertEqual(list(dstPrimSpec.properties), 
                         [l.GetAttributeAtPath("/Copy4.C")])
        self._VerifyExpectedData(
            l, expected = {
                "/Copy4.C" : {
                    "custom" : True,
                    "default" : 1.0,
                    "typeName" : Sdf.ValueTypeNames.Float,
                    "variability" : Sdf.VariabilityVarying
                }
            })

if __name__ == "__main__":
    unittest.main()
