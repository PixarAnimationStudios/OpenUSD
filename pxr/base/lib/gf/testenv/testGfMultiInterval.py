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

########################################################################
# Utilities

def subsets(l):
    '''yield the subsets of list l'''
    for bits in range(2**len(l)):
        yield [l[i] for i in range(len(l)) if bits & (2**i)]

def permute(l):
    '''yield the permutations of list l'''
    if len(l) <= 1:
        yield l
    else:
        for i in range(len(l)):
            cur = l[i]
            rest = l[:i]+l[i+1:]
            for p in permute(rest):
                yield [cur]+p

class TestGfMultiInterval(unittest.TestCase):
    def runTest(self):

        # Test utility functions
        self.assertEqual(list(subsets([1,2,3])),
            [[], [1], [2], [1, 2], [3], [1, 3], [2, 3], [1, 2, 3]])
        self.assertEqual(list(permute([1,2,3])), 
            [[1,2,3],[1,3,2],[2,1,3],[2,3,1],[3,1,2],[3,2,1]])

        intervals = []
        intervals.append(Gf.Interval(      ))
        intervals.append(Gf.Interval(-2, -1))
        intervals.append(Gf.Interval( 1    ))
        intervals.append(Gf.Interval( 2,  5))
        intervals.append(Gf.Interval( 3,  4))
        intervals.append(Gf.Interval( 5,  7))
        intervals.append(Gf.Interval( 6,  8))

        testSets = map(Gf.MultiInterval, intervals)

        # Test empty multi-intervals
        self.assertEqual(Gf.MultiInterval(), Gf.MultiInterval())
        self.assertEqual(Gf.MultiInterval(), Gf.MultiInterval( Gf.Interval() ))
        self.assertNotEqual(Gf.MultiInterval(), Gf.MultiInterval( Gf.Interval(1) ))

        # Test == and !=
        for i in range(len(testSets)):
            for j in range(i, len(testSets)):
                self.assertEqual((i==j), (testSets[i]==testSets[j]))
                self.assertEqual((i!=j), (testSets[i]!=testSets[j]))

        # Test repr and copy ctor
        for s in testSets:
            self.assertEqual(eval(repr(s)), s)
            self.assertEqual(Gf.MultiInterval(s), s)

        # Test that Add()ing intervals in any order yields same result
        expected = None
        for p in permute(intervals):
            x = Gf.MultiInterval()
            for i in p:
                x.Add(i)
            if expected is None:
                expected = x
            else:
                self.assertEqual(expected, x)

        # Track unique multi-intervals that we synthesize
        uniqueSets = set()
        num = 7

        # Test Add()
        for s in subsets(range(num)):
            for p in permute(s):
                x = Gf.MultiInterval()
                for i in p:
                    x.Add( Gf.Interval(i, i+1.0, True, False) )
                # Verify
                for i in range(num):
                    if i in p:
                        # Should have been added
                        self.assertTrue(x.Contains(i+0.5))
                    else:
                        # Should not have been added
                        self.assertFalse(x.Contains(i+0.5))
                # Accumulate unique multi-intervals we construct
                uniqueSets.add(x)

        # We expect 2**num unique sets, corresponding to the power-set of range(num).
        self.assertEqual(len(uniqueSets), 2**num)

        # Test Remove()
        for s in subsets(range(num)):
            for p in permute(s):
                x = Gf.MultiInterval( Gf.Interval(0, num) )
                for i in p:
                    x.Remove( Gf.Interval(i, i+1.0) )
                # Verify
                for i in range(num):
                    if i in p:
                        # Should have been removed
                        self.assertFalse(x.Contains(i+0.5))
                    else:
                        # Should have been retained
                        self.assertTrue(x.Contains(i+0.5))

        #Test add optimizing the interval merging for open and closed intervals
        num = 5
        for r in range(num):
            # Testing only on at least two intervals
            s = r + 2
            for p in permute(range(s)):
                # Adding range of [i, i+1) intervals should leave one interval in the set
                x = Gf.MultiInterval()
                for i in p:
                    x.Add( Gf.Interval(i, i+1.0, True, False) )
                print x
                self.assertEqual(x.bounds, Gf.Interval(0, s, True, False))
                self.assertTrue(x.Contains(Gf.Interval(0, s, True, False)))
                self.assertFalse(x.Contains(Gf.Interval(0, s, True, True)))
                self.assertEqual(x.size, 1)

                # Adding range of (i, i+1] intervals should leave one interval in the set
                x = Gf.MultiInterval()
                for i in p:
                    x.Add( Gf.Interval(i, i+1.0, False, True) )
                print x
                self.assertEqual(x.bounds, Gf.Interval(0, s, False, True))
                self.assertTrue(x.Contains(Gf.Interval(0, s, False, True)))
                self.assertFalse(x.Contains(Gf.Interval(0, s, True, True)))
                self.assertEqual(x.size, 1)

                # Adding range of [i, i+1] intervals should leave one interval in the set
                x = Gf.MultiInterval()
                for i in p:
                    x.Add( Gf.Interval(i, i+1.0, True, True) )
                print x
                self.assertEqual(x.bounds, Gf.Interval(0, s, True, True))
                self.assertTrue(x.Contains(Gf.Interval(0, s, True, True)))
                self.assertEqual(x.size, 1)

                # Adding range of (i, i+1) intervals should leave the size of range 
                # number of open intervals in the set
                x = Gf.MultiInterval()
                for i in p:
                    x.Add( Gf.Interval(i, i+1.0, False, False) )
                print x
                self.assertEqual(x.bounds, Gf.Interval(0, s, False, False))
                self.assertFalse(x.Contains(Gf.Interval(0, s, False, False)))
                self.assertEqual(x.size, s)

        # Test Remove() edge cases
        a = Gf.Interval( 0, 1, True, False )
        b = Gf.Interval( 0, 1, False, False )
        s = Gf.MultiInterval(a)
        s.Remove(b)
        self.assertEqual(s, Gf.MultiInterval( Gf.Interval(0,0) ))
        a = Gf.Interval( 0, 1, True, True )
        b = Gf.Interval( 0, 1, False, False )
        s = Gf.MultiInterval(a)
        s.Remove(b)
        self.assertEqual(s, Gf.MultiInterval( [Gf.Interval(0,0), Gf.Interval(1,1)] ))
        s = Gf.MultiInterval( [Gf.Interval(0,1), Gf.Interval(3,4)] )
        a = Gf.Interval( 0, 4, False, False )
        s.Remove(a)
        self.assertEqual(s, Gf.MultiInterval( [Gf.Interval(0,0), Gf.Interval(4,4)] ) )

        # Test GetComplement()
        for s in testSets:
            self.assertNotEqual(s.GetComplement(), s)
            self.assertEqual(s.GetComplement().GetComplement(), s)

        # Test that GetComplement() for empty multi-interval is the multi-interval
        # containing the full interval.
        s = Gf.MultiInterval()
        self.assertEqual(s.GetComplement(), Gf.MultiInterval(Gf.Interval.GetFullInterval()))
        self.assertEqual(s.GetComplement().GetComplement(), s)

        # Test Contains() with double values
        s = Gf.MultiInterval( [Gf.Interval(1,2), Gf.Interval(3,4)] )
        self.assertFalse(s.Contains(0.99))
        self.assertTrue(s.Contains(1.00))
        self.assertTrue(s.Contains(1.01))
        self.assertTrue(s.Contains(1.99))
        self.assertTrue(s.Contains(2.00))
        self.assertFalse(s.Contains(2.01))
        self.assertFalse(s.Contains(2.99))
        self.assertTrue(s.Contains(3.00))
        self.assertTrue(s.Contains(3.01))
        self.assertTrue(s.Contains(3.99))
        self.assertTrue(s.Contains(4.00))
        self.assertFalse(s.Contains(4.01))
        # Test Contains() with intervals
        self.assertFalse(s.Contains( Gf.Interval() ))
        self.assertFalse(s.Contains( Gf.Interval(0.99, 2) ))            # closed but larger
        self.assertTrue(s.Contains( Gf.Interval(1, 2, False, True) ))  # half-open
        self.assertTrue(s.Contains( Gf.Interval(1, 2, True, False) ))  # half-open
        self.assertTrue(s.Contains( Gf.Interval(1, 2, True, True) ))   # totally contained
        # Test Contains() with multi-intervals
        self.assertFalse(s.Contains( Gf.MultiInterval() ))
        self.assertTrue(s.Contains( s ))
        self.assertTrue(s.Contains( Gf.MultiInterval( [Gf.Interval(1,2)] ) ))
        self.assertTrue(s.Contains( Gf.MultiInterval( [Gf.Interval(3,4)] ) ))
        self.assertTrue(s.Contains( Gf.MultiInterval( [Gf.Interval(1,1.3), Gf.Interval(1.6,2)] ) ))
        self.assertFalse(s.Contains( Gf.MultiInterval( [Gf.Interval(1,4)] ) ))
        self.assertFalse(s.Contains( Gf.MultiInterval( [Gf.Interval(1,2), Gf.Interval(3,5)] ) ))
        self.assertFalse(s.Contains( Gf.MultiInterval( [Gf.Interval(1,2), Gf.Interval(3,4), Gf.Interval(5,6)] ) ))


        # Test iterator

        multiInterval = Gf.MultiInterval( [Gf.Interval( 2, 4),
                                        Gf.Interval(-1, 3),
                                        Gf.Interval( 6, 7)])

        intervals = [ Gf.Interval(-1, 4),
                    Gf.Interval( 6, 7) ]

        number = 0

        for m, i in zip(multiInterval, intervals):
            self.assertEqual(m, i)
            number += 1

        self.assertEqual(number, len(intervals))

        # Test ArithmeticAdd
        a = Gf.MultiInterval( [
                Gf.Interval( -10,   1, True,  False),
                Gf.Interval(   3,   4, False, True ),
                Gf.Interval( 100, 200, False, True),
                Gf.Interval( 201, 300, False, True)])

        a.ArithmeticAdd(Gf.Interval( -3, -1, False, True ))

        result = Gf.MultiInterval( [
                Gf.Interval( -13,   0, False, False),
                Gf.Interval(   0,   3, False, True ),
                Gf.Interval(  97, 299, False, True)])

        self.assertEqual(a, result)

        # XXX test Intersect
        # XXX test Bounds

if __name__ == '__main__':
    unittest.main()

