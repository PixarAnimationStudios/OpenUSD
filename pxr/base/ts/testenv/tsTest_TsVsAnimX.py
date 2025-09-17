#!/pxrpythonsubst

#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_AnimXEvaluator as AnimXEvaluator
from pxr.Ts import TsTest_TsEvaluator as TsEvaluator
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_Comparator as Comparator

import unittest


class TsTest_TsVsAnimX(unittest.TestCase):

    def _DoTest(self, case, dataId, extrapLoop = False):
        """
        Illustrate the differences between Ts and AnimX.
        """
        data = Museum.GetData(dataId)

        times = STimes(data)
        times.AddStandardTimes()

        tsSamples = TsEvaluator().Eval(data, times)
        animXSamples = AnimXEvaluator().Eval(data, times)

        comparator = Comparator(title = case)
        comparator.AddSpline("Ts", data, tsSamples)
        comparator.AddSpline("AnimX", data, animXSamples)

        if comparator.Init():
            comparator.Write(f"{case}.png")

        maxTime, maxSamples = comparator.GetMaxDiffSamples()
        preStr = " (pre)" if maxTime.pre else ""
        print(f"{case}: Max diff: time {maxTime.time}{preStr}: "
              f"Ts {maxSamples[0].value}, AnimX {maxSamples[1].value}")

        # Exempt the extrapolating-loop cases from comparison.  Look at the
        # graphs to understand what's going on.  Ts and AnimX agree quite well
        # on these, except at loop joins.  There is a limit to how closely they
        # can agree, given that AnimX doesn't support pre-evaluation; this
        # results in some larger, but still small, differences.  In addition, in
        # Reset mode, AnimX seems to choose inconsistently between the pre- and
        # post-sides at exactly the time of a loop-join discontinuity; this
        # results in large differences.
        if not extrapLoop:
            self.assertTrue(comparator.GetMaxDiff() < 1e-13)

    def test_Basic(self):
        self._DoTest("test_Basic", Museum.TwoKnotBezier)

    def test_BoldS(self):
        self._DoTest("test_BoldS", Museum.BoldS)

    def test_Cusp(self):
        self._DoTest("test_Cusp", Museum.Cusp)

    def test_ExtrapRepeat(self):
        self._DoTest(
            "test_ExtrapRepeat", Museum.ExtrapLoopRepeat,
            extrapLoop = True)

    def test_ExtrapReset(self):
        self._DoTest(
            "test_ExtrapReset", Museum.ExtrapLoopReset,
            extrapLoop = True)

    def test_ExtrapOscillate(self):
        self._DoTest(
            "test_ExtrapOscillate", Museum.ExtrapLoopOscillate,
            extrapLoop = True)


if __name__ == "__main__":
    unittest.main()
