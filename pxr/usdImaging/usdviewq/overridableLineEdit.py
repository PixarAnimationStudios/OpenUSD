#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import division
from .qt import QtCore, QtWidgets

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


