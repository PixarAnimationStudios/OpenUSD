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
import unittest
import math
from pxr import Gf

class TestGfInterval(unittest.TestCase):

    def runTest(self):
        testIntervals = []
        inf = float("inf")

        # Empty interval
        i0 = Gf.Interval()
        testIntervals.append(i0)
        self.assertTrue(i0.isEmpty)

        # Non-empty: [1,1]
        i1 = Gf.Interval(1)
        testIntervals.append(i1)
        self.assertFalse(i1.isEmpty)
        self.assertEqual(i1.min, 1)
        self.assertEqual(i1.max, 1)
        self.assertTrue(i1.minClosed)
        self.assertTrue(i1.maxClosed)
        self.assertFalse(i1.minOpen)
        self.assertFalse(i1.maxOpen)
        self.assertEqual(i1.size, 0)
        self.assertFalse(i1.Contains(0.99))
        self.assertTrue(i1.Contains(1))
        self.assertFalse(i1.Contains(1.01))

        # Non-empty: [2,4] fully closed
        i2 = Gf.Interval(2, 4)
        testIntervals.append(i2)
        self.assertFalse(i2.isEmpty)
        self.assertEqual(i2.min, 2)
        self.assertEqual(i2.max, 4)
        self.assertTrue(i2.minClosed)
        self.assertTrue(i2.maxClosed)
        self.assertEqual(i2.size, 2)
        self.assertFalse(i2.Contains(1.99))
        self.assertTrue(i2.Contains(2.00))
        self.assertTrue(i2.Contains(2.01))
        self.assertTrue(i2.Contains(3.99))
        self.assertTrue(i2.Contains(4.00))
        self.assertFalse(i2.Contains(4.01))

        # Non-empty: (1,3] half-open
        i3 = Gf.Interval(1, 3, False, True)
        testIntervals.append(i3)
        self.assertFalse(i3.isEmpty)
        self.assertEqual(i3.min, 1)
        self.assertEqual(i3.max, 3)
        self.assertTrue(i3.minOpen)
        self.assertTrue(i3.maxClosed)
        self.assertEqual(i3.size, 2)
        self.assertFalse(i3.Contains(0.99))
        self.assertFalse(i3.Contains(1.00))
        self.assertTrue(i3.Contains(1.01))
        self.assertTrue(i3.Contains(2.99))
        self.assertTrue(i3.Contains(3.00))
        self.assertFalse(i3.Contains(3.01))

        # Non-empty: [-1,2) half-open
        i4 = Gf.Interval(-1, 2, True, False)
        testIntervals.append(i4)
        self.assertFalse(i4.isEmpty)
        self.assertEqual(i4.min, -1)
        self.assertEqual(i4.max, 2)
        self.assertTrue(i4.minClosed)
        self.assertTrue(i4.maxOpen)
        self.assertEqual(i4.size, 3)
        self.assertFalse(i4.Contains(-1.01))
        self.assertTrue(i4.Contains(-1.00))
        self.assertTrue(i4.Contains(-0.99))
        self.assertTrue(i4.Contains(1.99))
        self.assertFalse(i4.Contains(2.00))
        self.assertFalse(i4.Contains(2.01))

        # Non-empty: (2,4) fully open
        i5 = Gf.Interval(2, 4, False, False)
        testIntervals.append(i5)
        self.assertFalse(i5.isEmpty)
        self.assertEqual(i5.min, 2)
        self.assertEqual(i5.max, 4)
        self.assertTrue(i5.minOpen)
        self.assertTrue(i5.maxOpen)
        self.assertEqual(i5.size, 2)
        self.assertFalse(i5.Contains(1.99))
        self.assertFalse(i5.Contains(2.00))
        self.assertTrue(i5.Contains(2.01))
        self.assertTrue(i5.Contains(3.99))
        self.assertFalse(i5.Contains(4.00))
        self.assertFalse(i5.Contains(4.01))

        # Empty degenerate intervals:
        self.assertTrue(Gf.Interval(1, 0).isEmpty)
        self.assertTrue(Gf.Interval(0, 0, False, True).isEmpty)
        self.assertTrue(Gf.Interval(0, 0, True, False).isEmpty)

        # Test (in)equality
        for i in range(len(testIntervals)):
            for j in range(len(testIntervals)):
                self.assertTrue((i==j) == (testIntervals[i] == testIntervals[j]))

        # Test repr()
        for i in testIntervals:
            self.assertEqual(eval(repr(i)), i)

        for i in testIntervals:
            self.assertEqual(i + Gf.Interval(), i, i)
            self.assertEqual(Gf.Interval() | i, i)

        # Test |
        self.assertEqual((i1 | i2), Gf.Interval(1, 4))
        self.assertEqual((i1 | i3), Gf.Interval(1, 3))
        self.assertEqual((i1 | i4), Gf.Interval(-1, 2, True, False))
        self.assertEqual((i1 | i5), Gf.Interval(1, 4, True, False))
        self.assertEqual((i3 | i5), Gf.Interval(1, 4, False, False))

        # Test &
        self.assertTrue((i1 & i2).isEmpty)
        self.assertTrue((i1 & i3).isEmpty)
        self.assertEqual((i1 & i4), i1)
        self.assertTrue((i1 & i5).isEmpty)
        self.assertTrue((i4 & i5).isEmpty) # adjacent open boundaries have no intersection
        self.assertEqual((i3 & i5), Gf.Interval(2, 3, False, True))

        # operators
        i1 = Gf.Interval(10, 20, False, True)
        i1 &= Gf.Interval(15, 25, False, False)
        self.assertEqual(i1.min, 15, ("&="))
        self.assertEqual(i1.max, 20, ("&="))
        self.assertTrue(i1.minOpen)
        self.assertTrue(i1.maxClosed)

        i1 = Gf.Interval(10, 20) & Gf.Interval(15, 25)
        self.assertEqual(i1.min, 15, ("&"))
        self.assertEqual(i1.max, 20, ("&"))
        self.assertTrue(i1.minClosed)
        self.assertTrue(i1.maxClosed)

        i1 = Gf.Interval(10, 20)
        i1 |= Gf.Interval(15, 25)
        self.assertEqual(i1.min, 10, ("|="))
        self.assertEqual(i1.max, 25, ("|="))

        i1 = Gf.Interval(10, 20) | Gf.Interval(15, 25)
        self.assertEqual(i1.min, 10, ("|"))
        self.assertEqual(i1.max, 25, ("|"))

        i1 = Gf.Interval(10, 20)
        i1 += Gf.Interval(30, 40)
        self.assertTrue(i1.min == 40 and i1.max == 60, ("+="))

        i1 = Gf.Interval(10, 20)
        i1 -= Gf.Interval(30, 40)
        self.assertTrue(i1.min == -30 and i1.max == -10, ("-="))

        i1 = Gf.Interval(10, 20)
        i1 *= Gf.Interval(30, 40)
        self.assertTrue(i1.min == 300 and i1.max == 800, ("*="))

        i1 = -Gf.Interval(10, 20)
        self.assertTrue(i1.min == -20 and i1.max == -10, ("unary -"))

        i1 = Gf.Interval(10, 20) + Gf.Interval(20, 30)
        self.assertTrue(i1.min == 30 and i1.max == 50, ("+"))

        i1 = Gf.Interval(10, 20) - Gf.Interval(20, 30)
        self.assertTrue(i1.min == -20 and i1.max == 0, ("-"))

        i1 = Gf.Interval(10, 20) * Gf.Interval(20, 30)
        self.assertEqual(i1.min, 200)
        self.assertEqual(i1.max, 600)
        self.assertTrue(i1.minClosed)
        self.assertTrue(i1.maxClosed)

        i1 = Gf.Interval(10, 20, False, True) * Gf.Interval(20, 30, True, False)
        self.assertEqual(i1.min, 200)
        self.assertEqual(i1.max, 600)
        self.assertTrue(i1.minOpen)
        self.assertTrue(i1.maxOpen)

        i1 = Gf.Interval(-10, 20, False, True) * Gf.Interval(-20, 30, True, False)
        self.assertEqual(i1.min, -400)
        self.assertEqual(i1.max, 600)
        self.assertTrue(i1.minClosed)
        self.assertTrue(i1.maxOpen)

        i1 = Gf.Interval.GetFullInterval()
        i2 = Gf.Interval(0, 1)
        self.assertEqual(i1, Gf.Interval(-inf, inf, False, False))
        self.assertEqual((i1 & i2), i2)

        #Test normalization of all infinite intervals as open intervals
        i1 = Gf.Interval.GetFullInterval()
        i2 = Gf.Interval(-inf, inf, False, False)
        i3 = Gf.Interval(-inf, inf, True, False)
        i4 = Gf.Interval(-inf, inf, False, True)
        i5 = Gf.Interval(-inf, inf, True, True)
        i6 = Gf.Interval(-inf, inf)
        self.assertEqual(i1, i2)
        self.assertEqual(i1, i3)
        self.assertEqual(i1, i4)
        self.assertEqual(i1, i5)
        self.assertEqual(i1, i6)

        # Test bounds
        i = Gf.Interval.GetFullInterval()
        self.assertFalse(i.minFinite)
        self.assertFalse(i.maxFinite)
        self.assertFalse(i.finite)

        i = Gf.Interval(-inf, -inf)
        self.assertFalse(i.minFinite)
        self.assertFalse(i.maxFinite)
        self.assertFalse(i.finite)

        i = Gf.Interval(inf, inf)
        self.assertFalse(i.minFinite)
        self.assertFalse(i.maxFinite)
        self.assertFalse(i.finite)

        i = Gf.Interval(inf, 0.0)
        self.assertFalse(i.minFinite)
        self.assertTrue(i.maxFinite)
        self.assertFalse(i.finite)

        i = Gf.Interval(0.0, inf)
        self.assertTrue(i.minFinite)
        self.assertFalse(i.maxFinite)
        self.assertFalse(i.finite)

        i = Gf.Interval(0.0, 1.0)
        self.assertTrue(i.minFinite)
        self.assertTrue(i.maxFinite)
        self.assertTrue(i.finite)

        # Test representations
        self.assertTrue(i1.min == eval(repr(i1)).min and \
            i1.max == eval(repr(i1)).max, ("repr"))

        self.assertTrue(len(str(Gf.Interval())), ("str"))

if __name__ == '__main__':
    unittest.main()
