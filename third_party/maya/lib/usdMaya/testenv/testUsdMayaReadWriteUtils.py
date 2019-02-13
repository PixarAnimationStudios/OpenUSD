#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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
#

from pxr import UsdMaya

from pxr import Gf
from pxr import Sdf
from pxr import Tf
from pxr import Vt

from maya import cmds
from maya import standalone

import unittest


class testUsdMayaReadWriteUtils(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testGetVtValue(self):
        cmds.file(new=True, force=True)

        cmds.group(name="group1", empty=True)

        mat = UsdMaya.WriteUtil.GetVtValue("group1.matrix",
                Sdf.ValueTypeNames.Matrix4d)
        self.assertEqual(mat, Gf.Matrix4d(1.0))

        vec3f = UsdMaya.WriteUtil.GetVtValue("group1.scale",
                Sdf.ValueTypeNames.Vector3f)
        self.assertEqual(vec3f, Gf.Vec3f(1.0))
        self.assertIsInstance(vec3f, Gf.Vec3f)

        vec3d = UsdMaya.WriteUtil.GetVtValue("group1.scale",
                Sdf.ValueTypeNames.Vector3d)
        self.assertEqual(vec3d, Gf.Vec3d(1.0))
        self.assertIsInstance(vec3d, Gf.Vec3d)

        cmds.addAttr("group1", ln="myArray", dt="vectorArray")
        cmds.setAttr("group1.myArray", 3, (0, 1, 2), (1, 2, 3), (2, 3, 4),
                type="vectorArray")
        float3Array = UsdMaya.WriteUtil.GetVtValue("group1.myArray",
                Sdf.ValueTypeNames.Float3Array)
        self.assertEqual(float3Array,
                Vt.Vec3fArray([(0, 1, 2), (1, 2, 3), (2, 3, 4)]))
        point3Array = UsdMaya.WriteUtil.GetVtValue("group1.myArray",
                Sdf.ValueTypeNames.Point3dArray)
        self.assertEqual(point3Array,
                Vt.Vec3dArray([(0, 1, 2), (1, 2, 3), (2, 3, 4)]))

        # Strings can be strings, tokens, or asset paths.
        cmds.addAttr("group1", ln="myString", dt="string")
        cmds.setAttr("group1.myString", "a_b_c", type="string")
        string = UsdMaya.WriteUtil.GetVtValue("group1.myString",
                Sdf.ValueTypeNames.String)
        self.assertEqual(string, "a_b_c")
        token = UsdMaya.WriteUtil.GetVtValue("group1.myString",
                Sdf.ValueTypeNames.Token) # alas, this becomes a str in Python
        self.assertEqual(token, "a_b_c")
        token = UsdMaya.WriteUtil.GetVtValue("group1.myString",
                Sdf.ValueTypeNames.Asset)
        self.assertEqual(token, Sdf.AssetPath("a_b_c"))

        # Colors have display-to-linear conversion applied.
        cmds.addAttr("group1", ln="myColor", at="double3")
        cmds.addAttr("group1", ln="myColorX", p="myColor", at="double")
        cmds.addAttr("group1", ln="myColorY", p="myColor", at="double")
        cmds.addAttr("group1", ln="myColorZ", p="myColor", at="double")
        cmds.setAttr("group1.myColor", 0.5, 0.5, 0.5)
        color3f = UsdMaya.WriteUtil.GetVtValue("group1.myColor",
                Sdf.ValueTypeNames.Color3f)
        self.assertAlmostEqual(color3f[0], 0.2176376)
        vec3f = UsdMaya.WriteUtil.GetVtValue("group1.myColor",
                Sdf.ValueTypeNames.Vector3f)
        self.assertAlmostEqual(vec3f[0], 0.5)

    def testFindOrCreateMayaAttr(self):
        cmds.file(new=True, force=True)

        cmds.group(name="group1", empty=True)
        attrName = UsdMaya.ReadUtil.FindOrCreateMayaAttr(
                Sdf.ValueTypeNames.Asset,
                Sdf.VariabilityUniform,
                "group1",
                "myAssetPath",
                "kittens")
        self.assertEqual(attrName, "group1.myAssetPath")
        self.assertEqual(
                cmds.attributeQuery("myAssetPath", n="group1", nn=True),
                "kittens")
        self.assertFalse(
                cmds.attributeQuery("myAssetPath", n="group1", uaf=True))
        cmds.setAttr(attrName, "cat.png", type="string")
        self.assertEqual(cmds.getAttr(attrName, type=True), "string")

        attrName = UsdMaya.ReadUtil.FindOrCreateMayaAttr(
                Sdf.ValueTypeNames.Double3Array,
                Sdf.VariabilityVarying,
                "group1",
                "myDouble3Array",
                "puppies")
        self.assertEqual(attrName, "group1.myDouble3Array")
        self.assertEqual(
                cmds.attributeQuery("myDouble3Array", n="group1", nn=True),
                "puppies")
        self.assertTrue(
                cmds.attributeQuery("myDouble3Array", n="group1", k=True))

        # Try re-creating the attribute with a different nice name. We should
        # get the same attribute back (no changes in-scene).
        attrName = UsdMaya.ReadUtil.FindOrCreateMayaAttr(
                Sdf.ValueTypeNames.Double3Array,
                Sdf.VariabilityVarying,
                "group1",
                "myDouble3Array",
                "dolphins")
        self.assertEqual(attrName, "group1.myDouble3Array")
        self.assertEqual(
                cmds.attributeQuery("myDouble3Array", n="group1", nn=True),
                "puppies")

        # Try re-creating the attribute with an incompatible type. We should
        # get an exception (Tf runtime error).
        with self.assertRaises(Tf.ErrorException):
            attrName = UsdMaya.ReadUtil.FindOrCreateMayaAttr(
                    Sdf.ValueTypeNames.String,
                    Sdf.VariabilityVarying,
                    "group1",
                    "myDouble3Array",
                    "dolphins")

    def testSetMayaAttr(self):
        cmds.file(new=True, force=True)

        cmds.group(name="group1", empty=True)
        attrName = UsdMaya.ReadUtil.FindOrCreateMayaAttr(
                Sdf.ValueTypeNames.Asset,
                Sdf.VariabilityUniform,
                "group1",
                "myAssetPath",
                "kittens")
        self.assertTrue(UsdMaya.ReadUtil.SetMayaAttr(attrName,
                Sdf.AssetPath("kittens.png")))
        self.assertEqual(cmds.getAttr("group1.myAssetPath"), "kittens.png")
        self.assertEqual(
                UsdMaya.WriteUtil.GetVtValue(attrName,
                    Sdf.ValueTypeNames.Asset),
                Sdf.AssetPath("kittens.png"))

        # Test color conversion from linear to display.
        attrName = UsdMaya.ReadUtil.FindOrCreateMayaAttr(
                Sdf.ValueTypeNames.Color3f,
                Sdf.VariabilityUniform,
                "group1",
                "furColor")
        self.assertTrue(UsdMaya.ReadUtil.SetMayaAttr(attrName,
                Gf.Vec3f(0.2176376)))
        self.assertAlmostEqual(
                UsdMaya.WriteUtil.GetVtValue(attrName,
                    Sdf.ValueTypeNames.Color3f)[0],
                0.2176376)
        self.assertAlmostEqual(
                UsdMaya.WriteUtil.GetVtValue(attrName,
                    Sdf.ValueTypeNames.Vector3f)[0],
                0.5)

        # Should even work on Maya built-in attrs.
        self.assertTrue(UsdMaya.ReadUtil.SetMayaAttr("group1.scale",
                Gf.Vec3d(1.0, 2.0, 3.0)))
        self.assertEqual(cmds.getAttr("group1.scale"), [(1.0, 2.0, 3.0)])

    def testSetMayaAttrKeyableState(self):
        cmds.file(new=True, force=True)

        cmds.group(name="group1", empty=True)
        UsdMaya.ReadUtil.SetMayaAttrKeyableState("group1.scaleX",
                Sdf.VariabilityUniform)
        self.assertFalse(cmds.getAttr("group1.scaleX", k=True))
        UsdMaya.ReadUtil.SetMayaAttrKeyableState("group1.scaleX",
                Sdf.VariabilityVarying)
        self.assertTrue(cmds.getAttr("group1.scaleX", k=True))


if __name__ == '__main__':
    unittest.main(verbosity=2)
