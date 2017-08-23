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
from pyside import QtWidgets, QtCore
from adjustClippingUI import Ui_AdjustClipping

class AdjustClipping(QtWidgets.QDialog):
    """The dataModel provided to this VC must conform to the following
    interface:
    
    Editable properties:
       overrideNear (float or None, which indicates the override is disabled)
       overrideFar  (float or None, which indicates the override is disabled)

    Readable properties:
       cameraFrustum (Gf.Frustum, or struct that has a Gf.Range1d 'nearFar' member)

    Signals:
       signalFrustumChanged() - whenever the near/far clipping values
                                may have changed.
    """
    def __init__(self, parent, dataModel):
        QtWidgets.QDialog.__init__(self,parent)
        self._ui = Ui_AdjustClipping()
        self._ui.setupUi(self)

        self._dataModel = dataModel
        clipRange = self._dataModel.cameraFrustum.nearFar
        self._nearCache = self._dataModel.overrideNear or clipRange.min
        self._farCache = self._dataModel.overrideFar or clipRange.max

        QtCore.QObject.connect(self._dataModel, 
                               QtCore.SIGNAL('signalFrustumChanged()'),
                               self.update)

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

        # Set the checkboxes to their initial state
        self._ui.overrideNear.setChecked(self._dataModel.overrideNear \
                                             is not None)
        self._ui.overrideFar.setChecked(self._dataModel.overrideFar \
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

    def _updateEditorsFromDataModel(self):
        """Read the dataModel-computed clipping planes and put them
           in the text boxes when they are deactivated."""
        clipRange = self._dataModel.cameraFrustum.nearFar
        if (not self._ui.overrideNear.isChecked()) and \
                self._nearCache != clipRange.min :
            self._nearCache = clipRange.min
            self._ui.nearEdit.setText(str(self._nearCache))

        if (not self._ui.overrideFar.isChecked()) and \
                self._farCache != clipRange.max :
            self._farCache = clipRange.max
            self._ui.farEdit.setText(str(self._farCache))

    def paintEvent(self, paintEvent):
        """Overridden from base class so we can perform JIT updating
        of editors to limit the number of redraws we perform"""
        self._updateEditorsFromDataModel()
        super(AdjustClipping, self).paintEvent(paintEvent)

    def _overrideNearToggled(self, state):
        """Called when the "Override Near" checkbox is toggled"""
        self._ui.nearEdit.setEnabled(state)
        if state:
            self._dataModel.overrideNear = self._nearCache
        else:
            self._dataModel.overrideNear = None

    def _overrideFarToggled(self, state):
        """Called when the "Override Far" checkbox is toggled"""
        self._ui.farEdit.setEnabled(state)
        if state:
            self._dataModel.overrideFar = self._farCache
        else:
            self._dataModel.overrideFar = None

    def _nearChanged(self, text):
        """Called when the Near text box changed.  This can happen when we
        are updating the value but the widget is actually inactive - don't
        do anything in that case."""
        if len(text) == 0 or not self._ui.nearEdit.isEnabled():
            return

        try:
            self._dataModel.overrideNear = float(text)
        except ValueError:
            pass

    def _farChanged(self, text):
        """Called when the Far text box changed.  This can happen when we
        are updating the value but he widget is actually inactive - don't
        do anything in that case."""
        if len(text) == 0 or not self._ui.farEdit.isEnabled():
            return

        try:
            self._dataModel.overrideFar = float(text)
        except ValueError:
            pass

    def closeEvent(self, event):
        # Ensure that even if the dialog doesn't get destroyed right away,
        # we'll stop doing work.
        QtCore.QObject.disconnect(self._dataModel, 
                                  QtCore.SIGNAL('signalFrustumChanged()'),
                                  self.update)

        event.accept()
        # Since the dialog is the immediate-edit kind, we consider
        # window-close to be an accept, so our clients can know the dialog is
        # done by listening for the finished(int) signal
        self.accept()
