#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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

# Leaving these in for future debugging
from __future__ import print_function
import sys

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

STAGE_START_TIME = 1
STAGE_END_TIME = 10
PRIM_PATH = 'myPrim'
PROP_NAME = 'myProp'

def _setup(appController):
    # ensure that the value tab is selected.
    appController._ui.attributeInspector.setCurrentIndex(0)
    appController._mainWindow.repaint()

    # select our property object
    appController._ui.primViewLineEdit.setText(PRIM_PATH)
    appController._primViewFindNext()
    appController._mainWindow.repaint()

    appController._ui.attrViewLineEdit.setText(PROP_NAME)
    appController._attrViewFindNext()
    appController._mainWindow.repaint()

def _collectRangeValues(appController):
    forwardControl  = appController._ui.actionFrame_Forward.triggered
    currentFrame = appController._ui.frameField
    valueTab = appController._ui.attributeValueEditor._ui.valueViewer
    getCurrentValue = lambda: float(str(valueTab.toPlainText()))
    resultingTimes = [getCurrentValue()]

    for i in xrange(STAGE_START_TIME, STAGE_END_TIME):
        forwardControl.emit()
        appController._mainWindow.repaint()
        resultingTimes.append(getCurrentValue())

    # Go forward once more to reset to the initial time
    forwardControl.emit()
    appController._mainWindow.repaint()

    return resultingTimes

def _toggleLinearInterpolationOn(appController, value):
    checkbox = appController._ui.linearInterpolationOn
    if value == checkbox.isChecked():
        return
    else:
        checkbox.toggle()

    appController._mainWindow.repaint()

def _testBasic(appController):
    # Set linear interpolation on, make sure values are representative of that
    _toggleLinearInterpolationOn(appController, True)
    expectedLinearValues = [1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0]
    assert _collectRangeValues(appController) == expectedLinearValues

    # Uncheck linear interpolation off, make sure values use held interpolation
    _toggleLinearInterpolationOn(appController, False)
    expectedHeldValues =  [1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 10.0]
    assert _collectRangeValues(appController) == expectedHeldValues 

def testUsdviewInputFunction(appController):
    _setup(appController)
    _testBasic(appController)
