#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_MayapyEvaluator as Evaluator
from pxr.Ts import TsTest_CompareBaseline as CompareBaseline
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Grapher as Grapher
from pxr.Ts import TsTest_Comparator as Comparator

import sys, unittest

g_evaluator = None


# MayapyEvaluator subclass that writes debug messages to stdout.
class _TestEvaluator(Evaluator):
    def _DebugLog(self, msg):
        sys.stdout.write(msg)


class TsTest_MayapyFramework(unittest.TestCase):

    def test_Grapher(self):
        """
        Verify that MayapyEvaluator and Grapher are working.
        To really be sure, inspect the graph image output.
        """
        data1 = Museum.GetData(Museum.TwoKnotBezier)
        data2 = Museum.GetData(Museum.TwoKnotLinear)

        times = STimes(data1)
        times.AddStandardTimes()

        samples1 = g_evaluator.Eval(data1, times)
        samples2 = g_evaluator.Eval(data2, times)

        grapher = Grapher("test_Grapher")
        grapher.AddSpline("Bezier", data1, samples1)
        grapher.AddSpline("Linear", data2, samples2)

        if Grapher.Init():
            grapher.Write("test_Grapher.png")

    def test_Comparator(self):
        """
        Verify that MayapyEvaluator and Comparator are working.
        To really be sure, inspect the graph image output.
        """
        data1 = Museum.GetData(Museum.TwoKnotBezier)
        data2 = Museum.GetData(Museum.TwoKnotLinear)

        times = STimes(data1)
        times.AddStandardTimes()

        samples1 = g_evaluator.Eval(data1, times)
        samples2 = g_evaluator.Eval(data2, times)

        comparator = Comparator("test_Comparator")
        comparator.AddSpline("Bezier", data1, samples1)
        comparator.AddSpline("Linear", data2, samples2)

        self.assertTrue(comparator.GetMaxDiff() < 1.0)
        if Comparator.Init():
            comparator.Write("test_Comparator.png")

    def test_Baseline(self):
        """
        Verify that MayapyEvaluator and CompareBaseline are working.
        """
        data = Museum.GetData(Museum.TwoKnotBezier)

        times = STimes(data)
        times.AddStandardTimes()

        samples = g_evaluator.Eval(data, times)

        self.assertTrue(CompareBaseline("test_Baseline", data, samples))

    @classmethod
    def tearDownClass(cls):
        """
        Clean up after all tests have run.
        """
        g_evaluator.Shutdown()


if __name__ == "__main__":

    mayapyPath = sys.argv.pop()
    g_evaluator = _TestEvaluator(
        mayapyPath, subprocessDebugFilePath = "debugMayapyDriver.txt")

    unittest.main()
