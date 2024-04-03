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
