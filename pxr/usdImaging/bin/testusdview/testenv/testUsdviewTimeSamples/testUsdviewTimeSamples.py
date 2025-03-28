#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

NUM_TIME_SAMPLES = 6
START_TIME_TEXT = "101.0"
END_TIME_TEXT = "106.0"

def _testBasic(appController):
    stageBegin   = appController._ui.stageBegin
    stageEnd     = appController._ui.stageEnd
    rangeBegin   = appController._ui.rangeBegin
    rangeEnd     = appController._ui.rangeEnd
    stepSize     = appController._ui.stepSize
    currentFrame = appController._ui.frameField

    frameForward  = appController._advanceFrame
    frameBackward = appController._retreatFrame

    # ensure our initial variable time samples are set.
    assert rangeBegin.text() == START_TIME_TEXT 
    assert rangeEnd.text() == END_TIME_TEXT 

    # ensure our static time samples are set.
    assert stageBegin.text() == START_TIME_TEXT
    assert stageEnd.text() == END_TIME_TEXT 

    def testIterateTimeRange(expectedRange, timeFunction):
        for i in expectedRange:
            assert currentFrame.text() == str(float(i))
            timeFunction()
            appController._mainWindow.repaint()

    # iterate through the default time samples by emitting our 
    # forward control signal.
    testIterateTimeRange([101, 102, 103, 104, 105, 106], frameForward)

    # iteratoe through the default time samples by emitting our
    # backward control signal
    testIterateTimeRange([101, 106, 105, 104, 103, 102], frameBackward)
    
    # set the begin time and ensure it takes
    newBeginValue = "103.0"
    rangeBegin.setText(newBeginValue)
    rangeBegin.editingFinished.emit()
    appController._mainWindow.repaint()
    assert rangeBegin.text() == newBeginValue

    # set the end time and ensure it takes
    newEndValue = "105.0"
    rangeEnd.setText(newEndValue)
    rangeEnd.editingFinished.emit()
    appController._mainWindow.repaint()
    assert rangeEnd.text() == newEndValue

    # set the step size and ensure it takes
    newStepSize = "0.5"
    stepSize.setText(newStepSize) 
    stepSize.editingFinished.emit()
    appController._mainWindow.repaint()
    assert stepSize.text() == newStepSize

    # use newly set values 
    testIterateTimeRange([103.0, 103.5, 104.0, 104.5, 105.0], frameForward)
    testIterateTimeRange([103.0, 105.0, 104.5, 104.0, 103.5], frameBackward)

def testUsdviewInputFunction(appController):
    _testBasic(appController)
