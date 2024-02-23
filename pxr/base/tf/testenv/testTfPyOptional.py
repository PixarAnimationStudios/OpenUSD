#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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
import sys
from pxr import Tf
import unittest

TestBoost = Tf.Tf_TestPyOptionalBoost
TestStd = Tf.Tf_TestPyOptionalStd

maxint = (2 ** 31) - 1

class TestTfPyOptional(unittest.TestCase):

    def test_OptionalArgs(self):
        for testKind in (TestBoost, TestStd):
            self.assertEqual(testKind.TakesOptional(), (None, None))
            self.assertEqual(testKind.TakesOptional(None), (None, None))
            self.assertEqual(testKind.TakesOptional(None, None), (None, None))
            self.assertEqual(testKind.TakesOptional('abc', None), ('abc', None))
            self.assertEqual(testKind.TakesOptional(None, []), (None, []))
            self.assertEqual(testKind.TakesOptional(None, ['a', 'b', 'c']), (None, ['a','b','c']))
            self.assertEqual(testKind.TakesOptional('', []), ('', []))
            self.assertEqual(testKind.TakesOptional('abc', ['a', 'b', 'c']), ('abc', ['a','b','c']))

    def test_OptionalReturns(self):
        for testKind in (TestBoost, TestStd):
            self.assertEqual(testKind.TestOptionalString(None), None)
            self.assertEqual(testKind.TestOptionalStringVector(['string', 'list']), ['string', 'list'])
            self.assertEqual(testKind.TestOptionalString('string'), 'string')
            self.assertEqual(testKind.TestOptionalDouble(7e-7), 7e-7)
            self.assertEqual(testKind.TestOptionalDouble(-7e-7), -7e-7)
            self.assertAlmostEqual(testKind.TestOptionalFloat(7e-7), 7e-7)
            self.assertAlmostEqual(testKind.TestOptionalFloat(7e-7), 7e-7)
            self.assertEqual(testKind.TestOptionalLong(-maxint - 1), -maxint - 1)
            self.assertEqual(testKind.TestOptionalULong(maxint + 1),  maxint + 1)
            self.assertEqual(testKind.TestOptionalInt(-7), -7)
            self.assertEqual(testKind.TestOptionalUInt(7), 7)
            self.assertEqual(testKind.TestOptionalChar('c'), 'c')
            self.assertEqual(testKind.TestOptionalUChar(63), 63)

    def test_OptionalBadArgs(self):
        # Boost throws a Boost.Python.ArgumentError
        for testKind in (TestBoost, TestStd):
            with self.assertRaises(Exception):
                testKind.TestOptionalString(43)
            with self.assertRaises(Exception):
                testKind.TestOptionalInt(sys.maxsize + 1)
            with self.assertRaises(Exception):
                testKind.TestOptionalUInt(-7)
            with self.assertRaises(Exception):
                testKind.TestOptionalLong(10e10)


if __name__ == '__main__':
    unittest.main()

