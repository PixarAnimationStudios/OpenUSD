#!/pxrpythonsubst

#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts

import unittest


class TsTest_Derivatives(unittest.TestCase):

    ############################################################################
    # HELPERS

    def _DoTest(
            self, case, extrap,
            interps, times, vals,
            expB, expP, expK,
            single = True, dual = True,
            graph = False, graphParams = {}):
        """
        Build a spline and validate expected derivatives.

        The knots are specified by the parallel lists 'types', 'times', and
        'vals'.  Interps specify the interpolation mode of the following
        segment, and may be "H" (held), "L" (linear), or "B" (curve).  Times
        and vals are floats.

        Extrapolation mode is specified by 'extrap', which is "H" (held), "L"
        (linear), "S" (sloped), "LR" (loop repeat), "LX" (loop reset), or "LO"
        (loop oscillate).

        Expected derivative values are specified by 'expB', 'expP', and 'expK'.
        These respectively are between knots, pre-values at knots, and ordinary
        values at knots.  'expB' is one element longer than the knot
        lists; it gives expected values before the first knot, between each pair
        of knots, and after the last.  'expP' and 'expK' correspond exactly to
        the knot lists.

        Values in 'expB', 'expP', and 'expK' may be floats, or they may be
        strings that denote special values; see 'expMap' in the implementation.

        Single-valued and dual-valued knots may be tested.  Specify which in
        'single' and 'dual', which are both true by default.

        If 'graph' is true, a graph of the spline will be written to a file to
        aid debugging.  If 'graphParams' is provided, the contents will be used
        as kwargs to TsTest_Grapher.
        """
        # Constants we use in our splines.
        extrapSlope = 1.2
        bezTans = {"lens": [.5, .5], "slopes": [.3, .3]}
        garbageTans = {"lens": [2.7, 0.8], "slopes": [0.4, -4.0]}

        # Meanings of convenience abbreviations.
        extrapMap = {"H": Ts.ExtrapHeld, "L": Ts.ExtrapLinear,
                     "S": Ts.ExtrapSloped, "LR": Ts.ExtrapLoopRepeat,
                     "LX": Ts.ExtrapLoopReset, "LO": Ts.ExtrapLoopOscillate}
        interpMap = {"H": Ts.InterpHeld, "L": Ts.InterpLinear,
                     "C": Ts.InterpCurve}

        # Special expected results that are specified by name.
        # "s" is the knot tangent slope we use.
        # "bb" is the slope centered between two of our Bezier knots.
        # "e" is the extrapolation slope we use.
        expMap = {"s": .3, "bb": 1.7, "e": 1.2}

        # Set up spline.
        spline = Ts.Spline()
        extrapolation = Ts.Extrapolation(extrapMap[extrap])
        extrapolation.slope = extrapSlope
        spline.SetPreExtrapolation(extrapolation)
        spline.SetPostExtrapolation(extrapolation)

        # Add knots as specified.
        # Give non-Bezier knots garbage tangents, to ensure they have no effect.
        for i in range(len(interps)):
            tans = bezTans if interps[i] == "C" else garbageTans
            knot = Ts.Knot(
                time = times[i], value = float(vals[i]),
                nextInterp = interpMap[interps[i]],
                preTanWidth = tans["lens"][0],
                preTanSlope = tans["slopes"][0],
                postTanWidth = tans["lens"][1],
                postTanSlope = tans["slopes"][1])
            spline.SetKnot(knot)

        # If requested, create a graph of the spline to aid debugging.
        grapher = None
        if graph and Ts.TsTest_Grapher.Init():
            evaluator = Ts.TsTest_TsEvaluator()
            data = evaluator.SplineToSplineData(spline)
            sampleTimes = Ts.TsTest_SampleTimes(data)
            sampleTimes.AddStandardTimes()
            samples = evaluator.Eval(data, sampleTimes)
            grapher = Ts.TsTest_Grapher(case, **graphParams)
            grapher.AddSpline(case, data, samples)

        # Build the list of between-knot times.
        # This also includes one pre-extrapolation and one post-extrapolation.
        betTimes = []
        betTimes.append(times[0] - 1)
        for i in range(len(times) - 1):
            betTimes.append((times[i] + times[i + 1]) / 2)
        betTimes.append(times[-1] + 1)

        # Translate any symbolic expected results into numeric values.
        for expList in expB, expP, expK:
            for i in range(len(expList)):
                expVal = expList[i]
                if type(expVal) == str:
                    if expVal.startswith("-"):
                        negate = True
                        expVal = expVal[1:]
                    else:
                        negate = False
                    expVal = expMap[expVal]
                    if negate:
                        expVal *= -1
                    expList[i] = expVal

        errors = 0

        if single:
            # Test with the original spline.
            errors += self._DoTestWithDualityVariation(
                case,
                spline, times, betTimes, expB, expP, expK)

        if dual:
            # Modify the spline to have dual-valued knots.
            # Give each segment an offset of 1 unit from previous.
            # This shouldn't affect derivatives at all.
            for i in range(len(times)):
                knot = spline.GetKnots()[times[i]]
                baseValue = knot.GetValue() + i
                knot.SetPreValue(baseValue)
                knot.SetValue(baseValue + 1)
                spline.SetKnot(knot)

            if grapher:
                data = evaluator.SplineToSplineData(spline)
                sampleTimes = Ts.TsTest_SampleTimes(data)
                sampleTimes.AddStandardTimes()
                samples = evaluator.Eval(data, sampleTimes)
                grapher.AddSpline(f"{case} (dual)", data, samples)

            # Test with the dual-valued spline.
            errors += self._DoTestWithDualityVariation(
                f"{case} (dual-valued)",
                spline, times, betTimes, expB, expP, expK)

        if grapher:
            grapher.Write(f"{case}.png")

        self.assertEqual(errors, 0)

    def _DoTestWithDualityVariation(
            self, case, spline, times, betTimes, expB, expP, expK):

        print()
        print(f"{case}:")

        # Compare results between knots, pre-values at knots, and at knots.
        errors = 0
        errors += self._DoTestWithPositionVariation(
            "Betweens", spline, betTimes, expB, pre = False)
        errors += self._DoTestWithPositionVariation(
            "Pre-Knots", spline, times, expP, pre = True)
        errors += self._DoTestWithPositionVariation(
            "Knots", spline, times, expK, pre = False)

        return errors

    def _DoTestWithPositionVariation(
            self, title, spline, times, expList, pre):

        DERIV_TOLERANCE = 1e-2
        SAMPLE_DISTANCE = 1e-3
        SAMPLE_TOLERANCE = 1e-5

        errors = 0

        print()
        print(f"  {title}:")

        # Verify each derivative result matches the expected result.  Also
        # verify the derivative is mathematically correct, predicting the value
        # a short distance away.
        for i in range(len(times)):

            # Look up time and expected derivative value.
            time = times[i]
            expected = expList[i]

            # Evaluate derivative.
            if pre:
                deriv = spline.EvalPreDerivative(time)
            else:
                deriv = spline.EvalDerivative(time)

            # Evaluate nearby predicted and actual values.
            if pre:
                valueAtTime = spline.EvalPreValue(time)
                valueNearby = spline.Eval(time - SAMPLE_DISTANCE)
                predicted = valueAtTime - deriv * SAMPLE_DISTANCE
            else:
                valueAtTime = spline.Eval(time)
                valueNearby = spline.Eval(time + SAMPLE_DISTANCE)
                predicted = valueAtTime + deriv * SAMPLE_DISTANCE

            # Compute errors from expected values.
            diff = deriv - expected
            sampleDiff = predicted - valueNearby

            # Check error tolerances.
            if abs(diff) < DERIV_TOLERANCE \
                   and abs(sampleDiff) < SAMPLE_TOLERANCE:
                status = "PASS"
            else:
                status = "**FAIL"
                errors += 1

            # Print results.
            print(f"    {time}: {status}: "
                  f"expected {expected}, actual {deriv}, diff {diff}, "
                  f"predicted {predicted}, actual {valueNearby}, "
                  f"diff {sampleDiff}")

        return errors

    ############################################################################
    # TEST ROUTINES

    def test_Main(self):
        """
        Exercise every segment type.
        """
        # H = held, L = linear, C = curve (Bezier)
        # expB = expected between; expP = expected pre; expK = expected at knot
        extrap = "H"
        interps = [ "H", "L", "C", "C" ]
        times =   [  0,   1,   2,   3  ]
        vals =    [  5,   6,   7,   8  ]
        expB =    [0,  0,   1,  "bb", 0]
        expP =    [  0,   0,   1,  "s" ]
        expK =    [  0,   1,  "s",  0  ]

        self._DoTest(
            "Main", extrap, interps, times, vals, expB, expP, expK,
            graph = True)

    def test_Empty(self):
        """
        Verify that an empty spline has a nonexistent derivative.
        """
        spline = Ts.Spline()
        self.assertEqual(spline.EvalDerivative(0), None)

    def test_Single(self):
        """
        Test derivatives of single-knot splines.  These splines are flat, except
        when there is sloped extrapolation.
        """
        interps = ["C"]
        times = [1]
        vals = [5]
        expB = [0, 0]
        expP = [0]
        expK = [0]

        extrap = "H"
        self._DoTest("Single, held extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "L"
        self._DoTest("Single, linear extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "LR"
        self._DoTest("Single, loop-repeat extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "LX"
        self._DoTest("Single, loop-reset extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "LO"
        self._DoTest("Single, loop-oscillate extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "S"
        expB = ["e", "e"]
        expP = ["e"]
        expK = ["e"]
        self._DoTest("Single, sloped extrap",
                     extrap, interps, times, vals, expB, expP, expK)

    def test_Extrapolation(self):
        """
        Test derivatives in exrapolation regions.  This includes regions outside
        all knots, and also on the outward-facing sides of edge knots.  Test all
        extrapolation modes, and special cases for linear extrapolation
        (final segment interpolation mode, dual-valued knots).
        """
        times = [0, 1]
        vals = [0, 1]

        extrap = "H"
        interps = ["L", "L"]
        expB = [0, 1, 0]
        expP = [0, 1]
        expK = [1, 0]
        self._DoTest("Extrapolation, linear interp, held extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "L"
        interps = ["H", "H"]
        expB = [0, 0, 0]
        expP = [0, 0]
        expK = [0, 0]
        self._DoTest("Extrapolation, held interp, linear extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "L"
        interps = ["L", "L"]
        expB = [1, 1, 1]
        expP = [1, 1]
        expK = [1, 1]
        self._DoTest("Extrapolation, linear interp, linear extrap",
                     extrap, interps, times, vals, expB, expP, expK,
                     single = True, dual = False)

        extrap = "L"
        interps = ["C", "C"]
        expB = ["s", "bb", "s"]
        expP = ["s", "s"]
        expK = ["s", "s"]
        self._DoTest("Extrapolation, Bezier interp, linear extrap",
                     extrap, interps, times, vals, expB, expP, expK,
                     single = True, dual = False)

        # Linear extrapolation behaves differently when there are dual values at
        # first/last knots.  The determination of an extrapolation slope is
        # abandoned because the dual values make it ambiguous.  Held
        # extrapolation is used instead.
        extrap = "L"
        interps = ["L", "L"]
        expB = [0, 1, 0]
        expP = [0, 1]
        expK = [1, 0]
        self._DoTest("Extrapolation, linear interp, linear extrap",
                     extrap, interps, times, vals, expB, expP, expK,
                     single = False, dual = True)

        extrap = "S"
        interps = ["H", "H"]
        expB = ["e", 0, "e"]
        expP = ["e", 0]
        expK = [0, "e"]
        self._DoTest("Extrapolation, held interp, sloped extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "LR"
        interps = ["C", "C"]
        expB = ["s", "bb", "s"]
        expP = ["s", "s"]
        expK = ["s", "s"]
        self._DoTest("Extrapolation, Bezier interp, loop-repeat extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "LX"
        interps = ["C", "C"]
        expB = ["s", "bb", "s"]
        expP = ["s", "s"]
        expK = ["s", "s"]
        self._DoTest("Extrapolation, Bezier interp, loop-reset extrap",
                     extrap, interps, times, vals, expB, expP, expK)

        extrap = "LO"
        interps = ["C", "C"]
        expB = ["-s", "bb", "s"]
        expP = ["-s", "s"]
        expK = ["s", "-s"]
        self._DoTest("Extrapolation, Bezier interp, loop-oscillate extrap",
                     extrap, interps, times, vals, expB, expP, expK)


if __name__ == "__main__":

    # 'buffer' means that all stdout will be captured and swallowed, unless
    # there is an error, in which case the stdout of the erroring case will be
    # printed on stderr along with the test results.  Suppressing the output of
    # passing cases makes it easier to find the output of failing ones.
    unittest.main(testRunner = unittest.TextTestRunner(buffer = True))
