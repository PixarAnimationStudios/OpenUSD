#!/pxrpythonsubst

#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts

import unittest

class TsTest_TangentAlgorithms(unittest.TestCase):
    def _AutoEaseSlope(self, pt, pv, t, v, nt, nv):
        """
        Given previous time and value, current time, and value, and next time and
        value, calculate the slope the auto ease algorithm should return
        """
        pSlope = (v - pv) / (t - pt)
        nSlope = (nv - v) / (nt - t)
        f = (t - pt) / (nt - pt)
        u = f - 0.5
        g = 0.5 + u * (0.5 + 2 * u * u)

        slope = (1 - g) * pSlope + g * nSlope
        if (pSlope * nSlope) < 0:
            slope = 0.0
        elif nSlope > 0:
            slope = min(slope, 3*nSlope, 3*pSlope)
        else:
            slope = max(slope, 3*nSlope, 3*pSlope)

        return slope

    def _CheckTimeTangents(self, spline, time,
                           preSlope, preWidth,
                           postSlope, postWidth):
        """Verify that the knot at time time has the expected tangent values."""
        knot = spline.GetKnot(time)

        self.assertIsNot(knot, None, f"Failed to get knot at time {time}.");

        return self._CheckKnotTangents(knot,
                                       preSlope, preWidth,
                                       postSlope, postWidth)

    def _CheckKnotTangents(self, knot,
                           preSlope, preWidth,
                           postSlope, postWidth):
        """Verify that the knot has the expected tangent values."""
        self.assertAlmostEqual(
            knot.GetPreTanSlope(), preSlope,
            msg=(f"Unexpected preTanSlope: got {knot.GetPreTanSlope()},"
                 f" expected {preSlope}."))
        self.assertEqual(
            knot.GetPreTanWidth(), preWidth,
            msg=(f"Unexpected preTanWidth: got {knot.GetPreTanWidth()},"
                 f" expected {preWidth}."))

        self.assertEqual(
            knot.GetPostTanSlope(), postSlope,
            msg=(f"Unexpected postTanSlope: got {knot.GetPostTanSlope()},"
                 f" expected {postSlope}."))
        self.assertEqual(
            knot.GetPostTanWidth(), postWidth,
            msg=(f"Unexpected postTanWidth: got {knot.GetPostTanWidth()},"
                 f" expected {postWidth}."))

    def test_TsTangentAlgorithmNone(self):
        """
        Test that the "None" algorithm does not modify the tangents for Bezier
        splines and adjusts the tangent widths of Hermite splines.

        Regardless of the tangent algorithm, anti-regression behavior still
        applies.
        """
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionNone):
            spline = Ts.Spline()
            spline.SetKnot(Ts.Knot(time=0.0, value=0.0,
                                   preTanSlope=0.0, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmNone,
                                   postTanSlope=0.0, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmNone,
                                   nextInterp=Ts.InterpCurve))

            # Verify that nothing has happened yet as there's only 1 knot in the
            # spline.
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 3.0)

            # Add a new knot.
            spline.SetKnot(Ts.Knot(time=6.0, value=3.0,
                                   preTanSlope=0.0, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmNone,
                                   postTanSlope=0.0, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmNone,
                                   nextInterp=Ts.InterpCurve))

            # Verify that still nothing has happened.
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 3.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 0.5, 0.0, 3.0)

            # Add a knot in the middle
            spline.SetKnot(Ts.Knot(time=3.0, value=2.0,
                                   preTanSlope=0.0, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmNone,
                                   postTanSlope=0.0, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmNone,
                                   nextInterp=Ts.InterpCurve))

            # Verify that still nothing has happened.
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 3.0)
            self._CheckTimeTangents(spline, 3.0, 0.0, 0.5, 0.0, 3.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 0.5, 0.0, 3.0)

            # Changing a Bezier spline to Hermite should recalculate the tangent
            # widths.
            spline.SetCurveType(Ts.CurveTypeHermite)
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 1.0)
            self._CheckTimeTangents(spline, 3.0, 0.0, 1.0, 0.0, 1.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 1.0, 0.0, 3.0)

            # Evaluate and save some values from the Hermite curve
            hermiteValues = [spline.Eval(t / 2.0)
                             for t in range(13)]

            # Changing back to Bezier should not change anything.
            spline.SetCurveType(Ts.CurveTypeHermite)
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 1.0)
            self._CheckTimeTangents(spline, 3.0, 0.0, 1.0, 0.0, 1.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 1.0, 0.0, 3.0)

            # The values should not change either.
            bezierValues = [spline.Eval(t / 2.0)
                            for t in range(13)]

            for h, b in zip(hermiteValues, bezierValues):
                self.assertAlmostEqual(h, b,
                                       f"Difference found between Hermite and"
                                       f" Bezier curves: {h} vs {b}.")

        # Now do it all again with a Hermite spline
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionNone):
            spline = Ts.Spline()
            spline.SetCurveType(Ts.CurveTypeHermite)

            spline.SetKnot(Ts.Knot(time=0.0, value=0.0,
                                   preTanSlope=0.0, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmNone,
                                   postTanSlope=0.0, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmNone,
                                   nextInterp=Ts.InterpCurve))

            # Verify that nothing has happened yet as there's only 1 knot in the
            # spline.
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 3.0)

            # Add a new knot.
            spline.SetKnot(Ts.Knot(time=6.0, value=3.0,
                                   preTanSlope=0.0, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmNone,
                                   postTanSlope=0.0, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmNone,
                                   nextInterp=Ts.InterpCurve))

            # Verify that appropriate tangent widths have changed, but only
            # interior to the knots.
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 2.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 2.0, 0.0, 3.0)

            # Add a knot in the middle
            spline.SetKnot(Ts.Knot(time=3.0, value=2.0,
                                   preTanSlope=0.0, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmNone,
                                   postTanSlope=0.0, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmNone,
                                   nextInterp=Ts.InterpCurve))

            # Verify that the tangents have updated again.
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 1.0)
            self._CheckTimeTangents(spline, 3.0, 0.0, 1.0, 0.0, 1.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 1.0, 0.0, 3.0)

            # Evaluate and save some values from the Hermite curve
            hermiteValues = [spline.Eval(t / 2.0)
                             for t in range(13)]

            # Changing to Bezier should not change anything.
            spline.SetCurveType(Ts.CurveTypeHermite)
            self._CheckTimeTangents(spline, 0.0, 0.0, 0.5, 0.0, 1.0)
            self._CheckTimeTangents(spline, 3.0, 0.0, 1.0, 0.0, 1.0)
            self._CheckTimeTangents(spline, 6.0, 0.0, 1.0, 0.0, 3.0)

            # The values should not change either.
            bezierValues = [spline.Eval(t / 2.0)
                            for t in range(13)]

            for h, b in zip(hermiteValues, bezierValues):
                self.assertAlmostEqual(h, b,
                                       f"Difference found between Hermite and"
                                       f" Bezier curves: {h} vs {b}.")

    def test_TsTangentAlgorithmAutoEase(self):
        """
        Test that the "Auto Ease" algorithm works.
        """
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionNone):
            spline = Ts.Spline()
            spline.SetKnot(Ts.Knot(time=0.0, value=0.0,
                                   preTanSlope=0.25, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   postTanSlope=0.75, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   nextInterp=Ts.InterpCurve))

            # Verify that nothing has happened yet as there's only 1 knot in the
            # spline.
            self._CheckTimeTangents(spline, 0.0, 0.25, 0.5, 0.75, 3.0)

            # Add a new knot.
            spline.SetKnot(Ts.Knot(time=6.0, value=3.0,
                                   preTanSlope=0.25, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   postTanSlope=0.75, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   nextInterp=Ts.InterpCurve))

            # Verify that the interior tangents have been updated.
            self._CheckTimeTangents(spline, 0.0, 0.25, 0.5, 0.00, 2.0)
            self._CheckTimeTangents(spline, 6.0, 0.00, 2.0, 0.75, 3.0)

            # Add a knot in the middle
            spline.SetKnot(Ts.Knot(time=3.0, value=2.0,
                                   preTanSlope=0.25, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   postTanSlope=0.75, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   nextInterp=Ts.InterpCurve))

            # Verify the algorithm worked.
            slope3 = self._AutoEaseSlope(0.0, 0.0, 3.0, 2.0, 6.0, 3.0)

            self._CheckTimeTangents(spline, 0.0,   0.25, 0.5,   0.00, 1.0)
            self._CheckTimeTangents(spline, 3.0, slope3, 1.0, slope3, 1.0)
            self._CheckTimeTangents(spline, 6.0,   0.00, 1.0,   0.75, 3.0)

            # Add another knot.
            spline.SetKnot(Ts.Knot(time=1.0, value=1.0,
                                   preTanSlope=0.25, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   postTanSlope=0.75, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   nextInterp=Ts.InterpCurve))

            slope1 = self._AutoEaseSlope(0.0, 0.0, 1.0, 1.0, 3.0, 2.0)
            slope3 = self._AutoEaseSlope(1.0, 1.0, 3.0, 2.0, 6.0, 3.0)

            self._CheckTimeTangents(spline, 0.0,   0.25, 0.5,   0.00, 1/3)
            self._CheckTimeTangents(spline, 1.0, slope1, 1/3, slope1, 2/3)
            self._CheckTimeTangents(spline, 3.0, slope3, 2/3, slope3, 1.0)
            self._CheckTimeTangents(spline, 6.0,   0.00, 1.0,   0.75, 3.0)

            # Add a knot that local minimum. It should force the slope to 0.0
            spline.SetKnot(Ts.Knot(time=5.0, value=1.0,
                                   preTanSlope=0.25, preTanWidth=0.5,
                                   preTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   postTanSlope=0.75, postTanWidth=3.0,
                                   postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                                   nextInterp=Ts.InterpCurve))

            slope1 = self._AutoEaseSlope(0.0, 0.0, 1.0, 1.0, 3.0, 2.0)
            slope3 = self._AutoEaseSlope(1.0, 1.0, 3.0, 2.0, 5.0, 1.0)
            slope5 = self._AutoEaseSlope(3.0, 2.0, 5.0, 1.0, 6.0, 3.0)

            self._CheckTimeTangents(spline, 0.0,   0.25, 0.5,   0.00, 1/3)
            self._CheckTimeTangents(spline, 1.0, slope1, 1/3, slope1, 2/3)
            self._CheckTimeTangents(spline, 3.0, slope3, 2/3, slope3, 2/3)
            self._CheckTimeTangents(spline, 5.0, slope5, 2/3, slope5, 1/3)
            self._CheckTimeTangents(spline, 6.0,   0.00, 1/3,   0.75, 3.0)

    def test_UpdateTangents(self):
        knot0 = Ts.Knot(time=0.0, value=0.0,
                        preTanSlope=0.25, preTanWidth=0.5,
                        preTanAlgorithm=Ts.TangentAlgorithmNone,
                        postTanSlope=0.75, postTanWidth=3.0,
                        postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                        nextInterp=Ts.InterpCurve)

        knot3 = Ts.Knot(time=3.0, value=2.0,
                        preTanSlope=0.25, preTanWidth=0.5,
                        preTanAlgorithm=Ts.TangentAlgorithmNone,
                        postTanSlope=0.75, postTanWidth=3.0,
                        postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                        nextInterp=Ts.InterpCurve)

        knot6 = Ts.Knot(time=6.0, value=3.0,
                        preTanSlope=0.25, preTanWidth=0.5,
                        preTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                        postTanSlope=0.75, postTanWidth=3.0,
                        postTanAlgorithm=Ts.TangentAlgorithmAutoEase,
                        nextInterp=Ts.InterpCurve)

        self._CheckKnotTangents(knot3, 0.25, 0.5, 0.75, 3.0)

        # Verify that update with no neighboring knots does nothing.
        knot3.UpdateTangents(None, None)
        self._CheckKnotTangents(knot3, 0.25, 0.5, 0.75, 3.0)
        knot3.UpdateTangents(None, None, curveType=Ts.CurveTypeHermite)
        self._CheckKnotTangents(knot3, 0.25, 0.5, 0.75, 3.0)

        # There's a preceding knot, but the pre-tangent algorithm is None
        knot3.UpdateTangents(knot0, None)
        self._CheckKnotTangents(knot3, 0.25, 0.5, 0.75, 3.0)
        # If the curve is Hermite, the tangent width gets updated.
        knot3.UpdateTangents(knot0, None, curveType=Ts.CurveTypeHermite)
        self._CheckKnotTangents(knot3, 0.25, 1.0, 0.75, 3.0)

        # Now set a subsequent knot but not previous. The auto ease algorithm
        # sets the tantents at the ends to be flat.
        knot3.UpdateTangents(None, knot6)
        self._CheckKnotTangents(knot3, 0.25, 1.0, 0.00, 1.0)
        # Reset the post tangent and try again as hermite
        knot3.SetPostTanSlope(0.75)
        knot3.SetPostTanWidth(3.0)
        knot3.UpdateTangents(None, knot6, curveType=Ts.CurveTypeHermite)
        self._CheckKnotTangents(knot3, 0.25, 1.0, 0.00, 1.0)

        # Now the full algorithm
        knot3.UpdateTangents(knot0, knot6)
        slope = self._AutoEaseSlope(0.0, 0.0, 3.0, 2.0, 6.0, 3.0)
        self._CheckKnotTangents(knot3, 0.25, 1.0, slope, 1.0)


if __name__ == "__main__":
    unittest.main()
