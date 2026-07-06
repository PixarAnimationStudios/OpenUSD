#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts
from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_SampleBezier as SampleBezier
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Comparator as Comparator

import unittest, functools


class TsTest_TsVsBezier(unittest.TestCase):

    def _DoTest(self, name):
        """
        Obtain a spline from the Museum, evaluate it, and compare the results
        against a ground-truth Bezier sampled with de Casteljau.
        """
        # Get the Museum spline.
        spline = Museum.GetSplineByName(name)

        # Perform Bezier sampling.
        bezSamples = SampleBezier(spline, numSamples = 200)

        # Copy the sample times that were returned.
        times = STimes()
        times.AddTimes([s.time for s in bezSamples])

        # Perform evaluation at the same times.
        evalSamples = Evaluator().Eval(spline, times)

        # Compare.
        comparator = Comparator(title = name)
        comparator.AddSpline("Bezier", spline, bezSamples)
        comparator.AddSpline("Ts", spline, evalSamples)

        # Create graph if possible.
        if Comparator.Init():
            comparator.Write(f"{name}.png")

        # Pick a tolerance based on the case.  Note that these are absolute
        # numbers, not adjusted for scale, and tuned based on the Museum data.
        if name in ["FourThirdOneThird", "FringeVert",
                    "OneThirdFourThird", "VerticalTorture"]:
            # These cases have single verticals, and eval-time anti-regression
            # kicks in conservatively, resulting in greater diffs at the
            # vertical.
            tolerance = 1e-1
        else:
            # The remaining cases are far from regressive, and match the Bezier
            # well.
            tolerance = 1e-12

        # Verify results are close.
        self.assertTrue(comparator.GetMaxDiff() < tolerance)


if __name__ == "__main__":

    # Dynamically generate test methods based on the Museum contents.
    # This must be done before unittest.main(), or it doesn't work.
    for name in Museum.GetAllNames():

        # Skip regressive cases.  We deliberately do not match the actual Bezier
        # curve in these cases.
        if name.startswith("Regressive"):
            continue

        # Skip cases that include non-Bezier segments, and that have features
        # unsupported by a simple bezier implementation -- inner loops,
        # extrapolation loops, sloped extrapolation.
        spline = Museum.GetSplineByName(name)
        if spline.GetCurveType() != Ts.CurveTypeBezier or \
                spline.HasInnerLoops() or \
                spline.GetPreExtrapolation().mode == Ts.ExtrapSloped or \
                spline.GetPostExtrapolation().mode == Ts.ExtrapSloped or \
                spline.GetPreExtrapolation().IsLooping() or \
                spline.GetPostExtrapolation().IsLooping():
            continue

        # Skip cases whose knots have auto ease tangent algorithms or have
        # dual values
        skipKnotFeatures = False
        for knot in spline.GetKnots().values():
            if knot.IsDualValued() or \
                    knot.GetPreTanAlgorithm() == Ts.TangentAlgorithmAutoEase or \
                    knot.GetPostTanAlgorithm() == Ts.TangentAlgorithmAutoEase:
                skipKnotFeatures = True
        if skipKnotFeatures:
            continue

        def func(self, name):
            self._DoTest(name)

        method = functools.partialmethod(func, name)
        setattr(TsTest_TsVsBezier, f"test_{name}", method)

    unittest.main()
