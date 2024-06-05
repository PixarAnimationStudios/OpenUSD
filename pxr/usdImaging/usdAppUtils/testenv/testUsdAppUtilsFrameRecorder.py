#!/pxrpythonsubst
#
# Copyright 2019 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

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
            from PySide6.QtOpenGLWidgets import QOpenGLWidget
            from PySide6.QtGui import QSurfaceFormat
            from PySide6.QtWidgets import QApplication
            PySideModule = 'PySide6'
        except ImportError:
            from PySide2 import QtOpenGL
            from PySide2.QtWidgets import QApplication
            PySideModule = 'PySide2'

        application = QApplication(sys.argv)

        if PySideModule == 'PySide6':
            glFormat = QSurfaceFormat()
            glFormat.setSamples(4)
            glWidget = QOpenGLWidget()
            glWidget.setFormat(glFormat)
        else:
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
                self._stage.GetStartTimeCode(), outputImagePath))

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
                    timeCode, outputImagePath.format(frame=timeCode)))


if __name__ == "__main__":
    unittest.main()
