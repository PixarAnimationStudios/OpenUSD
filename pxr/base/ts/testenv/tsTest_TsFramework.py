#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_CompareBaseline as CompareBaseline
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

        self.assertTrue(comparator.GetMaxDiff() < 1.0)
        if Comparator.Init():
            comparator.Write("test_Comparator.png")

    def test_Looping(self):
        """
        Verify that Grapher correctly displays loops.
        To really be sure, inspect the graph image output.
        """
        data = Museum.GetData(Museum.SimpleInnerLoop)

        baked = Evaluator().BakeInnerLoops(data)

        times = STimes(baked)
        times.AddStandardTimes()

        samples = Evaluator().Eval(data, times)

        grapher = Grapher("test_Looping")
        grapher.AddSpline("Looping", data, samples, baked = baked)

        if Grapher.Init():
            grapher.Write("test_Looping.png")

    def test_Baseline(self):
        """
        Verify that TsEvaluator and CompareBaseline are working.
        """
        data = Museum.GetData(Museum.TwoKnotBezier)

        times = STimes(data)
        times.AddStandardTimes()

        samples = Evaluator().Eval(data, times)

        self.assertTrue(CompareBaseline("test_Baseline", data, samples))

    def test_BaselineFail(self):
        """
        Simulate an unintended change in evaluation results, and verify that
        CompareBaseline catches and reports the difference correctly.
        """
        # Pretend this is the data we used.  This is the data that's in the
        # baseline file.
        data1 = Museum.GetData(Museum.TwoKnotBezier)

        # Actually evaluate using this data.
        data2 = Museum.GetData(Museum.TwoKnotLinear)

        times = STimes(data1)
        times.AddStandardTimes()

        samples = Evaluator().Eval(data2, times)

        # Pass the data2 samples, but the data1 input data.
        # This should trigger a value mismatch and a diff report.
        self.assertFalse(CompareBaseline("test_BaselineFail", data1, samples))


if __name__ == "__main__":
    unittest.main()
