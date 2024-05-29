#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_AnimXEvaluator as Evaluator
from pxr.Ts import TsTest_CompareBaseline as CompareBaseline
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Grapher as Grapher
from pxr.Ts import TsTest_Comparator as Comparator

import unittest


class TsTest_AnimXFramework(unittest.TestCase):

    def test_Grapher(self):
        """
        Verify that AnimXEvaluator and Grapher are working.
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
        Verify that AnimXEvaluator and Comparator are working.
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

    def test_Baseline(self):
        """
        Verify that AnimXEvaluator and CompareBaseline are working.
        """
        data = Museum.GetData(Museum.TwoKnotBezier)

        times = STimes(data)
        times.AddStandardTimes()

        samples = Evaluator().Eval(data, times)

        self.assertTrue(CompareBaseline("test_Baseline", data, samples))


if __name__ == "__main__":
    unittest.main()
