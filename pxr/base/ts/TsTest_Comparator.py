#
# Copyright 2024 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from .TsTest_Grapher import TsTest_Grapher
from pxr.Ts import TsTest_SampleTimes

class TsTest_Comparator(object):

    @classmethod
    def Init(cls):
        return TsTest_Grapher.Init()

    def __init__(self, title, widthPx = 1000, heightPx = 1500):
        self._sampleSets = []
        self._grapher = TsTest_Grapher(title, widthPx, heightPx)
        self._haveCompared = False

    def AddSpline(self, name, splineData, samples, baked = None):
        """
        Must always add at least two splines.  The first two will be compared.
        Additional splines will be graphed, but will not participate in the
        diff.
        """
        self._sampleSets.append(samples)
        self._grapher.AddSpline(name, splineData, samples, baked)

    def Display(self):
        self._Compare()
        self._grapher.Display()

    def Write(self, filePath):
        self._Compare()
        self._grapher.Write(filePath)

    def GetMaxDiff(self):
        self._Compare()
        return self._maxDiff

    def GetMaxDiffSamples(self):
        self._Compare()
        return (self._maxDiffSampleTime, self._maxDiffSamples)

    def VerifyDiffs(self, label, tolerance):
        self._Compare()
        if self._maxDiff > tolerance:
            preStr = " (pre)" if self._maxDiffSampleTime.pre else ""
            print(f"{label}: max diff {self._maxDiff} "
                  f"exceeds {tolerance} "
                  f"at time {self._maxDiffSampleTime.time}{preStr}, "
                  f"values {self._maxDiffSamples[0].value} "
                  f"and {self._maxDiffSamples[1].value}")
            return False
        return True

    def _Compare(self):

        if self._haveCompared:
            return

        self._FindDiffs()
        self._grapher.AddDiffData(self._diffs)
        self._haveCompared = True

    def _FindDiffs(self):

        self._diffs = []
        self._maxDiff = 0

        if len(self._sampleSets) < 2:
            raise Exception("Comparator: must call AddSpline at least twice")

        if len(self._sampleSets[0]) != len(self._sampleSets[1]):
            raise Exception("Mismatched eval results")

        numSamples = len(self._sampleSets[0])
        for i in range(numSamples):

            sample1 = self._sampleSets[0][i]
            sample2 = self._sampleSets[1][i]

            if sample2.time - sample1.time > 1e-4:
                raise Exception("Mismatched eval times at index %d" % i)

            diff = sample2.value - sample1.value
            self._diffs.append(TsTest_Grapher.Diff(sample1.time, diff))

            absDiff = abs(diff)
            if absDiff > self._maxDiff:

                self._maxDiff = absDiff
                self._maxDiffSamples = (sample1, sample2)

                isPre = (i < numSamples - 1
                         and abs(self._sampleSets[0][i + 1].time - sample1.time)
                         < 1e-4)
                self._maxDiffSampleTime = \
                    TsTest_SampleTimes.SampleTime(sample1.time, isPre)
