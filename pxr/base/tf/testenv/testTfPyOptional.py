#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

