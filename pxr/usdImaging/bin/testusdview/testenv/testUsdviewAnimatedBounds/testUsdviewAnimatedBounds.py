#!/pxrpythonsubst
#
# Copyright 2020 Pixar
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

NUM_TIME_SAMPLES = 3
START_TIME_TEXT = "1.0"
END_TIME_TEXT = "3.0"

def _testBasic(appController):
    stageBegin   = appController._ui.stageBegin
    stageEnd     = appController._ui.stageEnd
    rangeBegin   = appController._ui.rangeBegin
    rangeEnd     = appController._ui.rangeEnd
    stepSize     = appController._ui.stepSize
    currentFrame = appController._ui.frameField

    forwardControl  = appController._ui.actionFrame_Forward.triggered

    # ensure our initial variable time samples are set.
    assert rangeBegin.text() == START_TIME_TEXT 
    assert rangeEnd.text() == END_TIME_TEXT 

    # ensure our static time samples are set.
    assert stageBegin.text() == START_TIME_TEXT
    assert stageEnd.text() == END_TIME_TEXT 

    def testIterateTimeRange(expectedRange, control):
        for i in expectedRange:
            assert currentFrame.text() == str(float(i))
            appController._takeShot("bbox_frame_%d.png" % i)
            control.emit()
            appController._mainWindow.repaint()

    # iterate through the default time samples by emitting our 
    # forward control signal.
    testIterateTimeRange([1, 2, 3], forwardControl)

def testUsdviewInputFunction(appController):
    _testBasic(appController)
