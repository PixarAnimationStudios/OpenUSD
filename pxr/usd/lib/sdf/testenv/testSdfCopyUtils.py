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

    def test_AttributeConnectionRemapping(self):
        """Tests that attribute connections that point to a child of the 
        source path are remapped to the destination path."""
        layer = Sdf.Layer.CreateAnonymous()
        primSpec = Sdf.CreatePrimInLayer(layer, "/Test/Child")
        attrSpec = Sdf.AttributeSpec(primSpec, "attr", Sdf.ValueTypeNames.Float)
        attrSpec.connectionPathList.explicitItems = \
            [ "/Test/Child.attr2", "/Test/Child/Subchild.attr3", 
              "/Test/Sibling.attr" ]
        attrSpec.SetConnectionMarker("/Test/Child.attr2", "final")
        Sdf.MapperSpec(attrSpec, "/Test/Child.attr2", "mapper")

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
        self.assertEqual(
            dstAttrSpec.connectionMarkers["/Dest.attr2"], "final")
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
        self.assertEqual(
            dstAttrSpec.connectionMarkers["/Variant.attr2"], "final")
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
        self.assertEqual(
            dstAttrSpec.connectionMarkers["/Variant.attr2"], "final")
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
        relSpec.SetTargetMarker("/Test/Child.attr2", "final")

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
        self.assertEqual(dstRelSpec.targetMarkers["/Dest.attr2"], "final")

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
        self.assertEqual(dstRelSpec.targetMarkers["/Variant.attr2"], "final")

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
        self.assertEqual(dstRelSpec.targetMarkers["/Variant.attr2"], "final")

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
