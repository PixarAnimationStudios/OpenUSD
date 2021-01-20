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

import sys
import unittest

from pxr import Tf
from pxr import Usd


class TestUsdGeomTypeRegistry(unittest.TestCase):
    """Assert that the type registry is properly popluated by plugInfo.json
    even when UsdGeom is not loaded.

    In practice, this serves as the regression test for the static type API
    on UsdSchemaRegistry.
    """

    def _assertType(self, typeName, aliases, isAPI):
        # We aggressively check this to make sure no one has
        # accidently imported UsdGeom accidently.
        self.assertNotIn("pxr.UsdGeom", sys.modules)
        # The actual C++ typename is not registered as a valid type name in 
        # in schema registry so verify that you can't query the type from the
        # schema registry using the C++ type name.
        self.assertEqual(Usd.SchemaRegistry.GetTypeFromSchemaTypeName(typeName), 
                         Tf.Type.Unknown)
        for alias in aliases:
            # The type is queryable by alias from the schema registry.
            tfType = Usd.SchemaRegistry.GetTypeFromSchemaTypeName(alias)
            self.assertNotEqual(tfType, Tf.Type.Unknown)
            self.assertEqual(tfType.typeName, typeName)
            # Verify the inverse: the schema type name is queryable from the 
            # TfType and matches the alias
            self.assertEqual(
                Usd.SchemaRegistry.GetSchemaTypeName(tfType), alias)

            if isAPI:
                # For an API schema type verify that the API schema based schema
                # registy functions work and the concrete schema based functions
                # do not.
                self.assertEqual(
                    Usd.SchemaRegistry.GetAPITypeFromSchemaTypeName(alias),
                    tfType)
                self.assertEqual(
                    Usd.SchemaRegistry.GetAPISchemaTypeName(tfType), alias)
                self.assertEqual(
                    Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName(alias),
                    Tf.Type.Unknown)
                self.assertEqual(
                    Usd.SchemaRegistry.GetConcreteSchemaTypeName(tfType), "")
            else:
                # For a concrete schema type verify that the concrete schema 
                # based schema registy functions work and the API schema based 
                # functions do not.
                self.assertEqual(
                    Usd.SchemaRegistry.GetConcreteTypeFromSchemaTypeName(alias),
                    tfType)
                self.assertEqual(
                    Usd.SchemaRegistry.GetConcreteSchemaTypeName(tfType), alias)
                self.assertEqual(
                    Usd.SchemaRegistry.GetAPITypeFromSchemaTypeName(alias),
                    Tf.Type.Unknown)
                self.assertEqual(
                    Usd.SchemaRegistry.GetAPISchemaTypeName(tfType), "")

    def test_ConcreteTyped(self):
        """Test for concrete typed schemas"""
        self._assertType("UsdGeomMesh", ["Mesh"], False)
        self._assertType("UsdGeomSphere", ["Sphere"], False)

    def test_AbstractTyped(self):
        """Test for abstract typed schemas"""
        # NOTE.  Abstract typed schemas don't register type aliases.
        self._assertType("UsdGeomBoundable", [], False)
        self._assertType("UsdGeomImageable", [], False)

    def test_Applied(self):
        """Test for applied API schemas"""
        self._assertType("UsdGeomPrimvarsAPI", ["PrimvarsAPI"], True)
        self._assertType("UsdGeomMotionAPI", ["MotionAPI"], True)


if __name__ == "__main__":
    unittest.main()
