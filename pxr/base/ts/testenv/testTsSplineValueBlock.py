#!/pxrpythonsubst

#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts

import unittest


class TsTest_SplineValueBlocks(unittest.TestCase):
    def CheckEvalAllMethods(self, spline, time,
                            preValue, value,
                            preDeriv, deriv,
                            preHeld, held):
        """
        Evaluate the spline at time using EvalPreValue, Eval, EvalPreDerivative,
        EvalDerivative, EvalPreValueHeld, and EvalHeld. Check all of the results
        against the expected results and assert if different.
        """
        if preValue is None:
            self.assertIsNone(spline.EvalPreValue(time))
        else:
            self.assertEqual(spline.EvalPreValue(time), preValue)

        if value is None:
            self.assertIsNone(spline.Eval(time))
        else:
            self.assertEqual(spline.Eval(time), value)

        if preDeriv is None:
            self.assertIsNone(spline.EvalPreDerivative(time))
        else:
            self.assertEqual(spline.EvalPreDerivative(time), preDeriv)

        if deriv is None:
            self.assertIsNone(spline.EvalDerivative(time))
        else:
            self.assertEqual(spline.EvalDerivative(time), deriv)

        if preHeld is None:
            self.assertIsNone(spline.EvalPreValueHeld(time))
        else:
            self.assertEqual(spline.EvalPreValueHeld(time), preHeld)

        if held is None:
            self.assertIsNone(spline.EvalHeld(time))
        else:
            self.assertEqual(spline.EvalHeld(time), held)


    def test_SplineValueBlocks(self):
        """
        Verify that the spline correctly handles value blocked interpolation
        and extrapolation.
        """
        # Build a spline that looks like this:
        # [-inf .. 1.0): value block
        # [ 1.0 .. 2.0): linear from 1.0 to 2.0
        # [ 2.0 .. 3.0): value block
        # [ 3.0 .. 4.0): held from 3.0 to 4.0
        # [ 4.0 .. inf): value block
        spline = Ts.Spline()
        knots = [
            Ts.Knot(time=1.0, value=1.0, nextInterp=Ts.InterpLinear),
            Ts.Knot(time=2.0, value=2.0, nextInterp=Ts.InterpValueBlock),
            Ts.Knot(time=3.0, value=3.0, nextInterp=Ts.InterpHeld),
            Ts.Knot(time=4.0, value=4.0, nextInterp=Ts.InterpHeld),
        ]

        for knot in knots:
            spline.SetKnot(knot)

        extrapolation = Ts.Extrapolation(Ts.ExtrapValueBlock)
        spline.SetPreExtrapolation(extrapolation)
        spline.SetPostExtrapolation(extrapolation)

        # Automatically pass the spline and shorten the line length
        check = lambda t, pv, v, pd, d, ph, h: self.CheckEvalAllMethods(
            spline, t, pv, v, pd, d, ph, h)

        #    time  pVal   Val   pDv   Dv   pHld   Hld
        check(0.0, None, None, None, None, None, None)  # [-inf .. 1.0): block
        check(1.0, None,  1.0, None,  1.0, None,  1.0)  # [ 1.0 .. 2.0): linear
        check(1.5,  1.5,  1.5,  1.0,  1.0,  1.0,  1.0)
        check(2.0,  2.0, None,  1.0, None,  1.0, None)  # [ 2.0 .. 3.0): block
        check(2.5, None, None, None, None, None, None)
        check(3.0, None,  3.0, None,  0.0, None,  3.0)  # [ 3.0 .. 4.0): held
        check(3.5,  3.0,  3.0,  0.0,  0.0,  3.0,  3.0)
        check(4.0,  3.0, None,  0.0, None,  3.0, None)  # [ 4.0 .. inf): block
        check(5.0, None, None, None, None, None, None)

    def test_ExtrapolationSlope(self):
        """
        Verify that the extrapolated slope for linear extrapolation is correctly
        zero when the segments at the end are value blocked.
        """
        # Build a spline that looks like this:
        # [-inf .. 1.0): linear extrapolation (with a slope set)
        # [ 1.0 .. 2.0): value block
        # [ 2.0 .. 3.0): linear interpolation
        # [ 3.0 .. 4.0): value block
        # [ 4.0 .. inf): linear extrapolation (with a slope set)
        spline = Ts.Spline()
        knots = [
            Ts.Knot(time=1.0, value=1.0, nextInterp=Ts.InterpValueBlock,
                    postTanSlope=-1.0),
            Ts.Knot(time=2.0, value=2.0, nextInterp=Ts.InterpLinear),
            Ts.Knot(time=3.0, value=3.0, nextInterp=Ts.InterpValueBlock),
            Ts.Knot(time=4.0, value=4.0, nextInterp=Ts.InterpHeld,
                    preTanSlope=-1.0),
        ]

        for knot in knots:
            spline.SetKnot(knot)

        extrapolation = Ts.Extrapolation(Ts.ExtrapLinear)
        spline.SetPreExtrapolation(extrapolation)
        spline.SetPostExtrapolation(extrapolation)

        # Automatically pass the spline and shorten the line length
        check = lambda t, pv, v, pd, d, ph, h: self.CheckEvalAllMethods(
            spline, t, pv, v, pd, d, ph, h)

        #    time  pVal   Val   pDv   Dv   pHld   Hld
        check(0.0,  1.0,  1.0,  0.0,  0.0,  1.0,  1.0)  # [-inf .. 1.0): linear
        check(1.0,  1.0, None,  0.0, None,  1.0, None)  # [ 1.0 .. 2.0): block
        check(1.5, None, None, None, None, None, None)
        check(2.0, None,  2.0, None,  1.0, None,  2.0)  # [ 2.0 .. 3.0): linear
        check(2.5,  2.5,  2.5,  1.0,  1.0,  2.0,  2.0)
        check(3.0,  3.0, None,  1.0, None,  2.0, None)  # [ 3.0 .. 4.0): block
        check(3.5, None, None, None, None, None, None)
        check(4.0, None,  4.0, None,  0.0, None,  4.0)  # [ 4.0 .. inf): linear
        check(5.0,  4.0,  4.0,  0.0,  0.0,  4.0,  4.0)



if __name__ == '__main__':
    unittest.main()
