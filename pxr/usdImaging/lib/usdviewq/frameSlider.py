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

    def mousePressEvent(self, event):
        # If the slider has no range, or the event button isn't valid,
        # ignore the event.
        if (self.maximum() == self.minimum() or
                event.buttons() ^ event.button()):
            event.ignore()
            return

        event.accept()

        if event.button() == QtCore.Qt.LeftButton:
            # Set the slider down property.
            # This tells the slider to obey the tracking state.
            self.setSliderDown(True)
            # Get the slider value from the event position.
            value = QtGui.QStyle.sliderValueFromPosition(
                self.minimum(), self.maximum(), event.x(), self.width()
            )
            # Set the slider value
            self.setSliderPosition(value)

    def mouseMoveEvent(self, event):
        # If the slider isn't currently being pressed, ignore the event.
        if not self.isSliderDown():
            event.ignore()
            return

        event.accept()

        # Get the slider value from the event position.
        value = QtGui.QStyle.sliderValueFromPosition(
            self.minimum(), self.maximum(), event.x(), self.width()
        )
        # Set the slider value.
        self.setSliderPosition(value)

    def mouseReleaseEvent(self, event):
        # If the slider isn't currently being pressed, ignore the event.
        if (not self.isSliderDown()) or event.buttons():
            event.ignore()
            return

        event.accept()

        # Unset the slider down property.
        self.setSliderDown(False)
