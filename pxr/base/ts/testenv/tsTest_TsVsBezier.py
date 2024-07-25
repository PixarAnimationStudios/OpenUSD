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
from pxr.Ts import TsTest_SplineData as SData
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
        # Get the Museum data.
        data = Museum.GetDataByName(name)

        # Perform Bezier sampling.
        bezSamples = SampleBezier(data, numSamples = 200)

        # Copy the sample times that were returned.
        times = STimes()
        times.AddTimes([s.time for s in bezSamples])

        # Perform evaluation at the same times.
        evalSamples = Evaluator().Eval(data, times)

        # Compare.
        comparator = Comparator(title = name)
        comparator.AddSpline("Bezier", data, bezSamples)
        comparator.AddSpline("Ts", data, evalSamples)

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

        # Skip cases that include non-Bezier segments.
        data = Museum.GetDataByName(name)
        if data.GetRequiredFeatures() != SData.FeatureBezierSegments:
            continue

        def func(self, name):
            self._DoTest(name)

        method = functools.partialmethod(func, name)
        setattr(TsTest_TsVsBezier, f"test_{name}", method)

    unittest.main()
