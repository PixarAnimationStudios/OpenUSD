#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from .TsTest_Grapher import TsTest_Grapher
from .TsTest_Comparator import TsTest_Comparator
from pxr import Ts, Gf
import sys, os, re, traceback, difflib


class TsTest_Baseliner:
    """
    A test helper class for comparing a spline against a baseline.  Splines may
    be compared in two modes:

    - The parameters may be compared.  This is for tests that verify that a
      generated spline's parameters match a baseline.

    - The samples may be compared.  This is for tests that start from a fixed
      spline, and verify that evaluation matches a baseline.

    In addition, a "creation string" may optionally be provided, and will be
    recorded in the baseline and compared against it.  This may be any string,
    but a typical use is to record the result of an operation that was used to
    generate the spline parameters.

    Precision is specified as a number of decimal digits to the right of the
    decimal point.

    When Validate is called, one of the following will occur:

    - If there is no baseline file yet, a candidate baseline file will be
      written to the test run directory, along with a graph made from the spline
      data and samples.  If the graph looks right, both of these files should be
      copied into the test source, such that they are installed into a
      "baseline" subdirectory of the test run directory.  (The graph isn't
      necessary for operation of the test, but is a useful reference for
      maintainers.)  Validate will return False in this case.

    - If a creation string was provided, and it differs from what is recorded in
      the baseline file (beyond the specified precision), the differences will
      be listed on stdout, candidate baseline files will be written, and
      Validate will return False.  If the change in creation string is expected,
      the diff should be inspected and the new baseline files installed.

    - If comparing parameters:

      - If the spline data differs from what is recorded in the baseline file
        (beyond the specified precision), the differences will be listed on
        stdout, candidate baseline files will be written, a graph of the
        differences will also be written, and Validate will return False.  If
        the change in parameters is expected, the diff graph should be inspected
        and the new baseline files installed.

    - If comparing samples:

      - If the spline data, sample times, or precision differ from those
        recorded in the baseline file, that is an error in the test setup; it is
        a difference in the test inputs rather than the outputs.  Candidate
        baseline files will be written, and Validate will return False.  If the
        inputs are being changed deliberately, the new baseline files should be
        inspected and installed.

      - If any sample values differ from the baseline values by more than the
        specified precision, all differing samples will be listed on stdout,
        candidate baseline files will be written, a graph of the differences
        will also be written, and Validate will return False.  If the change in
        output is expected, the diff graph should be inspected and the new
        baseline files installed.

    - Otherwise, no files will be written, and Validate will return True.
    """
    @staticmethod
    def CreateForParamCompare(testName, splineData, samples, precision = 6):
        """
        Create a TsTest_Baseliner to compare spline parameters.  Validate() will
        compare only the parameters.  Samples are required so that graphs can be
        created in failure conditions.
        """
        return TsTest_Baseliner(
            testName, splineData, samples, precision, compareSamples = False)

    @staticmethod
    def CreateForEvalCompare(testName, splineData, samples, precision = 6):
        """
        Create a TsTest_Baseliner to compare spline evaluation.  Validate() will
        first verify that parameters match, then compare the samples.
        """
        return TsTest_Baseliner(
            testName, splineData, samples, precision, compareSamples = True)

    class _Spline:

        def __init__(
                self, splineData = None, samples = None,
                name = "", debugDesc = ""):

            self.name = name
            self.splineData = splineData
            self.samples = samples or list()
            self.debugDesc = debugDesc

    def __init__(
            self, testName, splineData, samples, precision, compareSamples):
        """
        Private constructor.  Call a CreateFor* method instead.
        """
        self._testName = testName
        self._fileName = "%s_TsTestBaseline.txt" % testName
        self._singleGraphFileName = "%s_TsTestGraph.png" % testName
        self._diffGraphFileName = "%s_TsTestDiff.png" % testName
        self._precision = precision
        self._resultSpline = self._Spline(
            splineData, samples,
            debugDesc = splineData.GetDebugDescription(self._precision))
        self._epsilon = 10 ** -self._precision
        self._compareSamples = compareSamples

        # These are filled in by optional mutator methods.
        self._creationStr = ""
        self._referenceSplines = []

        # These are filled in by _ReadBaseline.
        self._baselineCreationStr = ""
        self._baselineSpline = self._Spline()
        self._baselineSampleLineOffset = 0
        self._baselinePrecision = 0

        # These are filled in by Validate helpers.
        self._newCreationStr = None
        self._newParamStr = None

    def SetCreationString(self, creationString):
        """
        Add a string that describes the result of creating the spline.
        Optional.  Will be baselined and diffed as part of validation.
        """
        self._creationStr = creationString

    def AddReferenceSpline(self, name, splineData, samples):
        """
        Add a spline to be graphed along with the baseline and result.
        Optional.  Does not affect validation.
        """
        self._referenceSplines.append(
            self._Spline(
                splineData, samples, name,
                splineData.GetDebugDescription(self._precision)))

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

        # If we have a creation string, compare it against the baseline.
        if self._creationStr and not self._ValidateCreationString():
            # Creation string mismatch.  Also diff params for the new candidate.
            self._ValidateParams()
            self._WriteCandidates()
            return False

        # If comparing parameters, just do that.
        if not self._compareSamples:
            if self._ValidateParams():
                self._WriteOptionals()
                return True
            else:
                self._WriteCandidates()
                self._WriteDiffGraph()
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
        self._WriteOptionals()
        return True

    def _ValidateCreationString(self):

        # Compare creation strings.  If they don't match, present a diff and
        # return False.
        self._newCreationStr = self._DiffString(
            self._baselineCreationStr, self._creationStr,
            "creation string")
        return not self._newCreationStr

    def _ValidateParams(self, errorIfDifferent = False):

        # Compare spline parameter descriptions.  If they don't match, present a
        # diff and return False.
        self._newParamStr = self._DiffString(
            self._baselineSpline.debugDesc, self._resultSpline.debugDesc,
            "spline data", errorIfDifferent)
        return not self._newParamStr

    def _ValidateInputs(self):

        # Input spline data should always match.
        if not self._ValidateParams(errorIfDifferent = True):
            return False

        # Precision should always match.
        if self._precision != self._baselinePrecision:
            print("Error: %s: precision mismatch; "
                  "baseline %d, actual %d"
                  & (self._testName, self._baselinePrecision, self._precision))
            return False

        # Should always have same number of samples.
        count = len(self._resultSpline.samples)
        baselineCount = len(self._baselineSpline.samples)
        if count != baselineCount:
            print("Error: %s: sample count mismatch; "
                  "baseline %d, actual %d"
                  % (self._testName, baselineCount, count))
            return False

        # Sample times should always match.
        for i in range(baselineCount):

            lineNum = self._baselineSampleLineOffset + i
            base = self._baselineSpline.samples[i]
            sample = self._resultSpline.samples[i]

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
        for i in range(len(self._baselineSpline.samples)):

            lineNum = self._baselineSampleLineOffset + i
            base = self._baselineSpline.samples[i]
            sample = self._resultSpline.samples[i]

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
        self._WriteSingleGraph(isCandidate = True)

    def _WriteOptionals(self):

        # Baseline graphs are optional; they help illustrate the baseline, but
        # aren't needed for automated verification.  If the baseline graph is
        # absent, generate it as a debugging aid.
        baselineGraphPath = os.path.join("baseline", self._singleGraphFileName)
        if not os.path.exists(baselineGraphPath):
            self._WriteSingleGraph(isCandidate = False)

    def _WriteBaseline(self):

        with open(self._fileName, "w") as outfile:

            # Section 1: creation string.  Optional, but section separator is
            # always written.
            creationStr = self._newCreationStr or self._creationStr
            if creationStr:
                print(creationStr.rstrip(), file = outfile)
            print("-----", file = outfile)

            # Section 2: input data description.
            paramStr = self._newParamStr or self._resultSpline.debugDesc
            print(paramStr, file = outfile, end = "")
            print("-----", file = outfile)

            # Section 3: input data repr.
            print(repr(self._resultSpline.splineData), file = outfile)
            print("-----", file = outfile)

            # Section 4: samples, one per line.  Each is a time and a value.
            for s in self._resultSpline.samples:
                print("%.*f %.*f"
                      % (self._precision, s.time, self._precision, s.value),
                      file = outfile)

        print("Wrote baseline candidate %s" % self._fileName)

    def _ReadBaseline(self, infile, path):

        lineNum = 0
        section = 1

        # Read lines.
        for line in infile:

            lineNum += 1

            # Handle section separators.
            if line.rstrip() == "-----":
                section += 1
                if section == 4:
                    self._baselineSampleLineOffset = lineNum + 1
                continue

            # Read creation string if present.
            if section == 1:
                self._baselineCreationStr += line
                continue

            # Read spline parameter description.
            if section == 2:
                self._baselineSpline.debugDesc += line
                continue

            # Read spline parameter repr.
            if section == 3:
                try:
                    self._baselineSpline.splineData = eval(line)
                except:
                    print("%s: Error: section 2 eval failure" % path)
                    traceback.print_exc(file = sys.stdout)
                    return False
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
            self._baselineSpline.samples.append(sample)

        if section != 4:
            print("Error: %s: found %d sections, expected 4" % (path, section))
            return False

        if bool(self._baselineCreationStr) != bool(self._creationStr):
            print("Error: %s: creation string: baseline %s, result %s"
                  % (path,
                     "present" if self._baselineCreationStr else "absent",
                     "present" if self._creationStr else "absent"))
            return False

        return True

    def _DiffString(
            self, baseline, result, fieldName, errorIfDifferent = False):
        """
        Parse baseline and result, both strings, to find decimal numbers.  If
        all non-numeric portions agree exactly, and all numeric portions agree
        within self._epsilon, return None.  Otherwise find a string to be used
        as a new candidate baseline.  If non-numeric portions do not agree, use
        the raw result string as the candidate.  If mismatches are only in
        numeric portions, use the baseline string, with substitutions from the
        result string for numbers that do not match within tolerance.  This
        latter string is a minimal change to the baseline that will cause all
        numbers to match within tolerance.  Print a diff from the baseline
        string to the candidate string on stdout, and return the candidate
        string to be written to a new candidate file.
        """
        # Break a string into atoms, each of which is either a decmial number or
        # something else.  Construct a "skeleton" string that uses placeholders
        # for all numbers.
        class _DiffData:
            def __init__(self, inStr):
                self.atoms = re.split(r"(-?(?:\d+\.?\d*|\.\d+))", inStr)
                self.skeleton = str.join(
                    "",
                    ["X" if re.search(r"^[-\d\.]+$", a) else a
                     for a in self.atoms])

        # Analyze the two strings.
        baselineData = _DiffData(baseline)
        resultData = _DiffData(result)

        # If the strings differ in other than numeric ways, consider that a
        # test-setup error, and use the raw result string as the new candidate.
        if resultData.skeleton != baselineData.skeleton:
            self._PrintDiff(
                "%s structure mismatch" % fieldName,
                baseline, result,
                isError = True)
            return result

        # Compare all numbers in the strings.
        mismatchIndices = []
        for i in range(len(baselineData.atoms)):
            if not baselineData.atoms[i].isdecimal():
                continue
            if not Gf.IsClose(
                    float(resultData.atoms[i]), float(baselineData.atoms[i]),
                    self._epsilon):
                mismatchIndices.append(i)

        # If all numbers match within tolerance, signal success.
        if not mismatchIndices:
            return None

        # Build the new candidate string, starting from the baseline, and
        # substituting any numeric results that don't match within tolerance.
        rhsStr = ""
        nextMatch = 0
        for index in mismatchIndices:
            rhsStr += str.join("", baselineData.atoms[nextMatch:index])
            rhsStr += resultData.atoms[index]
            nextMatch = index + 1
        rhsStr += str.join("", baselineData.atoms[nextMatch:])

        # Display a diff of the numeric differences.
        self._PrintDiff(
            "%s mismatch" % fieldName,
            baseline, rhsStr,
            errorIfDifferent)
        return rhsStr

    def _PrintDiff(self, desc, leftStr, rightStr, isError):
        """
        Print a description string, then a unified diff of leftStr and rightStr,
        with all lines as context.
        """
        leftLines = leftStr.splitlines(keepends = True)
        rightLines = rightStr.splitlines(keepends = True)

        if isError:
            print("Error: ", end = "")
        print("%s: %s" % (self._testName, desc))
        print()
        sys.stdout.writelines(
            difflib.unified_diff(
                leftLines, rightLines,
                fromfile = "Baseline", tofile = "Actual",
                n = len(leftLines) + len(rightLines)))
        print()

    def _WriteSingleGraph(self, isCandidate):

        if not TsTest_Grapher.Init():
            return

        grapher = TsTest_Grapher(title = self._testName)
        grapher.AddSpline(
            "Baseline",
            self._resultSpline.splineData,
            self._resultSpline.samples)

        # Add reference splines.  Offset the color indices, so that reference
        # splines use the same colors in the single and diff graphs, despite the
        # single graph having one spline fewer.
        for i in range(len(self._referenceSplines)):
            spline = self._referenceSplines[i]
            grapher.AddSpline(
                spline.name, spline.splineData, spline.samples,
                colorIndex = i + 2)

        grapher.Write(self._singleGraphFileName)
        print("Wrote %s graph %s" % (
            "candidate" if isCandidate else "baseline",
            self._singleGraphFileName))

    def _WriteDiffGraph(self):

        if not TsTest_Grapher.Init():
            return

        if self._compareSamples:
            # Diff samples with Comparator, showing sample diffs over time.
            grapher = TsTest_Comparator(title = self._testName)
        else:
            # Diff parameters with Grapher; sample diffs are irrelevant.
            grapher = TsTest_Grapher(title = self._testName)

        grapher.AddSpline(
            "Baseline",
            self._baselineSpline.splineData,
            self._baselineSpline.samples)
        grapher.AddSpline(
            "Actual",
            self._resultSpline.splineData,
            self._resultSpline.samples)
        for spline in self._referenceSplines:
            grapher.AddSpline(
                spline.name, spline.splineData, spline.samples)

        grapher.Write(self._diffGraphFileName)
        print("Wrote diff graph %s" % self._diffGraphFileName)
