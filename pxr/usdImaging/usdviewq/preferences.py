#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
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
