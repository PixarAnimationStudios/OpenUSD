#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_Baseliner as Baseliner
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Grapher as Grapher
from pxr.Ts import TsTest_Comparator as Comparator

import unittest


class TsTest_TsFramework(unittest.TestCase):

    def test_Grapher(self):
        """
        Verify that TsEvaluator and Grapher are working.
        To really be sure, inspect the graph image output.
        """
        spline1 = Museum.GetSpline(Museum.TwoKnotBezier)
        spline2 = Museum.GetSpline(Museum.TwoKnotLinear)

        times = STimes(spline1)
        times.AddStandardTimes()

        samples1 = Evaluator().Eval(spline1, times)
        samples2 = Evaluator().Eval(spline2, times)

        grapher = Grapher("test_Grapher")
        grapher.AddSpline("Bezier", spline1, samples1)
        grapher.AddSpline("Linear", spline2, samples2)

        if Grapher.Init():
            grapher.Write("test_Grapher.png")

    def test_Comparator(self):
        """
        Verify that TsEvaluator and Comparator are working.
        To really be sure, inspect the graph image output.
        """
        spline1 = Museum.GetSpline(Museum.TwoKnotBezier)
        spline2 = Museum.GetSpline(Museum.TwoKnotLinear)

        times = STimes(spline1)
        times.AddStandardTimes()

        samples1 = Evaluator().Eval(spline1, times)
        samples2 = Evaluator().Eval(spline2, times)

        comparator = Comparator("test_Comparator")
        comparator.AddSpline("Bezier", spline1, samples1)
        comparator.AddSpline("Linear", spline2, samples2)

        if Comparator.Init():
            comparator.Write("test_Comparator.png")

        self.assertTrue(comparator.GetMaxDiff() < 1.0)

#    def test_Looping(self):
#        """
#        Verify that Grapher correctly displays loops.
#        To really be sure, inspect the graph image output.
#        """
#        spline = Museum.GetSpline(Museum.SimpleInnerLoop)
#
#        baked = Evaluator().BakeInnerLoops(spline)
#
#        times = STimes(baked)
#        times.AddStandardTimes()
#
#        samples = Evaluator().Eval(spline, times)
#
#        grapher = Grapher("test_Looping")
#        grapher.AddSpline("Looping", spline, samples, baked = baked)
#
#        if Grapher.Init():
#            grapher.Write("test_Looping.png")

    def test_Baseline(self):
        """
        Verify that TsEvaluator and Baseliner are working.
        """
        spline = Museum.GetSpline(Museum.TwoKnotBezier)

        times = STimes(spline)
        times.AddStandardTimes()

        samples = Evaluator().Eval(spline, times)

        baseliner = Baseliner.CreateForEvalCompare(
            "test_Baseline", spline, samples)
        self.assertTrue(baseliner.Validate())

    def test_BaselineParams(self):
        """
        Verify that Baseliner works in param-compare mode.
        """
        spline = Museum.GetSpline(Museum.TwoKnotBezier)

        times = STimes(spline)
        times.AddStandardTimes()

        samples = Evaluator().Eval(spline, times)

        baseliner = Baseliner.CreateForParamCompare(
            "test_BaselineParams", spline, samples)
        self.assertTrue(baseliner.Validate())

    def test_BaselineFail(self):
        """
        Simulate an unintended change in evaluation results, and verify that
        Baseliner catches and reports the difference correctly.
        """
        # Pretend this is the data we used.  This is the data that's in the
        # baseline file.
        spline1 = Museum.GetSpline(Museum.TwoKnotBezier)

        # Actually evaluate using this data.
        spline2 = Museum.GetSpline(Museum.TwoKnotLinear)
        times = STimes(spline1)
        times.AddStandardTimes()
        samples = Evaluator().Eval(spline2, times)

        # Also create a reference spline with a tweaked knot.  This just allows
        # us to test that reference splines are being drawn correctly in the
        # output graph.
        refSpline = Museum.GetSpline(Museum.TwoKnotBezier)
        refKnot = refSpline.GetKnots().values()[0]
        refKnot.SetPostTanSlope(refKnot.GetPostTanSlope() * 0.8)
        refSpline.SetKnot(refKnot)
        refSamples = Evaluator().Eval(refSpline, times)

        # Pass the spline2 samples, but the spline1 input data.
        # This should trigger a value mismatch and a diff report.
        baseliner = Baseliner.CreateForEvalCompare(
            "test_BaselineFail", spline1, samples)
        baseliner.AddReferenceSpline("Reference", refSpline, refSamples)
        self.assertFalse(baseliner.Validate())

    def test_BaselineParamsFail(self):
        """
        Simulate an unintended change in spline params, and verify that
        Baseliner catches and reports the difference correctly.
        """
        # Pretend this is the data we used.  This is the data that's in the
        # baseline file.
        spline1 = Museum.GetSpline(Museum.TwoKnotBezier)

        # Pass this data to Baseliner.  It differs from the baseline file, which
        # was made from Museum.TwoKnotBezier.
        spline2 = Museum.GetSpline(Museum.TwoKnotLinear)
        times = STimes(spline2)
        times.AddStandardTimes()
        samples = Evaluator().Eval(spline2, times)

        # Also create a reference spline with a tweaked knot.  This just allows
        # us to test that reference splines are being drawn correctly in the
        # output graph.
        refSpline = spline1
        refKnot = refSpline.GetKnots().values()[0]
        refKnot.SetPostTanSlope(refKnot.GetPostTanSlope() * 0.8);
        refSpline.SetKnot(refKnot)
        refSamples = Evaluator().Eval(refSpline, times)

        # This should trigger a param mismatch and a diff report.
        baseliner = Baseliner.CreateForParamCompare(
            "test_BaselineParamsFail", spline2, samples)
        baseliner.AddReferenceSpline("Reference", refSpline, refSamples)
        self.assertFalse(baseliner.Validate())


if __name__ == "__main__":
    unittest.main()
