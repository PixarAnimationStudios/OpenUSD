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
    """Custom QSlider class to allow scrubbing on left-click."""

    PAUSE_TIMER_INTERVAL = 500

    def __init__(self, parent=None):
        super(FrameSlider, self).__init__(parent=parent)
        # Create a mouse pause timer to trigger an update if the slider
        # scrubbing pauses.
        self._mousePauseTimer = QtCore.QTimer(self)
        self._mousePauseTimer.setInterval(self.PAUSE_TIMER_INTERVAL)
        self._mousePauseTimer.timeout.connect(self.mousePaused)

    def mousePaused(self):
        """Slot called when the slider scrubbing is paused."""
        self._mousePauseTimer.stop()
        self.valueChanged.emit(self.sliderPosition())

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            # Get the slider value from the event position.
            value = QtGui.QStyle.sliderValueFromPosition(
                self.minimum(), self.maximum(), event.x(), self.width()
            )
            # Set the slider value.
            self.setSliderPosition(value)

        super(FrameSlider, self).mousePressEvent(event)

    def mouseMoveEvent(self, event):
        super(FrameSlider, self).mouseMoveEvent(event)

        # Start the pause timer if tracking is disabled.
        if not self.hasTracking():
            self._mousePauseTimer.start()

    def mouseReleaseEvent(self, event):
        # Stop the pause timer.
        self._mousePauseTimer.stop()

        super(FrameSlider, self).mouseReleaseEvent(event)
