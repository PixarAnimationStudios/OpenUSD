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
from .qt import QtCore, QtGui, QtWidgets
from .preferencesUI import Ui_Preferences
from .common import FixableDoubleValidator

class Preferences(QtWidgets.QDialog):
    """The dataModel provided to this VC must conform to the following
    interface:

    Editable properties:
       fontSize, int

    Readable properties:

    Signals:
       viewSettings.signalSettingChanged() - whenever any view setting 
                                             may have changed.
    """
    def __init__(self, parent, dataModel):
        super(Preferences, self).__init__(parent)
        self._ui = Ui_Preferences()
        self._ui.setupUi(self)

        self._dataModel = dataModel
        self._dataModel.viewSettings.signalSettingChanged.connect(self._updateEditorsFromDataModel)
        self._muteUpdates = False

        # When the checkboxes change, we want to update instantly
        self._ui.buttonBox.clicked.connect(self._buttonBoxButtonClicked)

        self._updateEditorsFromDataModel()

    def _updateEditorsFromDataModel(self):
        if self._muteUpdates:
            return
        self._ui.fontSizeSpinBox.setValue(self._dataModel.viewSettings.fontSize)
        self.update()


    def _apply(self):
        self._muteUpdates = True
        self._dataModel.viewSettings.fontSize = self._ui.fontSizeSpinBox.value()
        self._muteUpdates = False

    def _buttonBoxButtonClicked(self, button):
        role = self._ui.buttonBox.buttonRole(button)
        Roles = QtWidgets.QDialogButtonBox.ButtonRole
        if role == Roles.AcceptRole or role == Roles.ApplyRole:
            self._apply()
        if role == Roles.AcceptRole or role == Roles.RejectRole:
            self.close()
