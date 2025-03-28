#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function
from __future__ import division

import sys, math
import unittest
from pxr import Gf, Tf

def makeValue( Value, vals ):
    if Value == float:
        return Value(vals[0])
    else:
        v = Value()
        for i in range(v.dimension):
            v[i] = vals[i]
    return v

class TestGfRange(unittest.TestCase):
    Ranges = [(Gf.Range1d, float), 
            (Gf.Range2d, Gf.Vec2d),
            (Gf.Range3d, Gf.Vec3d), 
            (Gf.Range1f, float), 
            (Gf.Range2f, Gf.Vec2f), 
            (Gf.Range3f, Gf.Vec3f)]

    def runTest(self):
        for Range, Value in self.Ranges:
            # constructors
            self.assertIsInstance(Range(), Range)
            self.assertIsInstance(Range(Value(), Value()), Range)

            v = makeValue(Value, [1, 2, 3, 4])
            r = Range()
            r.min = v
            self.assertEqual(r.min, v)
            r.max = v
            self.assertEqual(r.max, v)

            r = Range()
            self.assertTrue(r.IsEmpty())
            
            v1 = makeValue(Value, [-1, -2, -3, -4])
            v2 = makeValue(Value, [1, 2, 3, 4]) 
            r = Range(v2, v1)
            self.assertTrue(r.IsEmpty())
            r = Range(v1, v2)
            self.assertFalse(r.IsEmpty())
            r.SetEmpty()
            self.assertTrue(r.IsEmpty())

            r = Range(v1, v2)
            self.assertEqual(r.GetSize(), v2 - v1)

            v1 = makeValue(Value, [-1, 1, -11, 2])
            v2 = makeValue(Value, [1, 3, -10, 2])
            v3 = makeValue(Value, [0, 2, -10.5, 2])
            v4 = makeValue(Value, [0, 0, 0, 0])
            r1 = Range(v1,v2)
            self.assertEqual(r1.GetMidpoint(), v3)
            r1.SetEmpty()
            r2 = Range()
            self.assertEqual(r1.GetMidpoint(), v4)
            self.assertEqual(r2.GetMidpoint(), v4)
            
            v1 = makeValue(Value, [-1, -2, -3, -4])
            v2 = makeValue(Value, [1, 2, 3, 4])
            r = Range(v1,v2)
            v1 = makeValue(Value, [0, 0, 0, 0])
            v2 = makeValue(Value, [2, 3, 4, 5])
            self.assertTrue(r.Contains(v1))
            self.assertFalse(r.Contains(v2))


            v1 = makeValue(Value, [-2, -4, -6, -8])
            v2 = makeValue(Value, [2, 4, 6, 8])
            r1 = Range(v1,v2)

            v1 = makeValue(Value, [-1, -2, -3, -4])
            v2 = makeValue(Value, [1, 2, 3, 4])
            r2 = Range(v1, v2)

            self.assertTrue(r1.Contains(r2))
            self.assertFalse(r2.Contains(r1))

            v1 = makeValue(Value, [-1, -2, -3, -4])
            v2 = makeValue(Value, [2, 4, 6, 8])
            v3 = makeValue(Value, [-2, -4, -6, -8])
            v4 = makeValue(Value, [1, 2, 3, 4])
            r1 = Range(v1, v2)
            r2 = Range(v3, v4)
            self.assertEqual(Range.GetUnion(r1, r2), Range(v3, v2))
            self.assertEqual(Range.GetIntersection(r1, r2), Range(v1, v4))

            r1 = Range(v1, v2)
            self.assertEqual(r1.UnionWith(r2), Range(v3, v2))
            r1 = Range(v1, v2)
            self.assertEqual(r1.UnionWith(v3), Range(v3, v2))

            r1 = Range(v1, v2)
            self.assertEqual(r1.IntersectWith(r2), Range(v1, v4))

            r1 = Range(v1, v2)
            v5 = makeValue(Value, [100,100,100,100])
            dsqr = r1.GetDistanceSquared(v5)
            if Value == float:
                self.assertEqual(dsqr, 9604)
            elif Value.dimension == 2:
                self.assertEqual(dsqr, 18820)
            elif Value.dimension == 3:
                self.assertEqual(dsqr, 27656)
            else:
                self.fail()

            r1 = Range(v1, v2)
            v5 = makeValue(Value, [-100,-100,-100,-100])
            dsqr = r1.GetDistanceSquared(v5)
            if Value == float:
                self.assertEqual(dsqr, 9801)
            elif Value.dimension == 2:
                self.assertEqual(dsqr, 19405)
            elif Value.dimension == 3:
                self.assertEqual(dsqr, 28814)
            else:
                self.fail()


            v1 = makeValue(Value, [1, 2, 3, 4])
            v2 = makeValue(Value, [2, 3, 4, 5])
            v3 = makeValue(Value, [3, 4, 5, 6])
            v4 = makeValue(Value, [4, 5, 6, 7])
            
            r1 = Range(v1, v2)
            r2 = Range(v2, v3)
            r3 = Range(v3, v4)
            r4 = Range(v4, v5)

            self.assertEqual(r1 + r2, Range(makeValue(Value, [3, 5, 7, 9]), 
                makeValue(Value, [5, 7, 9, 11])))
            
            self.assertEqual(r1 - r2, 
                    Range(makeValue(Value, [-2, -2, -2, -2]), 
                        makeValue(Value, [0, 0, 0, 0])))
            
            self.assertEqual(r1 * 10,
                Range(makeValue(Value, [10, 20, 30, 40]),
                        makeValue(Value, [20, 30, 40, 50])))

            self.assertEqual(10 * r1,
                Range(makeValue(Value, [10, 20, 30, 40]),
                        makeValue(Value, [20, 30, 40, 50])))
            tmp = r1/10
            self.assertTrue(Gf.IsClose(tmp.min, makeValue(Value, [0.1, 0.2, 0.3, 0.4]), 0.00001) and \
                Gf.IsClose(tmp.max, makeValue(Value, [0.2, 0.3, 0.4, 0.5]), 0.00001))
            
            self.assertEqual(r1, Range(makeValue(Value, [1, 2, 3, 4]),
                            makeValue(Value, [2, 3, 4, 5])))

            self.assertFalse(r1 != Range(makeValue(Value, [1, 2, 3, 4]),
                                makeValue(Value, [2, 3, 4, 5])))

            
            r1 = Range(v1, v2)
            r1_original = r1
            r2 = Range(v2, v3)

            r1 += r2
            self.assertEqual(r1, Range(makeValue(Value, [3, 5, 7, 9]),
                            makeValue(Value, [5, 7, 9, 11])))
            self.assertTrue(r1 is r1_original)
            
            r1 = Range(v1, v2)
            r1_original = r1
            r1 -= r2
            self.assertEqual(r1, Range(makeValue(Value, [-2, -2, -2, -2]),
                            makeValue(Value, [0, 0, 0, 0])))
            self.assertTrue(r1 is r1_original)

            r1 = Range(v1, v2)
            r1_original = r1
            r1 *= 10
            self.assertEqual(r1, Range(makeValue(Value, [10, 20, 30, 40]),
                            makeValue(Value, [20, 30, 40, 50])))
            self.assertTrue(r1 is r1_original)
            
            r1 = Range(v1, v2)
            r1_original = r1
            r1 *= -10
            self.assertEqual(r1, Range(makeValue(Value, [-20, -30, -40, -50]),
                            makeValue(Value, [-10, -20, -30, -40])))
            self.assertTrue(r1 is r1_original)

            r1 = Range(v1, v2)
            r1_original = r1
            r1 /= 10
            self.assertTrue(Gf.IsClose(r1.min, makeValue(Value, [0.1, 0.2, 0.3, 0.4]), 0.00001) and
                Gf.IsClose(r1.max, makeValue(Value, [0.2, 0.3, 0.4, 0.5]), 0.00001))
            self.assertTrue(r1 is r1_original)

            self.assertEqual(r1, eval(repr(r1)))
            
            self.assertTrue(len(str(Range())))

        # now test GetCorner and GetQuadrant for Gf.Range2f and Gf.Range2d
        Ranges = [(Gf.Range2f, Gf.Vec2f), \
                (Gf.Range2d, Gf.Vec2d)]

        for Range, Value in Ranges:
            rf = Range.unitSquare
            self.assertTrue(rf.GetCorner(0) == Value(0,0) and \
                rf.GetCorner(1) == Value(1,0) and \
                rf.GetCorner(2) == Value(0,1) and \
                rf.GetCorner(3) == Value(1,1))

            # try a bogus corner
            print('=== EXPECT ERRORS ===', file=sys.stderr)
            try:
                rf.GetCorner(4)
                self.fail()
            except Tf.ErrorException:
                pass
            print('=== End of expected errors ===', file=sys.stderr)

            self.assertTrue(rf.GetQuadrant(0) == Range( Value(0,0), Value(.5, .5) ) and \
                rf.GetQuadrant(1) == Range( Value(.5,0), Value(1, .5) ) and \
                rf.GetQuadrant(2) == Range( Value(0, .5), Value(.5,1) ) and \
                rf.GetQuadrant(3) == Range( Value(.5, .5), Value(1,1) ))

            # try a bogus quadrant
            print('=== EXPECT ERRORS ===', file=sys.stderr)
            with self.assertRaises(Tf.ErrorException):
                rf.GetQuadrant(4)
            print('=== End of expected errors ===', file=sys.stderr)

        # now test GetCorner and GetOctant for Gf.Range3f and Gf.Range3d
        Ranges = [(Gf.Range3f, Gf.Vec3f), \
                (Gf.Range3d, Gf.Vec3d)]

        for Range, Value in Ranges:
            rf = Range.unitCube
            self.assertTrue(rf.GetCorner(0) == Value(0,0,0) and \
                rf.GetCorner(1) == Value(1,0,0) and \
                rf.GetCorner(2) == Value(0,1,0) and \
                rf.GetCorner(3) == Value(1,1,0) and \
                rf.GetCorner(4) == Value(0,0,1) and \
                rf.GetCorner(5) == Value(1,0,1) and \
                rf.GetCorner(6) == Value(0,1,1) and \
                rf.GetCorner(7) == Value(1,1,1))

            # try a bogus corner
            print('=== EXPECT ERRORS ===', file=sys.stderr)
            with self.assertRaises(Tf.ErrorException):
                rf.GetCorner(8)
            print('=== End of expected errors ===', file=sys.stderr)

            vals = [[(0.0, 0.0, 0.0), (0.5, 0.5, 0.5)],
                    [(0.5, 0.0, 0.0), (1.0, 0.5, 0.5)],
                    [(0.0, 0.5, 0.0), (0.5, 1.0, 0.5)],
                    [(0.5, 0.5, 0.0), (1.0, 1.0, 0.5)],
                    [(0.0, 0.0, 0.5), (0.5, 0.5, 1.0)],
                    [(0.5, 0.0, 0.5), (1.0, 0.5, 1.0)],
                    [(0.0, 0.5, 0.5), (0.5, 1.0, 1.0)],
                    [(0.5, 0.5, 0.5), (1.0, 1.0, 1.0)]]

            ranges = [Range(Value(v[0]), Value(v[1])) for v in vals]
            for i in range(8):
                self.assertEqual(rf.GetOctant(i), ranges[i])

            with self.assertRaises(OverflowError):
                rf.GetOctant(-1)

            with self.assertRaises(Tf.ErrorException):
                rf.GetOctant(8)

        # now test comparisons between ranges of different types
        # and constructors converting between different types.
        Ranges = [(Gf.Range1f, float,    Gf.Range1d, float),
                (Gf.Range2f, Gf.Vec2f, Gf.Range2d, Gf.Vec2d),
                (Gf.Range3f, Gf.Vec3f, Gf.Range3d, Gf.Vec3d)]


        for Rangef, Valuef, Ranged, Valued in Ranges:
            v1f = makeValue(Valuef, [-1, -2, -3, -4])
            v2f = makeValue(Valuef, [2, 4, 6, 8])
            v3f = makeValue(Valuef, [-2, -4, -6, -8])
            v4f = makeValue(Valuef, [1, 2, 3, 4])
            r1f = Rangef(v1f, v2f)
            r2f = Rangef(v3f, v4f)

            v1d = makeValue(Valued, [-1, -2, -3, -4])
            v2d = makeValue(Valued, [2, 4, 6, 8])
            v3d = makeValue(Valued, [-2, -4, -6, -8])
            v4d = makeValue(Valued, [1, 2, 3, 4])
            r1d = Ranged(v1d, v2d)
            r2d = Ranged(v3d, v4d)

            self.assertEqual(Rangef.GetUnion(r1f, r2f), Ranged(v3d, v2d))
            self.assertEqual(Ranged.GetUnion(r1d, r2d), Rangef(v3f, v2f))
            self.assertEqual(Rangef.GetIntersection(r1f, r2f), Ranged(v1d, v4d))
            self.assertEqual(Ranged.GetIntersection(r1d, r2d), Rangef(v1f, v4f))

            # Construct double from float type
            self.assertEqual(Ranged(r1f), r1d)

            # Construct float from double type
            self.assertEqual(Rangef(r1d), r1f)

    def test_Hash(self):
        for RangeType, ValueType in self.Ranges:
            r = RangeType(
                makeValue(ValueType, [-1.0, -2.0, -3.0, -4.0]),
                makeValue(ValueType, [4.0, 3.0, 2.0, 1.0])
            )
            self.assertEqual(hash(r), hash(r))
            self.assertEqual(hash(r), hash(RangeType(r)))

if __name__ == '__main__':
    unittest.main()
