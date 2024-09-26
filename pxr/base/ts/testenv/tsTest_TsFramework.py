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
from pxr.Ts import TsTest_SplineData as SData
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
        data1 = Museum.GetData(Museum.TwoKnotBezier)
        data2 = Museum.GetData(Museum.TwoKnotLinear)

        times = STimes(data1)
        times.AddStandardTimes()

        samples1 = Evaluator().Eval(data1, times)
        samples2 = Evaluator().Eval(data2, times)

        grapher = Grapher("test_Grapher")
        grapher.AddSpline("Bezier", data1, samples1)
        grapher.AddSpline("Linear", data2, samples2)

        if Grapher.Init():
            grapher.Write("test_Grapher.png")

    def test_Comparator(self):
        """
        Verify that TsEvaluator and Comparator are working.
        To really be sure, inspect the graph image output.
        """
        data1 = Museum.GetData(Museum.TwoKnotBezier)
        data2 = Museum.GetData(Museum.TwoKnotLinear)

        times = STimes(data1)
        times.AddStandardTimes()

        samples1 = Evaluator().Eval(data1, times)
        samples2 = Evaluator().Eval(data2, times)

        comparator = Comparator("test_Comparator")
        comparator.AddSpline("Bezier", data1, samples1)
        comparator.AddSpline("Linear", data2, samples2)

        if Comparator.Init():
            comparator.Write("test_Comparator.png")

        self.assertTrue(comparator.GetMaxDiff() < 1.0)

#    def test_Looping(self):
#        """
#        Verify that Grapher correctly displays loops.
#        To really be sure, inspect the graph image output.
#        """
#        data = Museum.GetData(Museum.SimpleInnerLoop)
#
#        baked = Evaluator().BakeInnerLoops(data)
#
#        times = STimes(baked)
#        times.AddStandardTimes()
#
#        samples = Evaluator().Eval(data, times)
#
#        grapher = Grapher("test_Looping")
#        grapher.AddSpline("Looping", data, samples, baked = baked)
#
#        if Grapher.Init():
#            grapher.Write("test_Looping.png")

    def test_Baseline(self):
        """
        Verify that TsEvaluator and Baseliner are working.
        """
        data = Museum.GetData(Museum.TwoKnotBezier)

        times = STimes(data)
        times.AddStandardTimes()

        samples = Evaluator().Eval(data, times)

        baseliner = Baseliner.CreateForEvalCompare(
            "test_Baseline", data, samples)
        self.assertTrue(baseliner.Validate())

    def test_BaselineParams(self):
        """
        Verify that Baseliner works in param-compare mode.
        """
        data = Museum.GetData(Museum.TwoKnotBezier)

        times = STimes(data)
        times.AddStandardTimes()

        samples = Evaluator().Eval(data, times)

        baseliner = Baseliner.CreateForParamCompare(
            "test_BaselineParams", data, samples)
        self.assertTrue(baseliner.Validate())

    def test_BaselineFail(self):
        """
        Simulate an unintended change in evaluation results, and verify that
        Baseliner catches and reports the difference correctly.
        """
        # Pretend this is the data we used.  This is the data that's in the
        # baseline file.
        data1 = Museum.GetData(Museum.TwoKnotBezier)

        # Actually evaluate using this data.
        data2 = Museum.GetData(Museum.TwoKnotLinear)
        times = STimes(data1)
        times.AddStandardTimes()
        samples = Evaluator().Eval(data2, times)

        # Also create a reference spline with a tweaked knot.  This just allows
        # us to test that reference splines are being drawn correctly in the
        # output graph.
        refData = SData(data1)
        refKnot = refData.GetKnots()[0]
        refKnot.postSlope *= 0.8;
        refData.AddKnot(refKnot)
        refSamples = Evaluator().Eval(refData, times)

        # Pass the data2 samples, but the data1 input data.
        # This should trigger a value mismatch and a diff report.
        baseliner = Baseliner.CreateForEvalCompare(
            "test_BaselineFail", data1, samples)
        baseliner.AddReferenceSpline("Reference", refData, refSamples)
        self.assertFalse(baseliner.Validate())

    def test_BaselineParamsFail(self):
        """
        Simulate an unintended change in spline params, and verify that
        Baseliner catches and reports the difference correctly.
        """
        # Pretend this is the data we used.  This is the data that's in the
        # baseline file.
        data1 = Museum.GetData(Museum.TwoKnotBezier)

        # Pass this data to Baseliner.  It differs from the baseline file, which
        # was made from Museum.TwoKnotBezier.
        data2 = Museum.GetData(Museum.TwoKnotLinear)
        times = STimes(data2)
        times.AddStandardTimes()
        samples = Evaluator().Eval(data2, times)

        # Also create a reference spline with a tweaked knot.  This just allows
        # us to test that reference splines are being drawn correctly in the
        # output graph.
        refData = SData(data1)
        refKnot = refData.GetKnots()[0]
        refKnot.postSlope *= 0.8;
        refData.AddKnot(refKnot)
        refSamples = Evaluator().Eval(refData, times)

        # This should trigger a param mismatch and a diff report.
        baseliner = Baseliner.CreateForParamCompare(
            "test_BaselineParamsFail", data2, samples)
        baseliner.AddReferenceSpline("Reference", refData, refSamples)
        self.assertFalse(baseliner.Validate())


if __name__ == "__main__":
    unittest.main()
