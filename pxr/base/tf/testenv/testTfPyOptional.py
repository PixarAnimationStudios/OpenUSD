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

Test = Tf.Tf_TestPyOptional

# sys.maxint was removed in python 3, because all ints are longs in the
# newer version. Note that on some python 2 systems this was (2**64)-1,
# for instance, that was the case on centos 64 bit cpython.
# Choosing a 32 bit int here arbitrarily, this was the value on windows
# in a 64 bit build.
if sys.version_info.major >= 3:
    maxint = (2 ** 31) - 1
else:
    maxint = sys.maxint

class TestTfPyOptional(unittest.TestCase):

    def test_OptionalArgs(self):
        self.assertEqual(Test.TakesOptional(), (None, None))
        self.assertEqual(Test.TakesOptional(None), (None, None))
        self.assertEqual(Test.TakesOptional(None, None), (None, None))
        self.assertEqual(Test.TakesOptional('abc', None), ('abc', None))
        self.assertEqual(Test.TakesOptional(None, []), (None, []))
        self.assertEqual(Test.TakesOptional(None, ['a', 'b', 'c']), (None, ['a','b','c']))
        self.assertEqual(Test.TakesOptional('', []), ('', []))
        self.assertEqual(Test.TakesOptional('abc', ['a', 'b', 'c']), ('abc', ['a','b','c']))

    def test_OptionalReturns(self):
        self.assertEqual(Test.TestOptionalString(None), None)
        self.assertEqual(Test.TestOptionalStringVector(['string', 'list']), ['string', 'list'])
        self.assertEqual(Test.TestOptionalString('string'), 'string')
        self.assertEqual(Test.TestOptionalDouble(7e-7), 7e-7)
        self.assertEqual(Test.TestOptionalDouble(-7e-7), -7e-7)
        self.assertAlmostEqual(Test.TestOptionalFloat(7e-7), 7e-7)
        self.assertAlmostEqual(Test.TestOptionalFloat(7e-7), 7e-7)
        self.assertEqual(Test.TestOptionalLong(-maxint - 1), -maxint - 1)
        self.assertEqual(Test.TestOptionalULong(maxint + 1),  maxint + 1)
        self.assertEqual(Test.TestOptionalInt(-7), -7)
        self.assertEqual(Test.TestOptionalUInt(7), 7)
        self.assertEqual(Test.TestOptionalChar('c'), 'c')
        self.assertEqual(Test.TestOptionalUChar(63), 63)

    def test_OptionalBadArgs(self):
        # Boost throws a Boost.Python.ArgumentError
        with self.assertRaises(Exception):
            Test.TestOptionalString(43)
        with self.assertRaises(Exception):
            Test.TestOptionalInt(sys.maxsize + 1)
        with self.assertRaises(Exception):
            Test.TestOptionalUInt(-7)
        with self.assertRaises(Exception):
            Test.TestOptionalLong(10e10)


if __name__ == '__main__':
    unittest.main()

