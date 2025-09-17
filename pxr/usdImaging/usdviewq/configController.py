#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from __future__ import print_function

import os, subprocess, sys
from functools import partial

from .qt import QtWidgets

class ConfigController:
    def __init__(self, currentConfig, appController):
        self._appController = appController
        self._currentConfig = currentConfig

        # XXX Temporarily hiding behind env variable until ready for public
        if os.getenv('USDVIEWQ_CONFIG_CONTROLLER') is None:
            self._hide(self._appController._ui)
        else:
            self.reloadConfigController()

    def _hide(self, ui):
        ui.menuLoad_New_State.menuAction().setVisible(False)
        ui.actionSave_State_To.setVisible(False)
        ui.menuSave_State_As.menuAction().setVisible(False)
        ui.actionSave_State_As_New_Config.setVisible(False)

    def reloadConfigController(self):
        """Can be used for refreshing UI if current config changes"""
        ui = self._appController._ui
        configs = self._appController._configManager.getConfigs()[1:]

        if configs:
            for config in configs:
                ui.menuLoad_New_State.addAction(config).triggered.connect(
                    partial(subprocess.Popen, [
                        sys.argv[0],
                        self._appController._parserData.usdFile,
                        '--config', config]))
            ui.menuLoad_New_State.menuAction().setVisible(True)
        else:
            ui.menuLoad_New_State.menuAction().setVisible(False)

        save = ui.actionSave_State_To
        if self._currentConfig:
            save.setText(save.text() + self._currentConfig)
            save.triggered.connect(self._appController._configManager.save)
            save.setVisible(True)
        else:
            save.setVisible(False)

        if configs:
            for config in configs:
                ui.menuSave_State_As.addAction(config).triggered.connect(
                    partial(self._appController._configManager.save, config))
            ui.menuSave_State_As.menuAction().setVisible(True)
        else:
            ui.menuSave_State_As.menuAction().setVisible(False)

        ui.actionSave_State_As_New_Config.triggered.connect(
            lambda: self._saveAsTriggered())

        # XXX When no longer hiding controller behind env variable, move this
        # separator to the static ui file
        ui.menuFile.insertSeparator(ui.actionQuit)

    def _validateAndSaveConfig(self, newName, dialog):
        if not newName:
            print("Invalid config name, not saving", file=sys.stderr)
            return
        self._appController._configManager.save(newName)
        dialog.close()

    def _saveAsTriggered(self):
        configDialog = QtWidgets.QDialog(self._appController._mainWindow)
        configDialog.setWindowTitle("Save State As")

        layout = QtWidgets.QHBoxLayout()
        field = QtWidgets.QLineEdit(self._currentConfig)
        field.textEdited.connect(lambda text: field.setText(text.lower()))
        layout.addWidget(field)

        fieldsLayout = QtWidgets.QVBoxLayout()
        fieldsLayout.addWidget(QtWidgets.QLabel(
            "Config Name"))
        fieldsLayout.addLayout(layout)
        fieldsLayout.addStretch()

        buttonBox = QtWidgets.QDialogButtonBox(
            QtWidgets.QDialogButtonBox.Cancel | QtWidgets.QDialogButtonBox.Save)
        buttonBox.rejected.connect(configDialog.close)
        buttonBox.accepted.connect(lambda :
            self._validateAndSaveConfig(field.text(), configDialog))

        configDialogLayout = QtWidgets.QVBoxLayout()
        configDialogLayout.addLayout(fieldsLayout)
        configDialogLayout.addWidget(buttonBox)
        configDialog.setLayout(configDialogLayout)

        configDialog.open()