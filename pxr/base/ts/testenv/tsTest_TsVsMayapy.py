#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_MayapyEvaluator as MayapyEvaluator
from pxr.Ts import TsTest_TsEvaluator as TsEvaluator
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Comparator as Comparator

import sys, unittest

g_mayapyEvaluator = None


class TsTest_TsVsMayapy(unittest.TestCase):

    def _DoTest(self, case, dataId, tolerance):
        """
        Illustrate the differences between Ts and mayapy.
        """
        data = Museum.GetData(dataId)

        times = STimes(data)
        times.AddStandardTimes()

        tsSamples = TsEvaluator().Eval(data, times)
        mayapySamples = g_mayapyEvaluator.Eval(data, times)

        comparator = Comparator(title = case)
        comparator.AddSpline("Ts", data, tsSamples)
        comparator.AddSpline("mayapy", data, mayapySamples)

        if comparator.Init():
            comparator.Write(f"{case}.png")

        self.assertTrue(comparator.GetMaxDiff() < tolerance)

    def test_Basic(self):
        self._DoTest("test_Basic", Museum.TwoKnotBezier, 1e-7)

    def test_BoldS(self):
        self._DoTest("test_BoldS", Museum.BoldS, 1e-4)

    def test_Cusp(self):
        self._DoTest("test_Cusp", Museum.Cusp, 1e-6)


def setUpModule():
    """
    Called before any test classes are instantiated.
    Set up a single instance of MayapyEvaluator, which is expensive to start.
    """
    global g_mayapyEvaluator
    g_mayapyEvaluator = MayapyEvaluator(g_mayapyPath)

def tearDownModule():
    """
    Called after all test classes have been run.
    Shut down MayapyEvaluator cleanly; otherwise it will hang on exit.
    """
    g_mayapyEvaluator.Shutdown()


if __name__ == "__main__":

    # Grab the path to mayapy from the command line.
    # Prevent unittest from seeing that arg, which it won't understand.
    g_mayapyPath = sys.argv.pop()

    unittest.main()
