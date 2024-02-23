#!/pxrpythonsubst

#
# Copyright 2023 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
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
