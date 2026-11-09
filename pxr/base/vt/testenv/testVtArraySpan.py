#!/pxrpythonsubst
#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

import unittest

from pxr import Gf, Vt

class TestVtArraySpan(unittest.TestCase):

    def test_ArraysAreViewed(self):
        # VtArrays are registered span sources, so spans refer to the array's
        # own elements, whatever the element type.
        ints = Vt.IntArray(range(10))
        self.assertEqual(Vt._test_IntSpanAddress(ints),
                         Vt._test_IntArrayAddress(ints))

        strs = Vt.StringArray(['a', 'bc', 'def'])
        self.assertEqual(Vt._test_StringSpanAddress(strs),
                         Vt._test_StringArrayAddress(strs))
        self.assertEqual(Vt._test_StringSpanToArray(strs), strs)

        vecs = Vt.Vec3fArray([Gf.Vec3f(1), Gf.Vec3f(2)])
        self.assertEqual(Vt._test_Vec3fSpanAddress(vecs),
                         Vt._test_Vec3fArrayAddress(vecs))

    def test_SequencesAreCopied(self):
        # This used to produce a span referring to a destroyed temporary
        # VtArray (USD-11894).  A large size makes reads of freed storage
        # likely to show up as wrong values, and lets valgrind see them.
        n = 100000
        self.assertEqual(Vt._test_IntSpanToArray(list(range(n))),
                         Vt.IntArray(range(n)))
        self.assertEqual(Vt._test_StringSpanToArray(['x', 'yz']),
                         Vt.StringArray(['x', 'yz']))

    def test_MutableSpans(self):
        ints = Vt.IntArray([0, 0, 0])
        Vt._test_FillIntSpan(ints, 4)
        self.assertEqual(ints, Vt.IntArray([4, 4, 4]))

        vecs = Vt.Vec3fArray(2)
        Vt._test_FillVec3fSpan(vecs, Gf.Vec3f(1, 2, 3))
        self.assertEqual(vecs, Vt.Vec3fArray([Gf.Vec3f(1, 2, 3)] * 2))

        # Writing detaches an array that shares its elements, so the other
        # array is unchanged.
        a = Vt.IntArray([1, 2, 3])
        b = Vt.IntArray(a)
        Vt._test_FillIntSpan(b, 9)
        self.assertEqual(a, Vt.IntArray([1, 2, 3]))
        self.assertEqual(b, Vt.IntArray([9, 9, 9]))

        # Writes to a temporary would be lost, so sequences are rejected.
        with self.assertRaises(TypeError):
            Vt._test_FillIntSpan([0, 0], 1)

    def test_ArraysOfOtherTypes(self):
        # A VtArray of another element type is not a span source for this
        # one, but it is a sequence, so its elements are copied if each
        # converts.  Python floats do not convert to int.
        doubles = Vt.DoubleArray([1, 2])
        with self.assertRaises(TypeError):
            Vt._test_IntSpanToArray(doubles)
        int64s = Vt.Int64Array([1, 2])
        self.assertEqual(Vt._test_IntSpanToArray(int64s), Vt.IntArray([1, 2]))

if __name__ == '__main__':
    unittest.main()
