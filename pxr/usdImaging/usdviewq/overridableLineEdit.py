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
from qt import QtCore, QtWidgets

# simple class to have a "clear" button on a line edit when the line edit
# contains an override. Clicking the clear button returns to the default value.

class OverridableLineEdit(QtWidgets.QLineEdit):
    def __init__(self, parent):
        QtWidgets.QLineEdit.__init__(self, parent)

        # create the clear button.
        self._clearButton = QtWidgets.QToolButton(self)
        self._clearButton.setText('x')
        self._clearButton.setCursor(QtCore.Qt.ArrowCursor)
        self._clearButton.setFixedSize(16, 16)
        self._clearButton.hide()
        self._defaultText = ''  # default value holder

        self._clearButton.clicked.connect(self._resetDefault)
        self.textEdited.connect(self._overrideSet)

    # properly place the button
    def resizeEvent(self, event):
        sz = QtCore.QSize(self._clearButton.size())
        frameWidth = self.style().pixelMetric(QtWidgets.QStyle.PM_DefaultFrameWidth)
        self._clearButton.move(self.rect().right() - frameWidth - sz.width(),
                               (self.rect().bottom() + 1 - sz.height())/2)

    # called when the user types an override
    def _overrideSet(self, text):
        self._clearButton.setVisible(True)

    # called programatically to reset the default text
    def setText(self, text):
        QtWidgets.QLineEdit.setText(self, text)
        self._defaultText = text

        self._clearButton.setVisible(False)

    # called when the clear button is clicked
    def _resetDefault(self):
        self.setText(self._defaultText)


