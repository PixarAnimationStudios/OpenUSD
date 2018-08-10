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

from pxr import Ndr
import unittest

class TestVersion(unittest.TestCase):
    def reprTests(self, v):
        self.assertEqual(eval(repr(v)), v)
        self.assertEqual(eval(repr(v)).IsDefault(), v.IsDefault())

    def relationalTestsEqual(self, lhs, rhs):
        self.assertEqual(lhs, rhs)
        self.assertEqual(rhs, lhs)
        self.assertFalse(lhs != rhs)
        self.assertFalse(rhs != lhs)
        self.assertFalse(lhs < rhs)
        self.assertTrue(lhs <= rhs)
        self.assertFalse(lhs > rhs)
        self.assertTrue(lhs >= rhs)

    def relationalTestsLess(self, lhs, rhs):
        self.assertNotEqual(lhs, rhs)
        self.assertNotEqual(rhs, lhs)
        self.assertTrue(lhs != rhs)
        self.assertTrue(rhs != lhs)
        self.assertTrue(lhs < rhs)
        self.assertTrue(lhs <= rhs)
        self.assertFalse(lhs > rhs)
        self.assertFalse(lhs >= rhs)

    def test_Version(self):
        """
        Test NdrVersion.
        """

        # Invalid version.
        v = Ndr.Version()
        self.assertFalse(v)
        self.assertFalse(v.IsDefault())
        self.relationalTestsEqual(v, Ndr.Version())
        self.reprTests(v)
        self.assertEqual(str(v), "<invalid version>")

        # Invalid default version.
        u = v.GetAsDefault()
        self.assertFalse(u)
        self.assertTrue(u.IsDefault())
        self.relationalTestsEqual(u, Ndr.Version())
        self.reprTests(u)
        self.assertEqual(str(u), "<invalid version>")

        # Valid versions.
        v1 = Ndr.Version(1)
        v1_0 = Ndr.Version(1, 0).GetAsDefault()
        v1_1 = Ndr.Version(1, 1)
        v2_0 = Ndr.Version(2, 0)
        self.assertTrue(v1)
        self.assertTrue(v1_0)
        self.assertTrue(v1_1)
        self.assertTrue(v2_0)
        self.assertFalse(v1.IsDefault())
        self.assertTrue(v1_0.IsDefault())
        self.assertFalse(v1_1.IsDefault())
        self.assertFalse(v2_0.IsDefault())
        self.assertEqual(str(v1), "1")
        self.assertEqual(str(v1_0), "1")
        self.assertEqual(str(v1_1), "1.1")
        self.assertEqual(str(v2_0), "2")
        self.assertEqual(v1.GetStringSuffix(), "_1")
        self.assertEqual(v1_0.GetStringSuffix(), "")
        self.assertEqual(v1_1.GetStringSuffix(), "_1.1")
        self.assertEqual(v2_0.GetStringSuffix(), "_2")
        self.relationalTestsEqual(v1, v1)
        self.relationalTestsEqual(v1_0, v1_0)
        self.relationalTestsEqual(v1, v1_0)
        self.relationalTestsEqual(v1_1, v1_1)
        self.relationalTestsEqual(v2_0, v2_0)
        self.relationalTestsLess(v1, v1_1)
        self.relationalTestsLess(v1_0, v1_1)
        self.relationalTestsLess(v1_0, v2_0)
        self.relationalTestsLess(v1_1, v2_0)
        self.reprTests(v1)
        self.reprTests(v1_0)
        self.reprTests(v1_1)
        self.reprTests(v2_0)

if __name__ == '__main__':
    unittest.main()
