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

TestStd = Tf.Tf_TestPyOptionalStd

maxint = (2 ** 31) - 1

class TestTfPyOptional(unittest.TestCase):

    def test_OptionalArgs(self):
        self.assertEqual(TestStd.TakesOptional(), (None, None))
        self.assertEqual(TestStd.TakesOptional(None), (None, None))
        self.assertEqual(TestStd.TakesOptional(None, None), (None, None))
        self.assertEqual(TestStd.TakesOptional('abc', None), ('abc', None))
        self.assertEqual(TestStd.TakesOptional(None, []), (None, []))
        self.assertEqual(TestStd.TakesOptional(None, ['a', 'b', 'c']), (None, ['a','b','c']))
        self.assertEqual(TestStd.TakesOptional('', []), ('', []))
        self.assertEqual(TestStd.TakesOptional('abc', ['a', 'b', 'c']), ('abc', ['a','b','c']))

    def test_OptionalReturns(self):
        self.assertEqual(TestStd.TestOptionalString(None), None)
        self.assertEqual(TestStd.TestOptionalStringVector(['string', 'list']), ['string', 'list'])
        self.assertEqual(TestStd.TestOptionalString('string'), 'string')
        self.assertEqual(TestStd.TestOptionalDouble(7e-7), 7e-7)
        self.assertEqual(TestStd.TestOptionalDouble(-7e-7), -7e-7)
        self.assertAlmostEqual(TestStd.TestOptionalFloat(7e-7), 7e-7)
        self.assertAlmostEqual(TestStd.TestOptionalFloat(7e-7), 7e-7)
        self.assertEqual(TestStd.TestOptionalLong(-maxint - 1), -maxint - 1)
        self.assertEqual(TestStd.TestOptionalULong(maxint + 1),  maxint + 1)
        self.assertEqual(TestStd.TestOptionalInt(-7), -7)
        self.assertEqual(TestStd.TestOptionalUInt(7), 7)
        self.assertEqual(TestStd.TestOptionalChar('c'), 'c')
        self.assertEqual(TestStd.TestOptionalUChar(63), 63)

    def test_OptionalBadArgs(self):
        # Boost throws a Boost.Python.ArgumentError
        with self.assertRaises(Exception):
            TestStd.TestOptionalString(43)
        with self.assertRaises(Exception):
            TestStd.TestOptionalInt(sys.maxsize + 1)
        with self.assertRaises(Exception):
            TestStd.TestOptionalUInt(-7)
        with self.assertRaises(Exception):
            TestStd.TestOptionalLong(10e10)


if __name__ == '__main__':
    unittest.main()

