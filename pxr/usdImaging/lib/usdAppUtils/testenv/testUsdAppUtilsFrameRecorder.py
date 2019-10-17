#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

from pxr import Usd
from pxr import UsdAppUtils
from pxr import UsdUtils

import os
import sys
import unittest


class TestUsdAppUtilsFrameRecorder(unittest.TestCase):

    @staticmethod
    def _SetupOpenGLContext():
        try:
            from PySide2 import QtOpenGL
            from PySide2.QtWidgets import QApplication
        except ImportError:
            from PySide import QtOpenGL
            from PySide.QtGui import QApplication

        application = QApplication(sys.argv)

        glFormat = QtOpenGL.QGLFormat()
        glFormat.setSampleBuffers(True)
        glFormat.setSamples(4)

        glWidget = QtOpenGL.QGLWidget(glFormat)
        glWidget.setFixedSize(640, 480)
        glWidget.show()
        glWidget.setHidden(True)

        return glWidget

    @classmethod
    def setUpClass(cls):
        cls._usdFilePath = os.path.abspath('AnimCube.usda')
        cls._stage = Usd.Stage.Open(cls._usdFilePath)
        cls._usdCamera = UsdAppUtils.GetCameraAtPath(cls._stage,
            UsdUtils.GetPrimaryCameraName())

        cls._glWidget = cls._SetupOpenGLContext()

        cls._frameRecorder = UsdAppUtils.FrameRecorder()
        cls._frameRecorder.SetColorCorrectionMode('sRGB')

    @classmethod
    def tearDownClass(cls):
        # Release our reference to the frame recorder so it can be deleted
        # before the Qt stuff.
        cls._frameRecorder = None

    def testStageOpens(self):
        """
        Tests that the USD stage was opened successfully.
        """
        self.assertTrue(self._stage)
        self.assertTrue(self._usdCamera)

    def testRecordSingleFrame(self):
        """
        Tests recording a single frame.
        """
        outputImagePath = os.path.abspath('AnimCube.png')
        self.assertTrue(
            self._frameRecorder.Record(self._stage, self._usdCamera,
                Usd.TimeCode.EarliestTime(), outputImagePath))

    def testRecordMultipleFrames(self):
        """
        Tests recording multiple frames.
        """
        outputImagePath = os.path.abspath('AnimCube.#.png')
        outputImagePath = UsdAppUtils.framesArgs.ConvertFramePlaceholderToFloatSpec(
            outputImagePath)

        frameSpecIter = UsdAppUtils.framesArgs.FrameSpecIterator('1,5,10')
        for timeCode in frameSpecIter:
            self.assertTrue(
                self._frameRecorder.Record(self._stage, self._usdCamera,
                    timeCode, outputImagePath.format(frame=timeCode.GetValue())))


if __name__ == "__main__":
    unittest.main()
