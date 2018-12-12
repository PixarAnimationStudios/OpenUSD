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

    In practice, this serves as the regression test for
    Usd.SchemaRegistry.FindDerivedByType.
    """

    def _assertType(self, typeName, aliases):
        # We aggressively check this to make sure no one has
        # accidently imported UsdGeom accidently.
        self.assertNotIn("pxr.UsdGeom", sys.modules)
        tfType = Usd.SchemaRegistry.GetTypeFromName(typeName)
        self.assertNotEqual(tfType, Tf.Type.Unknown)
        for alias in aliases:
            aliasTfType = Usd.SchemaRegistry.GetTypeFromName(alias)
            self.assertEqual(tfType, aliasTfType)

    def test_ConcreteTyped(self):
        """Test for concrete typed schemas"""
        self._assertType("UsdGeomMesh", ["Mesh"])
        self._assertType("UsdGeomSphere", ["Sphere"])

    def test_AbstractTyped(self):
        """Test for abstract typed schemas"""
        # NOTE.  Abstract typed schemas don't register type aliases.
        self._assertType("UsdGeomBoundable", [])
        self._assertType("UsdGeomImageable", [])

    def test_Applied(self):
        """Test for applied API schemas"""
        self._assertType("UsdGeomPrimvarsAPI", ["PrimvarsAPI"])
        self._assertType("UsdGeomMotionAPI", ["MotionAPI"])


if __name__ == "__main__":
    unittest.main()
