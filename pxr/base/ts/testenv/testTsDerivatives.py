#!/pxrpythonsubst

#
# Copyright 2023 Pixar
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

from pxr import Ts, Gf

import unittest


class TsTest_Derivatives(unittest.TestCase):

    ############################################################################
    # HELPERS

    def _DoTest(
            self, case, extrap,
            types, times, vals,
            expB, expL, expR,
            single = True, dual = True):
        """
        Build a spline and validate expected derivatives.

        The knots are specified by the parallel lists 'types', 'times', and
        'vals'.  Types may be "H" (held), "L" (linear), or "B" (Bezier).  Times
        and vals are floats.

        Extrapolation mode is specified by 'extrap', which is "H" (held) or "L"
        (linear).

        Expected derivative values are specified by 'expB', 'expL', and 'expR'.
        These respectively are between knots, on the left sides of knots, and on
        the right sides of knots.  'expB' is one element longer than the knot
        lists; it gives expected values before the first knot, between each pair
        of knots, and after the last.  'expL' and 'expR' correspond exactly to
        the knot lists.

        Values in 'expB', 'expL', and 'expR' may be floats, or they may be
        strings that denote special values; see 'expMap' in the implementation.

        Single-valued and dual-valued knots may be tested.  Specify which in
        'single' and 'dual', which are both true by default.

        Float-valued splines are always tested.  If there are no Bezier knots in
        the 'types' list, vector-valued splines will also be tested.  Vector
        values are just Gf.Vec2d with both components set to the float values
        specified in 'vals'.
        """
        # Constants we use in our splines.
        bezTans = {"lens": [.5, .5], "slopes": [.3, .3]}
        garbageTans = {"lens": [2.7, 0.8], "slopes": [0.4, -4.0]}

        # Meanings of convenience abbreviations.
        extrapMap = {"H": Ts.ExtrapolationHeld, "L": Ts.ExtrapolationLinear}
        typeMap = {"H": Ts.KnotHeld, "L": Ts.KnotLinear, "B": Ts.KnotBezier}

        # Special expected results that are specified by name.
        # "b" is the Bezier tangent slope we use.
        # "xy" is the midpoint tangent in a segment bracketed by types x and y.
        # The midpoint tangents are empirical, not mathematically derived.
        expMap = {"b": .3, "bb": 1.70, "bl": 1.35, "bh": 1.68, "lb": 1.19}

        # Set up spline.
        spline = Ts.Spline()
        extrapMode = extrapMap[extrap]
        spline.extrapolation = (extrapMode, extrapMode)

        # Add knots as specified.
        # Give non-Bezier knots garbage tangents, to ensure they have no effect.
        # Remember whether there are any Beziers; if there are, no vectors.
        doVectors = True
        for i in range(len(types)):
            knot = Ts.KeyFrame(times[i], float(vals[i]), typeMap[types[i]])
            tans = bezTans if types[i] == "B" else garbageTans
            knot.leftLen = tans["lens"][0]
            knot.leftSlope = tans["slopes"][0]
            knot.rightLen = tans["lens"][1]
            knot.rightSlope = tans["slopes"][1]
            spline.SetKeyFrame(knot)
            if types[i] == "B":
                doVectors = False

        # Build the list of between-knot times.
        # This also includes one pre-extrapolation and one post-extrapolation.
        betTimes = []
        betTimes.append(times[0] - 1)
        for i in range(len(times) - 1):
            betTimes.append((times[i] + times[i + 1]) / 2)
        betTimes.append(times[-1] + 1)

        # Translate any symbolic expected results into numeric values.
        for expList in expB, expL, expR:
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

        # Test with scalar values.
        self._DoTestWithValueTypeVariation(
            case,
            spline, times, betTimes, expB, expL, expR,
            single, dual)

        # Test with vector values if the knot types permit.
        if doVectors:
            # Build vector-valued spline.
            vecSpline = Ts.Spline()
            vecSpline.extrapolation = (extrapMode, extrapMode)
            for i in range(len(types)):
                floatVal = float(vals[i])
                value = Gf.Vec2d(floatVal, floatVal)
                knot = Ts.KeyFrame(times[i], value, typeMap[types[i]])
                vecSpline.SetKeyFrame(knot)

            # Build vector-valued expected values.
            vecExpB = [Gf.Vec2d(v, v) for v in expB]
            vecExpL = [Gf.Vec2d(v, v) for v in expL]
            vecExpR = [Gf.Vec2d(v, v) for v in expR]

            # Test with vector values.
            self._DoTestWithValueTypeVariation(
                f"{case} (vectors)",
                vecSpline, times, betTimes, vecExpB, vecExpL, vecExpR,
                single, dual)

    def _DoTestWithValueTypeVariation(
            self, case, spline, times, betTimes, expB, expL, expR,
            single, dual):

        if single:
            # Test with the original spline.
            self._DoTestWithDualityVariation(
                case,
                spline, times, betTimes, expB, expL, expR)

        if dual:
            # Modify the spline to have dual-valued knots.
            # Give each segment an offset of 1 unit from previous.
            # This shouldn't affect derivatives at all.
            for i in range(len(times)):
                knot = spline[times[i]]
                knot.isDualValued = True
                values = list(knot.value)
                if type(values[0]) is float:
                    values[0] += i
                    values[1] += i + 1
                else:
                    values[0][0] += i
                    values[0][1] += i
                    values[1][0] += i + 1
                    values[1][1] += i + 1
                knot.value = values
                spline.SetKeyFrame(knot)

            # Test with the dual-valued spline.
            self._DoTestWithDualityVariation(
                f"{case} (dual-valued)",
                spline, times, betTimes, expB, expL, expR)

    def _DoTestWithDualityVariation(
            self, case, spline, times, betTimes, expB, expL, expR):

        print()
        print(f"{case}:")

        # Compare results between knots, and at left and right sides of knots.
        errors = 0
        errors += self._DoTestWithPositionVariation(
            "Betweens", spline, betTimes, expB, Ts.Right)
        errors += self._DoTestWithPositionVariation(
            "Lefts", spline, times, expL, Ts.Left)
        errors += self._DoTestWithPositionVariation(
            "Rights", spline, times, expR, Ts.Right)

        self.assertEqual(errors, 0)

    def _DoTestWithPositionVariation(
            self, title, spline, times, expList, side):

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
            deriv = spline.EvalDerivative(time, side)

            # Evaluate nearby predicted and actual values.
            if side == Ts.Left:
                valueAtTime = spline.Eval(time, Ts.Left)
                valueNearby = spline.Eval(time - SAMPLE_DISTANCE)
                predicted = valueAtTime - deriv * SAMPLE_DISTANCE
            else:
                valueAtTime = spline.Eval(time, Ts.Right)
                valueNearby = spline.Eval(time + SAMPLE_DISTANCE)
                predicted = valueAtTime + deriv * SAMPLE_DISTANCE

            # Compute errors from expected values.
            if type(deriv) is float:
                diff = deriv - expected
                sampleDiff = predicted - valueNearby
            else:
                diff = max(
                    deriv[0] - expected[0],
                    deriv[1] - expected[1])
                sampleDiff = max(
                    predicted[0] - valueNearby[0],
                    predicted[1] - valueNearby[1])

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
        Exercise every combination of successive knot types.
        """
        # Fit-in-80-column madness
        nbl = "-bl"
        bb = "bb"
        nbh = "-bh"
        lb = "lb"

        # H = held, L = linear, B = bezier
        # expB = expected between; expL = expected left; expR = expected right
        extrap = "H"
        types = [ "H", "H", "L", "L", "B", "B", "H", "B", "L", "B", "L", "H" ]
        times = [  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11  ]
        vals =  [  0,   1,   0,   1,   0,   1,   0,   1,   0,   1,   0,   1  ]
        expB =  [0,  0,   0,   1,  nbl,  bb, nbh,  0,  nbl,  lb, nbl,  1,   0]
        expL =  [  0,   0,   0,   1,  .3,  .3,   0,   0,  -1,  .3,  -1,   1  ]
        expR =  [  0,   0,   1,  -1,  .3,  .3,   0,  .3,   1,  .3,   1,   0  ]

        self._DoTest("Main", extrap, types, times, vals, expB, expL, expR)

    def test_Vectors(self):
        """
        Exercise every combination of successive knot types for vector values.
        """
        extrap = "H"
        types = [ "H", "H", "L", "L", "H" ]
        times = [  0,   1,   2,   3,   4  ]
        vals =  [  0,   1,   0,   1,   0  ]
        expB =  [0,  0,   0,   1,   -1,  0]
        expL =  [  0,   0,   0,   1,  -1  ]
        expR =  [  0,   0,   1,  -1,   0  ]

        self._DoTest("Vectors", extrap, types, times, vals, expB, expL, expR)

    def test_Empty(self):
        """
        Verify that an empty spline has a nonexistent derivative.
        """
        spline = Ts.Spline()
        self.assertEqual(spline.EvalDerivative(0), None)

    def test_StringValued(self):
        """
        Verify that a string-valued spline has no meaningful derivatives.
        """
        spline = Ts.Spline()
        spline.SetKeyFrame(Ts.KeyFrame(0, "welcome"))
        spline.SetKeyFrame(Ts.KeyFrame(1, "dandelions"))
        self.assertEqual(spline.EvalDerivative(-1), "")
        self.assertEqual(spline.EvalDerivative(0, Ts.Left), "")
        self.assertEqual(spline.EvalDerivative(0, Ts.Right), "")
        self.assertEqual(spline.EvalDerivative(.5), "")
        self.assertEqual(spline.EvalDerivative(1, Ts.Left), "")
        self.assertEqual(spline.EvalDerivative(1, Ts.Right), "")
        self.assertEqual(spline.EvalDerivative(2), "")

    def test_QuatValued(self):
        """
        Verify that a quaternion-valued spline has no meaningful derivatives.
        """
        spline = Ts.Spline()
        spline.SetKeyFrame(Ts.KeyFrame(0, Gf.Quatd(1, 2, 3, 4)))
        spline.SetKeyFrame(Ts.KeyFrame(1, Gf.Quatd(5, 6, 7, 8)))
        self.assertEqual(spline.EvalDerivative(-1), Gf.Quatd())
        self.assertEqual(spline.EvalDerivative(0, Ts.Left), Gf.Quatd())
        self.assertEqual(spline.EvalDerivative(0, Ts.Right), Gf.Quatd())
        self.assertEqual(spline.EvalDerivative(.5), Gf.Quatd())
        self.assertEqual(spline.EvalDerivative(1, Ts.Left), Gf.Quatd())
        self.assertEqual(spline.EvalDerivative(1, Ts.Right), Gf.Quatd())
        self.assertEqual(spline.EvalDerivative(2), Gf.Quatd())

    def test_Single(self):
        """
        Test derivatives of single-knot splines.  These splines are flat, except
        in the case of a Bezier knot and linear extrapolation.
        """
        times = [1]
        vals = [5]
        expB = [0, 0]
        expL = [0]
        expR = [0]

        extrap = "H"
        types = ["H"]
        self._DoTest("Single, held knot, held extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "L"
        types = ["H"]
        self._DoTest("Single, held knot, linear extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "H"
        types = ["L"]
        self._DoTest("Single, linear knot, held extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "L"
        types = ["L"]
        self._DoTest("Single, linear knot, linear extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "H"
        types = ["B"]
        self._DoTest("Single, Bezier knot, held extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "L"
        types = ["B"]
        expB = ["b", "b"]
        expL = ["b"]
        expR = ["b"]
        self._DoTest("Single, Bezier knot, linear extrap",
                     extrap, types, times, vals, expB, expL, expR)

    def test_Extrapolation(self):
        """
        Test derivatives in exrapolation regions.  This includes regions outside
        all knots, and also on the outward-facing sides of edge knots.  Test all
        combinations of knot type and extrapolation mode.
        """
        times = [0, 1]
        vals = [0, 1]

        extrap = "H"
        types = ["H", "H"]
        expB = [0, 0, 0]
        expL = [0, 0]
        expR = [0, 0]
        self._DoTest("Extrapolation, held knots, held extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "L"
        types = ["H", "H"]
        expB = [0, 0, 0]
        expL = [0, 0]
        expR = [0, 0]
        self._DoTest("Extrapolation, held knots, linear extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "H"
        types = ["L", "L"]
        expB = [0, 1, 0]
        expL = [0, 1]
        expR = [1, 0]
        self._DoTest("Extrapolation, linear knots, held extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "L"
        types = ["L", "L"]
        expB = [1, 1, 1]
        expL = [1, 1]
        expR = [1, 1]
        self._DoTest("Extrapolation, linear knots, linear extrap",
                     extrap, types, times, vals, expB, expL, expR,
                     single = True, dual = False)

        # Linear edge knots, with linear extrapolation, behave differently when
        # there are dual values.  The determination of an extrapolation slope
        # from the line between the last two knots is abandoned because the dual
        # values make it ambiguous.  Held extrapolation is used instead.
        extrap = "L"
        types = ["L", "L"]
        expB = [0, 1, 0]
        expL = [0, 1]
        expR = [1, 0]
        self._DoTest("Extrapolation, linear knots, linear extrap",
                     extrap, types, times, vals, expB, expL, expR,
                     single = False, dual = True)

        extrap = "H"
        types = ["B", "B"]
        expB = [0, "bb", 0]
        expL = [0, "b"]
        expR = ["b", 0]
        self._DoTest("Extrapolation, Bezier knots, held extrap",
                     extrap, types, times, vals, expB, expL, expR)

        extrap = "L"
        types = ["B", "B"]
        expB = ["b", "bb", "b"]
        expL = ["b", "b"]
        expR = ["b", "b"]
        self._DoTest("Extrapolation, Bezier knots, linear extrap",
                     extrap, types, times, vals, expB, expL, expR)


if __name__ == "__main__":

    # 'buffer' means that all stdout will be captured and swallowed, unless
    # there is an error, in which case the stdout of the erroring case will be
    # printed on stderr along with the test results.  Suppressing the output of
    # passing cases makes it easier to find the output of failing ones.
    unittest.main(testRunner = unittest.TextTestRunner(buffer = True))
