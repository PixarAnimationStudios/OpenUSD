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
from PySide import QtGui, QtCore
from adjustDefaultMaterialUI import Ui_AdjustDefaultMaterial

class AdjustDefaultMaterial(QtGui.QDialog):
    """The dataModel provided to this VC must conform to the following
    interface:
    
    Editable properties:
       defaultMaterialAmbient (float)
       defaultMaterialSpecular (float)

    Methods:
       ResetDefaultMaterialSettings()

    Signals:
       signalDefaultMaterialChanged() - when either property is
                                        set in the dataModel.
    """
    def __init__(self, parent, dataModel):
        QtGui.QDialog.__init__(self,parent)
        self._ui = Ui_AdjustDefaultMaterial()
        self._ui.setupUi(self)

        self._dataModel = dataModel
        self._ambientCache = None
        self._specularCache = None

        QtCore.QObject.connect(self._ui.ambientIntSpinBox,
                               QtCore.SIGNAL('valueChanged(double)'),
                               self._ambientChanged)

        QtCore.QObject.connect(self._ui.specularIntSpinBox,
                               QtCore.SIGNAL('valueChanged(double)'),
                               self._specularChanged)

        QtCore.QObject.connect(dataModel,
                               QtCore.SIGNAL('signalDefaultMaterialChanged()'),
                               self._updateFromData)

        QtCore.QObject.connect(self._ui.resetButton,
                               QtCore.SIGNAL('clicked(bool)'),
                               self._reset)

        QtCore.QObject.connect(self._ui.doneButton,
                               QtCore.SIGNAL('clicked(bool)'),
                               self._done)

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
        self._dataModel.ResetDefaultMaterialSettings()

    def _done(self, unused):
        self.close()

    def closeEvent(self, event):
        event.accept()
        # Since the dialog is the immediate-edit kind, we consider
        # window-close to be an accept, so our clients can know the dialog is
        # done
        self.accept()
