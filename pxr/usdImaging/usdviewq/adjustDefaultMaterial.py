#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from .qt import QtCore, QtWidgets
from .adjustDefaultMaterialUI import Ui_AdjustDefaultMaterial

class AdjustDefaultMaterial(QtWidgets.QDialog):
    """Popup widget to adjust the default material used for rendering.
    `datamodel` should be a ViewSettingsDataModel.
    """

    def __init__(self, parent, dataModel):
        QtWidgets.QDialog.__init__(self,parent)
        self._ui = Ui_AdjustDefaultMaterial()
        self._ui.setupUi(self)

        self._dataModel = dataModel
        self._ambientCache = None
        self._specularCache = None

        self._ui.ambientIntSpinBox.valueChanged['double'].connect(self._ambientChanged)

        self._ui.specularIntSpinBox.valueChanged['double'].connect(self._specularChanged)

        dataModel.signalDefaultMaterialChanged.connect(self._updateFromData)

        self._ui.resetButton.clicked[bool].connect(self._reset)

        self._ui.doneButton.clicked[bool].connect(self._done)

        self._updateFromData()

    def _updateFromData(self):
        if self._dataModel.defaultMaterialAmbient != self._ambientCache:
            self._ambientCache = self._dataModel.defaultMaterialAmbient
            self._ui.ambientIntSpinBox.setValue(self._ambientCache)

        if self._dataModel.defaultMaterialSpecular != self._specularCache:
            self._specularCache = self._dataModel.defaultMaterialSpecular
            self._ui.specularIntSpinBox.setValue(self._specularCache)


    def _ambientChanged(self, val):
        if val != self._ambientCache:
            # Must do update cache first to prevent update cycle
            self._ambientCache = val
            self._dataModel.defaultMaterialAmbient = val

    def _specularChanged(self, val):
        if val != self._specularCache:
            # Must do update cache first to prevent update cycle
            self._specularCache = val
            self._dataModel.defaultMaterialSpecular = val

    def _reset(self, unused):
        self._dataModel.resetDefaultMaterial()

    def _done(self, unused):
        self.close()

    def closeEvent(self, event):
        event.accept()
        # Since the dialog is the immediate-edit kind, we consider
        # window-close to be an accept, so our clients can know the dialog is
        # done
        self.accept()
