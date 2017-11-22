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
import unittest

class TestSdfCopyUtils(unittest.TestCase):
    def _VerifyExpectedData(self, layer, expected):
        for (path, fieldValues) in expected.items():
            spec = layer.GetObjectAtPath(path)
            self.assertTrue(spec)

            for (field, value) in fieldValues.items():
                self.assertEqual(spec.GetInfo(field), value)
                
            for field in spec.ListInfoKeys():
                self.assertIn(field, fieldValues)

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
