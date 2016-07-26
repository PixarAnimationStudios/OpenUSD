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
from PySide import QtGui, QtCore
from adjustClippingUI import Ui_AdjustClipping

class AdjustClipping(QtGui.QDialog):
    def __init__(self, parent):
        QtGui.QDialog.__init__(self,parent)
        self._ui = Ui_AdjustClipping()
        self._ui.setupUi(self)

        self._parent = parent
        self._viewer = parent._stageView
        clipRange = self._viewer.computeGfCamera().frustum.nearFar
        self._nearCache = self._viewer.overrideNear or clipRange.min
        self._farCache = self._viewer.overrideFar or clipRange.max

        self._refreshTimer = QtCore.QTimer(self)
        self._refreshTimer.setInterval(250)
        self._refreshTimer.start()

        # Connect timer
        QtCore.QObject.connect(self._refreshTimer, QtCore.SIGNAL('timeout()'),
                               self._updateAutomaticValues)

        # When the checkboxes change, we want to update instantly
        QtCore.QObject.connect(self._ui.overrideNear,
                               QtCore.SIGNAL('stateChanged(int)'),
                               self._overrideNearToggled)

        QtCore.QObject.connect(self._ui.overrideFar,
                               QtCore.SIGNAL('stateChanged(int)'),
                               self._overrideFarToggled)

        # we also want to update the clipping planes as the user is typing
        QtCore.QObject.connect(self._ui.nearEdit,
                               QtCore.SIGNAL('textChanged(QString)'),
                               self._nearChanged)

        QtCore.QObject.connect(self._ui.farEdit,
                               QtCore.SIGNAL('textChanged(QString)'),
                               self._farChanged)

        # uncheck the main window menu item when the window is closed
        QtCore.QObject.connect(self,
                               QtCore.SIGNAL('finished(int)'),
                               self._cleanUpAndClose)

        # Set the checkboxes to their initial state
        self._ui.overrideNear.setChecked(self._viewer.overrideNear \
                                             is not None)
        self._ui.overrideFar.setChecked(self._viewer.overrideFar \
                                            is not None)

        # load the initial values for the text boxes, but first deactivate them
        # if their corresponding checkbox is off.
        self._ui.nearEdit.setEnabled(self._ui.overrideNear.isChecked())
        self._ui.nearEdit.setText(str(self._nearCache))

        self._ui.farEdit.setEnabled(self._ui.overrideFar.isChecked())
        self._ui.farEdit.setText(str(self._farCache))

        # Make sure only doubles can be typed in the text boxes
        self._ui.nearEdit.setValidator(QtGui.QDoubleValidator(self))
        self._ui.farEdit.setValidator(QtGui.QDoubleValidator(self))

    def _updateAutomaticValues(self):
        """Read the automatically computed clipping planes and put them
           in the text boxes when they are deactivated"""
        clipRange = self._viewer.computeGfCamera().frustum.nearFar
        if (not self._ui.overrideNear.isChecked()) and \
                self._nearCache != clipRange.min :
            self._nearCache = clipRange.min
            self._ui.nearEdit.setText(str(self._nearCache))

        if (not self._ui.overrideFar.isChecked()) and \
                self._farCache != clipRange.max :
            self._farCache = clipRange.max
            self._ui.farEdit.setText(str(self._farCache))

    def _overrideNearToggled(self, state):
        """Called when the "Override Near" checkbox is toggled"""
        self._ui.nearEdit.setEnabled(state)
        if state:
            self._viewer.overrideNear = self._nearCache
        else:
            self._viewer.overrideNear = None

    def _overrideFarToggled(self, state):
        """Called when the "Override Far" checkbox is toggled"""
        self._ui.farEdit.setEnabled(state)
        if state:
            self._viewer.overrideFar = self._farCache
        else:
            self._viewer.overrideFar = None

    def _nearChanged(self, text):
        """Called when the Near text box changed.  This can happen when we
        are updating the value but the widget is actually inactive - don't
        do anything in that case."""
        if len(text) == 0 or not self._ui.nearEdit.isEnabled():
            return

        try:
            self._viewer.overrideNear = float(text)
        except ValueError:
            pass

    def _farChanged(self, text):
        """Called when the Far text box changed.  This can happen when we
        are updating the value but he widget is actually inactive - don't
        do anything in that case."""
        if len(text) == 0 or not self._ui.farEdit.isEnabled():
            return

        try:
            self._viewer.overrideFar = float(text)
        except ValueError:
            pass

    def _cleanUpAndClose(self, result):
        self._refreshTimer.stop()
        self._parent._ui.actionAdjust_Clipping.setChecked(False)
