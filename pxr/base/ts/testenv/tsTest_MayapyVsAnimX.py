#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_MayapyEvaluator as MayapyEvaluator
from pxr.Ts import TsTest_AnimXEvaluator as AnimXEvaluator
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Comparator as Comparator

import sys, unittest

g_mayapyEvaluator = None


class TsTest_MayapyVsAnimX(unittest.TestCase):

    def test_Basic(self):
        """
        Verify mayapy and AnimX evaluation are close in one simple case.
        To really be sure, inspect the graph image output.

        TODO: can these two backends match exactly?
        """
        data = Museum.GetData(Museum.TwoKnotBezier)

        times = STimes(data)
        times.AddStandardTimes()

        mayapySamples = g_mayapyEvaluator.Eval(data, times)
        animXSamples = AnimXEvaluator().Eval(data, times)

        comparator = Comparator("test_Basic")
        comparator.AddSpline("mayapy", data, mayapySamples)
        comparator.AddSpline("AnimX", data, animXSamples)

        self.assertTrue(comparator.GetMaxDiff() < 1e-7)
        if Comparator.Init():
            comparator.Write("test_Basic.png")

    @classmethod
    def tearDownClass(cls):
        """
        Clean up after all tests have run.
        """
        g_mayapyEvaluator.Shutdown()


if __name__ == "__main__":

    mayapyPath = sys.argv.pop()
    g_mayapyEvaluator = MayapyEvaluator(mayapyPath)

    unittest.main()
