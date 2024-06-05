#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import division

import sys, math
import unittest
from pxr import Gf

def makeValue( Value, vals ):
    v = Value()
    for i in range(v.dimension):
        v[i] = vals[i]
    return v

Sizes = [Gf.Size2, Gf.Size3]

class TestGfSize(unittest.TestCase):

    def runTest(self):
        for Size in Sizes:
            # constructors
            self.assertIsInstance(Size(), Size)
            self.assertIsInstance(Size(Size()), Size)
            
            if Size.dimension == 2:
                self.assertIsInstance(Size(Gf.Vec2i()), Size)
                self.assertIsInstance(Size(3, 4), Size)
                s = Size()
                self.assertEqual(s.Set(3,4), Size(3,4))
            elif Size.dimension == 3:
                self.assertIsInstance(Size(Gf.Vec3i()), Size)
                self.assertIsInstance(Size(3, 4, 5), Size)
                s = Size()
                self.assertEqual(s.Set(3,4,5), Size(3,4,5))

            s = makeValue(Size, (1,2,3))
            self.assertEqual(s, makeValue(Size,(1,2,3)))

            s = makeValue(Size, (1,2,3))
            self.assertNotEqual(s, makeValue(Size,(3,2,1)))

            s1 = makeValue(Size, (1,2,3))
            s2 = makeValue(Size, (3,4,5))
            self.assertEqual(s1 + s2, makeValue(Size, (4,6,8)))
            self.assertEqual(s2 - s1, makeValue(Size, (2,2,2)))
            self.assertEqual(s1 * s2, makeValue(Size, (3,8,15)))

            s1 = makeValue(Size, (1,2,3))
            s1_original = s1
            s1 -= makeValue(Size, (1,1,1))
            self.assertEqual(s1, makeValue(Size, (0,1,2)))
            self.assertTrue(s1 is s1_original)
            s1 = makeValue(Size, (1,2,3))
            s1_original = s1
            s1 += makeValue(Size, (1,1,1))
            self.assertEqual(s1, makeValue(Size, (2,3,4)))
            self.assertTrue(s1 is s1_original)

            s1 = makeValue(Size, (1,2,3))
            s1_original = s1

            s1 *= 10
            self.assertEqual(s1, makeValue(Size, (10,20,30)))
            self.assertTrue(s1 is s1_original)

            s1 = makeValue(Size, (30,60,90))
            s1_original = s1
            s1 /= 10
            self.assertEqual(s1, makeValue(Size, (3,6,9)))
            self.assertTrue(s1 is s1_original)

            self.assertEqual(makeValue(Size,(1,2,3)) * 10, makeValue(Size,(10,20,30)))
            self.assertEqual(10 * makeValue(Size,(1,2,3)), makeValue(Size,(10,20,30)))
            self.assertEqual(makeValue(Size,(10,20,30)) / 10, makeValue(Size,(1,2,3)))

            self.assertEqual(s1, eval(repr(s1)))
            
            self.assertTrue(len(str(makeValue(Size,(1,2,3)))))
            
            # indexing
            s = Size()
            s[-1] = 3
            self.assertEqual(s[-1], 3)
            self.assertTrue(3 in s)
            self.assertEqual(len(s), s.dimension)

            s = makeValue(Size, (1,2,3))
            self.assertTrue(not 10 in s)

            # expect error
            with self.assertRaises(IndexError):
                s[-10] = 3

if __name__ == '__main__':
    unittest.main()
