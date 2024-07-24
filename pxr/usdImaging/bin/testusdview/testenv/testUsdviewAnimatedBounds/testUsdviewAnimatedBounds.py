#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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

    frameForward  = appController._advanceFrame

    # ensure our initial variable time samples are set.
    assert rangeBegin.text() == START_TIME_TEXT 
    assert rangeEnd.text() == END_TIME_TEXT 

    # ensure our static time samples are set.
    assert stageBegin.text() == START_TIME_TEXT
    assert stageEnd.text() == END_TIME_TEXT 

    def testIterateTimeRange(expectedRange, timeFunction):
        for i in expectedRange:
            assert currentFrame.text() == str(float(i))
            appController._takeShot("bbox_frame_%d.png" % i)
            timeFunction()
            appController._mainWindow.repaint()

    # iterate through the default time samples by emitting our 
    # forward control signal.
    testIterateTimeRange([1, 2, 3], frameForward)

def testUsdviewInputFunction(appController):
    _testBasic(appController)
