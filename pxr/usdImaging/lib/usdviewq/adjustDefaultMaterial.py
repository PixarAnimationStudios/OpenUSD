#
# Copyright 2017 Pixar
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
from adjustDefaultMaterialUI import Ui_AdjustDefaultMaterial

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
        self._dataModel.setDefaultMaterial(0.2, 0.1)

    def _done(self, unused):
        self.close()

    def closeEvent(self, event):
        event.accept()
        # Since the dialog is the immediate-edit kind, we consider
        # window-close to be an accept, so our clients can know the dialog is
        # done
        self.accept()
