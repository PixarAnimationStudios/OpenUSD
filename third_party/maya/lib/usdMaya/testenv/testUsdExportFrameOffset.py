#!/pxrpythonsubst
#
# Copyright 2016 Pixar
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

import os
import unittest

from pxr import Usd

from maya import cmds
from maya import standalone


class testUsdExportFrameOffset(unittest.TestCase):

    @classmethod
    def tearDownClass(cls):
        standalone.uninitialize()

    @classmethod
    def setUpClass(cls):
        standalone.initialize('usd')
        cmds.file(os.path.abspath('UsdExportFrameOffsetTest.ma'), open=True,
                  force=True)

    def _ExportWithFrameSamplesAndStride(
            self, frameSamples=[], frameStride=1.0):
        # Export to USD.
        usdFilePath = os.path.abspath('UsdExportFrameOffsetTest.usda')
        cmds.loadPlugin('pxrUsd')
        cmds.usdExport(mergeTransformAndShape=True,
            file=usdFilePath,
            frameRange=(1, 10),
            frameSample=frameSamples,
            frameStride=frameStride)

        stage = Usd.Stage.Open(usdFilePath)
        self.assertTrue(stage)
        return stage

    def _AssertTimeSamples(self, stage, expectedSamples):
        cube = stage.GetPrimAtPath('/Cube')
        attr = cube.GetAttribute('xformOp:translate')
        timeSamples = attr.GetTimeSamples()
        self.assertEqual(timeSamples, expectedSamples)

    def testBasicFrameOffset(self):
        stage = self._ExportWithFrameSamplesAndStride([-0.1, 0.2])
        self._AssertTimeSamples(stage, [
            0.9, 1.2,
            1.9, 2.2,
            2.9, 3.2,
            3.9, 4.2,
            4.9, 5.2,
            5.9, 6.2,
            6.9, 7.2,
            7.9, 8.2,
            8.9, 9.2,
            9.9, 10.2
        ])

    def testReallyBigFrameOffset(self):
        stage = self._ExportWithFrameSamplesAndStride([0.0, 1.2])
        self._AssertTimeSamples(stage, sorted([
            1.0, 2.2,
            2.0, 3.2,
            3.0, 4.2,
            4.0, 5.2,
            5.0, 6.2,
            6.0, 7.2,
            7.0, 8.2,
            8.0, 9.2,
            9.0, 10.2,
            10.0, 11.2
        ]))

    def testRepeatedFrameOffset(self):
        stage = self._ExportWithFrameSamplesAndStride(
                [-0.1, 0.0, 0.1, 0.0, -0.1])
        self._AssertTimeSamples(stage, [
            0.9, 1.0, 1.1,
            1.9, 2.0, 2.1,
            2.9, 3.0, 3.1,
            3.9, 4.0, 4.1,
            4.9, 5.0, 5.1,
            5.9, 6.0, 6.1,
            6.9, 7.0, 7.1,
            7.9, 8.0, 8.1,
            8.9, 9.0, 9.1,
            9.9, 10.0, 10.1
        ])

    def testNoFrameOffset(self):
        stage = self._ExportWithFrameSamplesAndStride([])
        self._AssertTimeSamples(stage, [
            1.0, 
            2.0, 
            3.0, 
            4.0, 
            5.0, 
            6.0, 
            7.0, 
            8.0, 
            9.0,
            10.0
        ])

    def testFrameStrideSkips(self):
        """Tests frame stride > 1.0"""
        stage = self._ExportWithFrameSamplesAndStride(frameStride=3.0)
        self._AssertTimeSamples(stage, [1.0, 4.0, 7.0, 10.0])

    def testFrameStrideSkipsNonInteger(self):
        """Tests non-integer frame stride > 1.0"""
        stage = self._ExportWithFrameSamplesAndStride(frameStride=1.5)
        self._AssertTimeSamples(stage, [1.0, 2.5, 4.0, 5.5, 7.0, 8.5, 10.0])

    def testFrameStrideSubframes(self):
        """Tests frame stride < 1.0"""
        stage = self._ExportWithFrameSamplesAndStride(frameStride=0.25)
        self._AssertTimeSamples(stage, [
            1.0, 1.25, 1.5, 1.75,
            2.0, 2.25, 2.5, 2.75,
            3.0, 3.25, 3.5, 3.75,
            4.0, 4.25, 4.5, 4.75,
            5.0, 5.25, 5.5, 5.75,
            6.0, 6.25, 6.5, 6.75,
            7.0, 7.25, 7.5, 7.75,
            8.0, 8.25, 8.5, 8.75,
            9.0, 9.25, 9.5, 9.75,
            10.0,
        ])

    def testFrameSamplesAndStride(self):
        """Tests using both frameSample and frameStride."""
        stage = self._ExportWithFrameSamplesAndStride(
                frameSamples=[-0.1, 0.2], frameStride=0.5)
        self._AssertTimeSamples(stage, [
            0.9, 1.2,
            1.4, 1.7,
            1.9, 2.2,
            2.4, 2.7,
            2.9, 3.2,
            3.4, 3.7,
            3.9, 4.2,
            4.4, 4.7,
            4.9, 5.2,
            5.4, 5.7,
            5.9, 6.2,
            6.4, 6.7,
            6.9, 7.2,
            7.4, 7.7,
            7.9, 8.2,
            8.4, 8.7,
            8.9, 9.2,
            9.4, 9.7,
            9.9, 10.2,
        ])

if __name__ == '__main__':
    unittest.main(verbosity=2)
