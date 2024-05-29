#
# Copyright 2023 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from .TsTest_Grapher import TsTest_Grapher

class TsTest_Comparator(object):

    @classmethod
    def Init(cls):
        return TsTest_Grapher.Init()

    def __init__(self, title, widthPx = 1000, heightPx = 1500):
        self._sampleSets = []
        self._grapher = TsTest_Grapher(title, widthPx, heightPx)
        self._haveCompared = False

    def AddSpline(self, name, splineData, samples, baked = None):
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

    def _Compare(self):

        if self._haveCompared:
            return

        self._FindDiffs()
        self._grapher.AddDiffData(self._diffs)
        self._haveCompared = True

    def _FindDiffs(self):

        self._diffs = []
        self._maxDiff = 0

        if len(self._sampleSets) != 2:
            raise Exception("Comparator: must call AddSpline exactly twice")

        if len(self._sampleSets[0]) != len(self._sampleSets[1]):
            raise Exception("Mismatched eval results")

        for i in range(len(self._sampleSets[0])):

            sample1 = self._sampleSets[0][i]
            sample2 = self._sampleSets[1][i]

            if sample2.time - sample1.time > 1e-4:
                raise Exception("Mismatched eval times at index %d" % i)

            diff = sample2.value - sample1.value
            self._maxDiff = max(self._maxDiff, abs(diff))
            self._diffs.append(TsTest_Grapher.Diff(sample1.time, diff))
