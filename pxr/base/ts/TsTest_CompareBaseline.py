#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from .TsTest_Grapher import TsTest_Grapher
from .TsTest_Comparator import TsTest_Comparator
from pxr import Ts, Gf
import os, re


def TsTest_CompareBaseline(
        testName, splineData, samples, precision = 7):
    """
    A test helper function for spline evaluation.  Compares samples against the
    contents of a baseline file, and returns whether they match within the
    specified precision.

    Precision is specified as a number of decimal digits to the right of the
    decimal point.

    One of the following will occur:

    - If there is no baseline file yet, a candidate baseline file will be
    written to the test run directory, along with a graph made from the spline
    data and samples.  If the graph looks right, both of these files should be
    copied into the test source, such that they are installed into a "baseline"
    subdirectory of the test run directory.  (The graph isn't necessary for
    operation of the test, but is a useful reference for maintainers.)  The
    function will return False in this case.

    - If the spline data, sample times, or precision differ from those recorded
    in the baseline file, that is an error in the test setup; it is a difference
    in the test inputs rather than the outputs.  Candidate baseline files will
    be written as in the above case, and the function will return False.  If the
    inputs are being changed deliberately, the new baseline files should be
    inspected and installed.

    - If any sample values differ from the baseline values by more than the
    specified precision, all differing samples will be listed on stdout,
    candidate baseline files will be written, a graph of the differences will
    also be written, and the function will return False.  If the change in
    output is expected, the diff graph should be inspected and the new baseline
    files installed.

    - Otherwise, no files will be written, and the function will return True.
    """
    baseliner = _Baseliner(testName, splineData, samples, precision)
    return baseliner.Validate()


class _Baseliner(object):

    def __init__(self, testName, splineData, samples, precision):

        self._testName = testName
        self._fileName = "%s_TsTestBaseline.txt" % testName
        self._splineData = splineData
        self._samples = samples
        self._precision = precision
        self._epsilon = 10 ** -self._precision

        # These are filled in by _ReadBaseline.
        self._baselineSplineDesc = ""
        self._baselineSampleLineOffset = 0
        self._baselinePrecision = 0
        self._baselineSamples = []

    def Validate(self):

        # If there's no baseline file, create candidate baseline and graph.
        baselinePath = os.path.join("baseline", self._fileName)
        if not os.path.exists(baselinePath):
            print("%s: no baseline yet" % self._testName)
            self._WriteCandidates()
            return False

        # Read baseline file.  Verify it can be parsed correctly.
        with open(baselinePath) as infile:
            if not self._ReadBaseline(infile, baselinePath):
                self._WriteCandidates()
                return False

        # Verify evaluation inputs match baseline inputs.
        if not self._ValidateInputs():
            self._WriteCandidates()
            return False

        # Verify sample values match baseline values.
        if not self._ValidateValues():
            self._WriteCandidates()
            self._WriteDiffGraph()
            return False

        # Everything matches.
        return True

    def _ValidateInputs(self):

        # Input spline data should always match.
        splineDesc = self._splineData.GetDebugDescription()
        if splineDesc != self._baselineSplineDesc:
            print("Error: %s: spline data mismatch" % self._testName)
            print("Baseline:\n%s" % self._baselineSplineDesc)
            print("Actual:\n%s" % splineDesc)
            return False

        # Precision should always match.
        if self._precision != self._baselinePrecision:
            print("Error: %s: precision mismatch; "
                  "baseline %d, actual %d"
                  & (self._testName, self._baselinePrecision, self._precision))
            return False

        # Should always have same number of samples.
        count = len(self._samples)
        baselineCount = len(self._baselineSamples)
        if count != baselineCount:
            print("Error: %s: sample count mismatch; "
                  "baseline %d, actual %d"
                  % (self._testName, baselineCount, count))
            return False

        # Sample times should always match.
        for i in range(baselineCount):

            lineNum = self._baselineSampleLineOffset + i
            base = self._baselineSamples[i]
            sample = self._samples[i]

            if not Gf.IsClose(sample.time, base.time, self._epsilon):
                print("Error: %s: time mismatch on line %d; "
                      "baseline %g, actual %g, diff %g"
                      % (self._fileName, lineNum,
                         base.time, sample.time,
                         abs(sample.time - base.time)))
                return False

        # All inputs match.
        return True

    def _ValidateValues(self):

        # Examine all samples for mismatches.
        mismatch = False
        for i in range(len(self._baselineSamples)):

            lineNum = self._baselineSampleLineOffset + i
            base = self._baselineSamples[i]
            sample = self._samples[i]

            if not Gf.IsClose(sample.value, base.value, self._epsilon):
                print("%s: value mismatch on line %d; "
                      "baseline %g, actual %g, diff %g"
                      % (self._fileName, lineNum,
                         base.value, sample.value,
                         abs(sample.value - base.value)))
                mismatch = True

        return not mismatch

    def _WriteCandidates(self):

        self._WriteBaseline()
        self._WriteSingleGraph()

    def _WriteBaseline(self):

        with open(self._fileName, "w") as outfile:

            # Write prologue with input data description.
            print(self._splineData.GetDebugDescription(),
                  file = outfile, end = "")
            print("-----", file = outfile)

            # Write samples, one per line.  Each is a time and a value.
            for s in self._samples:
                print("%.*f %.*f"
                      % (self._precision, s.time, self._precision, s.value),
                      file = outfile)

        print("Wrote baseline candidate %s" % self._fileName)

    def _ReadBaseline(self, infile, path):

        lineNum = 0
        readingSamples = False

        # Read lines.
        for line in infile:

            lineNum += 1

            # Read prologue containing spline input data description.
            if not readingSamples:
                if line.strip() == "-----":
                    readingSamples = True
                    self._baselineSampleLineOffset = lineNum + 1
                else:
                    self._baselineSplineDesc += line
                continue

            # Parse sample lines.  Each is a time and a value.
            match = re.search(r"^(-?\d+\.(\d+)) (-?\d+\.(\d+))$", line)
            if not match:
                print(
                    "Error: %s: unexpected format on line %d"
                    % (path, lineNum))
                return False
            timeStr, timeFracStr, valStr, valFracStr = match.groups()

            # Verify consistent baseline precision.
            timePrecision = len(timeFracStr)
            valPrecision = len(valFracStr)
            if not self._baselinePrecision:
                self._baselinePrecision = valPrecision
            if valPrecision != timePrecision \
                    or valPrecision != self._baselinePrecision:
                print("Error: %s: inconsistent precision" % path)
                return False

            # Append a new in-memory sample for each line.
            sample = Ts.TsTest_Sample()
            sample.time = float(timeStr)
            sample.value = float(valStr)
            self._baselineSamples.append(sample)

        return True

    def _WriteSingleGraph(self):

        if not TsTest_Grapher.Init():
            return

        grapher = TsTest_Grapher(
            title = self._testName,
            widthPx = 1000, heightPx = 750)
        grapher.AddSpline(self._testName, self._splineData, self._samples)

        graphFileName = "%s_TsTestGraph.png" % self._testName
        grapher.Write(graphFileName)
        print("Wrote candidate graph %s" % graphFileName)

    def _WriteDiffGraph(self):

        if not TsTest_Comparator.Init():
            return

        comparator = TsTest_Comparator(
            title = self._testName,
            widthPx = 1000, heightPx = 1500)
        comparator.AddSpline(
            "Baseline", self._splineData, self._baselineSamples)
        comparator.AddSpline(
            "Actual", self._splineData, self._samples)

        graphFileName = "%s_TsTestDiff.png" % self._testName
        comparator.Write(graphFileName)
        print("Wrote diff graph %s" % graphFileName)
