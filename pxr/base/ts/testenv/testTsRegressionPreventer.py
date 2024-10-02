#!/pxrpythonsubst

#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Ts
from pxr.Ts import TsTest_Museum as Museum
from pxr.Ts import TsTest_TsEvaluator as Evaluator
from pxr.Ts import TsTest_Baseliner as Baseliner
from pxr.Ts import TsTest_SampleTimes as STimes
from pxr.Ts import TsTest_SampleBezier as SampleBezier

import sys, unittest, difflib


class TestTsRegressionPreventer(unittest.TestCase):

    def _DoTest(
            self, caseName, museumId, knotTime,
            knotDelta = 0, preDelta = 0, postDelta = 0,
            isRegressive = False, isInitiallyRegressive = False):
        """
        Modify a knot with a RegressionPreventer, verify the outcome, and, if
        possible, write a graph image of the modified spline.
        """
        # Get the starting spline.
        origData = Museum.GetData(museumId)
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionNone):
            origSpline = Evaluator().SplineDataToSpline(
                origData)

        # Copy the knot we're modifying.
        knot = origSpline.GetKnot(knotTime)

        # Modify knot time.
        knot.SetTime(knot.GetTime() + knotDelta)

        # Modify pre-tan width.
        knot.SetPreTanWidth(knot.GetPreTanWidth() + preDelta)

        # Modify post-tan width.
        knot.SetPostTanWidth(knot.GetPostTanWidth() + postDelta)

        # Copy the starting spline, and set the modified knot without
        # anti-regression.
        unfilteredSpline = Ts.Spline(origSpline)
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionNone):
            nonPreventer = Ts.RegressionPreventer(unfilteredSpline, knotTime)
        result = nonPreventer.Set(knot)
        self.assertTrue(result)

        # Convert the unfiltered spline to splineData, and sample as a Bezier.
        # This illustrates the possibly regressive spline that results from the
        # unfiltered edit.
        bezSplineData = Evaluator().SplineToSplineData(unfilteredSpline)
        bezSamples = SampleBezier(bezSplineData, numSamples = 200)

        # Gather test results, but always keep going.  This ensures we get all
        # possible information.
        ok = True

        # Verify TsSpline::HasRegressiveTangents works as expected.
        with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionKeepRatio):
            regressiveBefore = unfilteredSpline.HasRegressiveTangents()
        if regressiveBefore != isRegressive:
            print(f"{caseName}: HasRegressiveTangents mismatch beforehand")
            ok = False

        # Exercise all modes.
        for mode in \
                Ts.AntiRegressionMode.allValues \
                + Ts.RegressionPreventer.InteractiveMode.allValues:

            # Generate a name for the (case, mode) pair.
            modeStr = mode.displayName.replace(" ", "")
            subCaseStr = caseName + modeStr

            # Copy the starting spline.
            spline = Ts.Spline(origSpline)

            # Set up RegressionPreventer.
            if mode in Ts.AntiRegressionMode.allValues:
                with Ts.AntiRegressionAuthoringSelector(mode):
                    preventer = Ts.RegressionPreventer(spline, knotTime)
            else:
                preventer = Ts.RegressionPreventer(spline, knotTime, mode)

            # Use RegressionPreventer to set the modified knot.
            result = preventer.Set(knot)
            self.assertTrue(result)

            # Use Evaluator to generate splineData and samples.
            splineData = Evaluator().SplineToSplineData(spline)
            times = STimes(splineData)
            times.AddStandardTimes()
            samples = Evaluator().Eval(splineData, times)

            # Use Baseliner to validate modified + anti-regressed spline.
            # Include the result of Set() to be diffed against baseline.
            # Include the non-anti-regressed Bezier as a reference.
            baseliner = Baseliner.CreateForParamCompare(
                subCaseStr, splineData, samples, precision = 6)
            baseliner.SetCreationString(
                result.GetDebugDescription(precision = 6))
            baseliner.AddReferenceSpline("Bezier", bezSplineData, bezSamples)
            subCaseOk = baseliner.Validate()
            if not subCaseOk:
                ok = False

            # Verify HasRegressiveTangents works as expected after
            # anti-regression.
            with Ts.AntiRegressionAuthoringSelector(Ts.AntiRegressionKeepRatio):
                regressiveAfter = unfilteredSpline.HasRegressiveTangents()
            if mode == Ts.AntiRegressionNone:
                if regressiveAfter != isRegressive:
                    print(f"{subCaseStr}: HasRegressiveTangents "
                          "mismatch afterward")
                    ok = False
            else:
                if spline.HasRegressiveTangents():
                    print(f"{subCaseStr}: HasRegressiveTangents "
                          "returns True afterward")
                    ok = False

            # Verify that other APIs give the same result.
            if mode in [Ts.AntiRegressionContain,
                        Ts.AntiRegressionKeepRatio,
                        Ts.AntiRegressionKeepStart]:

                # If the spline was regressive to begin with, then batch and
                # interactive cases won't necessarily agree.  In the batch case,
                # only initial anti-regression is performed; in the interactive
                # case, initial anti-regression is performed, then the active
                # knot or tangent is potentially moved from its de-regressed
                # position.
                if isInitiallyRegressive:
                    continue

                # Call AdjustRegressiveTangents.
                batchSpline = Ts.Spline(unfilteredSpline)
                with Ts.AntiRegressionAuthoringSelector(mode):
                    batchSpline.AdjustRegressiveTangents()
                if not self._DiffSplines(
                        "RegressionPreventer", spline,
                        "AdjustRegressiveTangents", batchSpline,
                        f"{subCaseStr}: batch spline differs"):
                    ok = False

                # Call SetKnot.
                editSpline = Ts.Spline(origSpline)
                editSpline.RemoveKnot(knotTime)
                with Ts.AntiRegressionAuthoringSelector(mode):
                    editSpline.SetKnot(knot)
                if not self._DiffSplines(
                        "RegressionPreventer", spline,
                        "SetKnot", editSpline,
                        f"{subCaseStr}: edit spline differs"):
                    ok = False

        # Fail if any sub-cases failed.
        self.assertTrue(ok)

    def _DiffSplines(self, name1, spline1, name2, spline2, msg):

        # XXX: should just stringify splines, rather than going through TsTest
        data1 = Evaluator().SplineToSplineData(spline1)
        data2 = Evaluator().SplineToSplineData(spline2)
        lines1 = data1.GetDebugDescription().splitlines(keepends = True)
        lines2 = data2.GetDebugDescription().splitlines(keepends = True)

        # ...however, this produced a strange error (Unicode?)
        #lines1 = str(spline1)
        #lines2 = str(spline2)

        if lines2 == lines1:
            return True

        print(msg)
        print()
        sys.stdout.writelines(
            difflib.unified_diff(
                lines1, lines2,
                fromfile = name1, tofile = name2,
                n = len(lines1) + len(lines2)))
        print()

        return False

    def test_Noop(self):
        """
        Test what happens with no change.
        """
        self._DoTest(
            "Noop", Museum.NearCenterVertical,
            knotTime = 0,
            isRegressive = False)

    def test_InitiallyRegressive(self):
        """
        Make a no-op change to a spline that was regressive to begin with.  This
        illustrates initial anti-regression, plus re-asserting the original
        point.
        """
        self._DoTest(
            "InitiallyRegressive", Museum.RegressiveS,
            knotTime = 156,
            isRegressive = True, isInitiallyRegressive = True)

    def test_PreRegressive(self):
        """
        Lengthen the start tangent to cause regression.
        """
        self._DoTest(
            "PreRegressive", Museum.NearCenterVertical,
            knotTime = 0, postDelta = 0.5,
            isRegressive = True)

    def test_PostRegressive(self):
        """
        Lengthen the end tangent to cause regression.
        """
        self._DoTest(
            "PostRegressive", Museum.NearCenterVertical,
            knotTime = 1, preDelta = 0.5,
            isRegressive = True)

    def test_MoveStart(self):
        """
        Move the start knot later to cause regression.
        """
        self._DoTest(
            "MoveStart", Museum.NearCenterVertical,
            knotTime = 0, knotDelta = 0.4,
            isRegressive = True)

    def test_Replace(self):
        """
        Move one knot exactly over another, replacing it.
        """
        self._DoTest(
            "Replace", Museum.FourKnotBezier,
            knotTime = 2, knotDelta = 1,
            isRegressive = False)

    def test_Reorder(self):
        """
        Move one knot across another, close enough to cause regression.
        """
        self._DoTest(
            "Reorder", Museum.FourKnotBezier,
            knotTime = 2, knotDelta = 1.2,
            isRegressive = True)


if __name__ == "__main__":

    unittest.main()
