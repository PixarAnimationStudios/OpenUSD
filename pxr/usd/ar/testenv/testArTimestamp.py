#!/pxrpythonsubst
#
# Copyright 2021 Pixar
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

