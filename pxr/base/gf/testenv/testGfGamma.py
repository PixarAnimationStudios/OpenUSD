#!/pxrpythonsubst
#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from __future__ import division
from pxr import Gf
import unittest


class TestGfGamma(unittest.TestCase):

    def test_DifferentVecs(self):
        '''Test gamma converting vecs of different types and sizes'''
        self.assertEqual(Gf.ApplyGamma(Gf.Vec3f(1, 2, 3), 2.0),
                         Gf.Vec3f(1, 4, 9))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec3d(1, 2, 3), 2.0),
                         Gf.Vec3d(1, 4, 9))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec3h(1, 2, 3), 2.0),
                         Gf.Vec3h(1, 4, 9))

        self.assertEqual(Gf.ApplyGamma(Gf.Vec4f(1, 2, 3, 4), 2.0),
                         Gf.Vec4f(1, 4, 9, 4))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec4d(1, 2, 3, 4), 2.0),
                         Gf.Vec4d(1, 4, 9, 4))
        self.assertEqual(Gf.ApplyGamma(Gf.Vec4h(1, 2, 3, 4), 2.0),
                         Gf.Vec4h(1, 4, 9, 4))

    def test_DisplayGamma(self):
        # Check that the display gamma functions work too
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3f(.5, .5, .5), 2.2) ==
                        Gf.ConvertDisplayToLinear(Gf.Vec3f(.5, .5, .5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3d(.5, .5, .5), 2.2) ==
                        Gf.ConvertDisplayToLinear(Gf.Vec3d(.5, .5, .5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3h(.5, .5, .5), 2.2) ==
                        Gf.ConvertDisplayToLinear(Gf.Vec3h(.5, .5, .5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4f(.5, .5, .5, .8), 2.2) ==
                        Gf.ConvertDisplayToLinear(Gf.Vec4f(.5, .5, .5, .8)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4d(.5, .5, .5, .8), 2.2) ==
                        Gf.ConvertDisplayToLinear(Gf.Vec4d(.5, .5, .5, .8)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4h(.5, .5, .5, .8), 2.2) ==
                        Gf.ConvertDisplayToLinear(Gf.Vec4h(.5, .5, .5, .8)))

        self.assertTrue(Gf.ApplyGamma(Gf.Vec3f(.5, .5, .5), 1.0/2.2) ==
                        Gf.ConvertLinearToDisplay(Gf.Vec3f(.5, .5, .5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3d(.5, .5, .5), 1.0/2.2) ==
                        Gf.ConvertLinearToDisplay(Gf.Vec3d(.5, .5, .5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec3h(.5, .5, .5), 1.0/2.2) ==
                        Gf.ConvertLinearToDisplay(Gf.Vec3h(.5, .5, .5)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4f(.5, .5, .5, .8), 1.0/2.2) ==
                        Gf.ConvertLinearToDisplay(Gf.Vec4f(.5, .5, .5, .8)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4d(.5, .5, .5, .8), 1.0/2.2) ==
                        Gf.ConvertLinearToDisplay(Gf.Vec4d(.5, .5, .5, .8)))
        self.assertTrue(Gf.ApplyGamma(Gf.Vec4h(.5, .5, .5, .8), 1.0/2.2) ==
                        Gf.ConvertLinearToDisplay(Gf.Vec4h(.5, .5, .5, .8)))

if __name__ == '__main__':
    unittest.main()
