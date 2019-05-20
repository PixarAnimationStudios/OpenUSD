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
from qt import QtCore, QtGui, QtWidgets

class FrameSlider(QtWidgets.QSlider):
    # Emitted when the current frame of the slider changes and the stage's 
    # current frame needs to be updated.
    signalFrameChanged = QtCore.Signal(int)

    # Emitted when the slider position has changed but the underlying frame 
    # value hasn't been changed.
    signalPositionChanged = QtCore.Signal(int)

    def __init__(self, parent):
        super(FrameSlider, self).__init__(parent)
        self._sliderTimer = QtCore.QTimer(self)
        self._sliderTimer.setInterval(500)
        self._sliderTimer.timeout.connect(self.sliderTimeout)     
        self.valueChanged.connect(self.sliderValueChanged)
        self._mousePressed = False
        self._scrubbing = False
        self._updateOnFrameScrub = False

    def setUpdateOnFrameScrub(self, updateOnFrameScrub):
        self._updateOnFrameScrub = updateOnFrameScrub

    def sliderTimeout(self):
        if not self._updateOnFrameScrub and self._mousePressed:
            self._sliderTimer.stop()
            self.signalPositionChanged.emit(self.value())
            return
        self.frameChanged()

    def frameChanged(self):
        self._sliderTimer.stop()
        self.signalFrameChanged.emit(self.value())

    def sliderValueChanged(self, value):
        self._sliderTimer.stop()
        self._sliderTimer.start()

    def setValueImmediate(self, value):
        self.setValue(value)
        self.frameChanged()

    def setValueFromEvent(self, event, immediate=True):
        currentValue = self.value()
        movePosition = self.minimum() + ((self.maximum()-self.minimum()) * 
            event.x()) / float(self.width())
        targetPosition = round(movePosition)
        if targetPosition == currentValue:
            if (movePosition - currentValue) >= 0:
                targetPosition = currentValue + 1
            else:
                targetPosition = currentValue - 1;
        if immediate:
            self.setValueImmediate(targetPosition)
        else:
            self.setValue(targetPosition)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._mousePressed = True
            self.setValueFromEvent(event)
            event.accept()
        super(FrameSlider, self).mousePressEvent(event)

    def mouseMoveEvent(self, event):
        # Since mouseTracking is disabled by default, this event callback is 
        # only invoked when a mouse is pressed down and moved (i.e. dragged or 
        # scrubbed).
        self._scrubbing = True
        self.setValueFromEvent(event, immediate=self._updateOnFrameScrub)
        event.accept()

    def mouseReleaseEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self._mousePressed = False
            # If this is just a click (and not a drag with mouse pressed), 
            # we don't want setValue twice for the same frame value.
            if self._scrubbing:
                self.setValueFromEvent(event)
            event.accept()
            self._scrubbing = False
        super(FrameSlider, self).mouseReleaseEvent(event)

    def advanceFrame(self):
        newValue = self.value() + 1
        if newValue > self.maximum():
            newValue = self.minimum()
        self.setValueImmediate(newValue)

    def retreatFrame(self):
        newValue = self.value() - 1
        if newValue < self.minimum():
            newValue = self.maximum()
        self.setValueImmediate(newValue)

    def resetSlider(self, numTimeSamples):
        self.setRange(0, numTimeSamples-1)
        self.resetToMinimum()

    def resetToMinimum(self):
        self.setValue(self.minimum())
        # Call this here to push the update immediately.
        self.frameChanged()
