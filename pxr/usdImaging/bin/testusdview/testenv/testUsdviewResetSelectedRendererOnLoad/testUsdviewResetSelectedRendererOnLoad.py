#!/pxrpythonsubst
#
# Copyright 2024 Pixar
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

from pxr import Sdf
from pxr.Usdviewq.qt import QtWidgets

def testUsdviewInputFunction(appController):
    _verifyCurrentRendererIsDefault(appController)

    numberOfRenderers = len(appController._stageView.GetRendererPlugins())

    if numberOfRenderers > 1:
        newRendererPlugin = appController._stageView.GetRendererPlugins()[1]
        appController._stageView.SetRendererPlugin(newRendererPlugin)

        _verifyCurrentRendererIsNotDefault(appController)
        _emitReopen_StageAction(appController)

        _verifyCurrentRendererIsDefault(appController)

def _verifyCurrentRendererIsDefault(appController):
    action = appController._ui.rendererPluginActionGroup.actions()[0]
    assert action.isChecked()

def _verifyCurrentRendererIsNotDefault(appController):
    action = appController._ui.rendererPluginActionGroup.actions()[0]
    assert not action.isChecked()

def _emitReopen_StageAction(appController):
    appController._ui.actionReopen_Stage.triggered.emit() 
    QtWidgets.QApplication.processEvents()