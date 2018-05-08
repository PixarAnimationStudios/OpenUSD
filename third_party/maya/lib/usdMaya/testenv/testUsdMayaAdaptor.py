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

from pxr import Sdf, Tf, UsdGeom

# XXX: The try/except here is temporary until we change the Pixar-internal
# package name to match the external package name.
try:
    from pxr import UsdMaya
except ImportError:
    from pixar import UsdMaya

from maya import cmds
from maya import standalone

import unittest

class testUsdMayaAdaptor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.loadPlugin('pxrUsd')

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    def testApplySchema(self):
        """Tests schema application."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = UsdMaya.Adaptor('group1')
        self.assertTrue(proxy)
        self.assertEqual(proxy.GetAppliedSchemas(), [])

        modelAPI = proxy.ApplySchema(UsdGeom.ModelAPI)
        self.assertTrue(modelAPI)
        self.assertEqual(proxy.GetAppliedSchemas(), ["GeomModelAPI"])
        self.assertEqual(modelAPI.GetAuthoredAttributeNames(), [])

        motionAPI = proxy.ApplySchema(UsdGeom.MotionAPI)
        self.assertTrue(motionAPI)
        self.assertEqual(proxy.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])
        self.assertEqual(motionAPI.GetAuthoredAttributeNames(), [])

        # No support currently for typed schemas.
        with self.assertRaises(Tf.ErrorException):
            proxy.ApplySchema(UsdGeom.Xformable)

        with self.assertRaises(Tf.ErrorException):
            proxy.ApplySchema(UsdGeom.Mesh)

        self.assertEqual(proxy.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])

    def testGetSchema(self):
        """Tests getting schemas from the proxy."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = UsdMaya.Adaptor('group1')
        modelAPI = proxy.GetSchema(UsdGeom.ModelAPI)
        self.assertTrue(modelAPI)

        # No support currently for typed schemas.
        with self.assertRaises(Tf.ErrorException):
            proxy.GetSchema(UsdGeom.Xformable)

        with self.assertRaises(Tf.ErrorException):
            proxy.GetSchema(UsdGeom.Mesh)

    def testUnapplySchema(self):
        """Tests unapplying schemas and effect on existing proxy objects."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = UsdMaya.Adaptor('group1')
        self.assertEqual(proxy.GetAppliedSchemas(), [])

        proxy.ApplySchema(UsdGeom.ModelAPI)
        proxy.ApplySchema(UsdGeom.MotionAPI)
        self.assertEqual(proxy.GetAppliedSchemas(),
                ["GeomModelAPI", "MotionAPI"])

        proxy.UnapplySchema(UsdGeom.ModelAPI)
        self.assertEqual(proxy.GetAppliedSchemas(), ["MotionAPI"])

        # Note that schema objects still remain alive after being
        # unapplied.
        modelAPI = proxy.GetSchema(UsdGeom.ModelAPI)
        self.assertTrue(modelAPI)

        # But they expire once the underlying Maya node is removed.
        cmds.delete("group1")
        self.assertFalse(modelAPI)
        self.assertFalse(proxy)

    def testAttributes(self):
        """Tests creating and removing schema attributes."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = UsdMaya.Adaptor('group1')
        modelAPI = proxy.ApplySchema(UsdGeom.ModelAPI)

        # Schema attributes list versus authored attributes list.
        self.assertIn(UsdGeom.Tokens.modelCardTextureXPos,
                modelAPI.GetAttributeNames())
        self.assertNotIn(UsdGeom.Tokens.modelCardTextureXPos,
                modelAPI.GetAuthoredAttributeNames())

        # Unauthored attribute.
        self.assertFalse(
                modelAPI.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos))

        # Non-existent attribute.
        with self.assertRaises(Tf.ErrorException):
            modelAPI.GetAttribute("fakeAttr")

        # Create and set an attribute.
        attr = modelAPI.CreateAttribute(UsdGeom.Tokens.modelCardTextureXPos)
        self.assertTrue(attr)
        self.assertTrue(attr.Set(Sdf.AssetPath("example.png")))

        attr = modelAPI.GetAttribute(UsdGeom.Tokens.modelCardTextureXPos)
        self.assertTrue(attr)
        self.assertEqual(attr.Get(), Sdf.AssetPath("example.png"))

        self.assertEqual(attr.GetAttributeDefinition().name,
                "model:cardTextureXPos")
        self.assertEqual(modelAPI.GetAuthoredAttributeNames(),
                [UsdGeom.Tokens.modelCardTextureXPos])

        # Existing attrs become invalid when the plug is removed.
        modelAPI.RemoveAttribute(UsdGeom.Tokens.modelCardTextureXPos)
        self.assertFalse(attr)

        # Should error on invalid attr access.
        with self.assertRaises(Tf.ErrorException):
            attr.Get()

    def testMetadata(self):
        """Tests setting and clearing metadata."""
        cmds.file(new=True, force=True)
        cmds.group(name="group1", empty=True)

        proxy = UsdMaya.Adaptor('group1')
        self.assertEqual(proxy.GetAllAuthoredMetadata(), {})
        self.assertIsNone(proxy.GetMetadata("instanceable"))

        proxy.SetMetadata("instanceable", True)
        self.assertEqual(proxy.GetMetadata("instanceable"), True)

        proxy.SetMetadata("kind", "awesomeKind")
        self.assertEqual(proxy.GetAllAuthoredMetadata(), {
            "instanceable": True,
            "kind": "awesomeKind"
        })

        # Unregistered key.
        with self.assertRaises(Tf.ErrorException):
            proxy.SetMetadata("fakeMetadata", True)

        with self.assertRaises(Tf.ErrorException):
            proxy.GetMetadata("otherFakeMetadata")

        proxy.ApplySchema(UsdGeom.ModelAPI)
        apiSchemas = proxy.GetMetadata("apiSchemas")
        self.assertEqual(apiSchemas.prependedItems, ["GeomModelAPI"])

        # Clear metadata.
        proxy.ClearMetadata("kind")
        self.assertIsNone(proxy.GetMetadata("kind"))

    def testStaticHelpers(self):
        """Tests the static helpers for querying metadata and schema info."""
        self.assertIn("MotionAPI", UsdMaya.Adaptor.GetRegisteredAPISchemas())
        self.assertIn("hidden", UsdMaya.Adaptor.GetPrimMetadataFields())

if __name__ == '__main__':
    unittest.main(verbosity=2)
