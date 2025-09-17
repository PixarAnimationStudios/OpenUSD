#!/pxrpythonsubst
#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Vt
import unittest

class TestVtArrayEdit(unittest.TestCase):

    def test_Basics(self):

        empty = Vt.IntArray()
        ident = Vt.IntArrayEdit()
        one23 = Vt.IntArray(range(1,4))

        self.assertEqual(ident.ComposeOver(ident), ident)
        self.assertEqual(ident.ComposeOver(empty),empty)
        self.assertEqual(ident.ComposeOver(Vt.IntArray(one23)), one23)

        self.assertEqual(hash(ident), hash(Vt.IntArrayEdit()))
        emptyDense = Vt.IntArrayEdit(empty)
        self.assertEqual(hash(emptyDense), hash(Vt.IntArrayEdit(Vt.IntArray())))
        self.assertEqual(hash(Vt.IntArrayEdit(one23)),
                         hash(Vt.IntArrayEdit(Vt.IntArray(range(1,4)))))

    def test_BuilderAndComposition(self):

        empty = Vt.IntArray()

        builder = Vt.IntArrayEditBuilder()
        zeroNine = builder.Prepend(0).Append(9).FinalizeAndReset()

        self.assertEqual(zeroNine.ComposeOver(empty), [0,9])
        self.assertEqual(zeroNine.ComposeOver([5]), [0,5,9])

        zero09Nine = zeroNine.ComposeOver(zeroNine)
        self.assertEqual(zero09Nine.ComposeOver(empty), [0,0,9,9])
        self.assertEqual(zero09Nine.ComposeOver([3,4,5]), [0,0,3,4,5,9,9])

        # Build an edit that writes the last element to index 2, the first
        # element to index 4, then erases the first and last element.
        mixAndTrim = (builder
                      .WriteRef(-1, 2)
                      .WriteRef(0, 4)
                      .EraseRef(-1)
                      .EraseRef(0)
                      .FinalizeAndReset())

        self.assertEqual(
            mixAndTrim.ComposeOver([0,0,3,4,5,9,9]), [0,9,4,0,9])

        # Out-of-bounds operations should be ignored.
        self.assertEqual(mixAndTrim.ComposeOver([4,5,6,7]), [5,7])

        zeroNineMixAndTrim = mixAndTrim.ComposeOver(zeroNine)
        self.assertEqual(zeroNineMixAndTrim.ComposeOver(
            [1,2,3,4,5,6,7]), [1,9,3,0,5,6,7])
        self.assertEqual(zeroNineMixAndTrim.ComposeOver([4,5]), [4,9])

        minSize10 = builder.MinSize(10).FinalizeAndReset()
        self.assertEqual(minSize10.ComposeOver([]),
                         ([0,0,0,0,0,0,0,0,0,0]))
        self.assertEqual(minSize10.ComposeOver([7]*15), [7]*15)

        minSize10Fill9 = builder.MinSize(10, fill=9).FinalizeAndReset()
        self.assertEqual(minSize10Fill9.ComposeOver([]),
                         ([9,9,9,9,9,9,9,9,9,9]))
        self.assertEqual(minSize10Fill9.ComposeOver([7]*15), [7]*15)

        maxSize15 = builder.MaxSize(15).FinalizeAndReset()
        self.assertEqual(maxSize15.ComposeOver([]), [])
        self.assertEqual(maxSize15.ComposeOver([2]*20), [2]*15)

        size10to15 = maxSize15.ComposeOver(minSize10)
        self.assertEqual(size10to15.ComposeOver([1]*7), [1,1,1,1,1,1,1,0,0,0])
        self.assertEqual(size10to15.ComposeOver([2]*20), [2]*15)
        self.assertEqual(size10to15.ComposeOver([3]*13), [3]*13)

        size7 = builder.SetSize(7).FinalizeAndReset()
        self.assertEqual(size7.ComposeOver([1]*7), [1]*7)
        self.assertEqual(size7.ComposeOver([]), [0]*7)
        self.assertEqual(size7.ComposeOver([9]*27), [9]*7)
        
        size7Fill3 = builder.SetSize(7, fill=3).FinalizeAndReset()
        self.assertEqual(size7Fill3.ComposeOver([1]*7), [1]*7)
        self.assertEqual(size7Fill3.ComposeOver([]), [3]*7)
        self.assertEqual(size7Fill3.ComposeOver([9]*27), [9]*7)

if __name__ == '__main__':
    unittest.main()
