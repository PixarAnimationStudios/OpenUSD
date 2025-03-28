#!/pxrpythonsubst
#
# Copyright 2021 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

import math
import unittest

from pxr import Ar, Tf

class TestArTimestamp(unittest.TestCase):
    def testBasic(self):
        timestamp = Ar.Timestamp(94608)
        self.assertTrue(timestamp.IsValid())
        self.assertEqual(repr(timestamp), 'Ar.Timestamp(94608.0)')
        self.assertEqual(timestamp.GetTime(), 94608)
        
        self.assertEqual(timestamp, Ar.Timestamp(94608))
        self.assertFalse(timestamp < Ar.Timestamp(94608))
        self.assertTrue(timestamp <= Ar.Timestamp(94608))
        self.assertFalse(timestamp > Ar.Timestamp(94608))
        self.assertTrue(timestamp >= Ar.Timestamp(94608))

        self.assertNotEqual(timestamp, Ar.Timestamp())
        self.assertTrue(timestamp > Ar.Timestamp())
        self.assertTrue(timestamp >= Ar.Timestamp())
        self.assertFalse(timestamp < Ar.Timestamp())
        self.assertFalse(timestamp <= Ar.Timestamp())

        self.assertNotEqual(timestamp, Ar.Timestamp(0.0))
        self.assertTrue(timestamp > Ar.Timestamp(0.0))
        self.assertTrue(timestamp >= Ar.Timestamp(0.0))
        self.assertFalse(timestamp < Ar.Timestamp(0.0))
        self.assertFalse(timestamp <= Ar.Timestamp(0.0))

        self.assertIsNotNone(hash(timestamp))

    def testInvalid(self):
        invalidTimestamp = Ar.Timestamp()
        self.assertFalse(invalidTimestamp.IsValid())
        self.assertEqual(repr(invalidTimestamp), 'Ar.Timestamp()')

        with self.assertRaises(Tf.ErrorException):
            self.assertTrue(math.isnan(invalidTimestamp.GetTime()))

        self.assertEqual(invalidTimestamp, Ar.Timestamp())
        self.assertTrue(invalidTimestamp >= Ar.Timestamp())
        self.assertTrue(invalidTimestamp <= Ar.Timestamp())

        self.assertNotEqual(invalidTimestamp, Ar.Timestamp(0.0))
        self.assertTrue(invalidTimestamp < Ar.Timestamp(0.0))
        self.assertTrue(invalidTimestamp <= Ar.Timestamp(0.0))
        self.assertFalse(invalidTimestamp > Ar.Timestamp(0.0))
        self.assertFalse(invalidTimestamp >= Ar.Timestamp(0.0))

        self.assertIsNotNone(hash(invalidTimestamp))

if __name__ == "__main__":
    unittest.main()

