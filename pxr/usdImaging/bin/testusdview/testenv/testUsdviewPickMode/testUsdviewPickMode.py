#!/pxrpythonsubst
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

from pxr import Sdf
from pxr.Usdviewq.selectionDataModel import ALL_INSTANCES
from pxr.Usdviewq.qt import QtCore

INSTANCER_PATH = "/Foo/Cube/Instancer"
FOO_PATH = "/Foo"

# Remove any unwanted visuals from the view and set complexity.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Select a pick mode and update the view.
def _setPickModeAction(appController, action):
    action.setChecked(True)
    appController._changePickMode(action)

# Check that the current selection is the expected selection.
def _checkPrimSelection(appController, path):
    primSelection = appController._dataModel.selection.getPrimPaths()
    assert len(primSelection) == 1
    assert primSelection[0] == path

# Check that no instances are selected.
def _checkNoInstancesSelected(appController, path):
    instanceSelection = appController._dataModel.selection.getPrimPathInstances()
    for instances in instanceSelection.values():
        assert instances == ALL_INSTANCES

# Check that the specified instance is the only selected instance.
def _checkInstanceSelection(appController, path, instance):
    path = Sdf.Path(str(path))
    instanceSelection = appController._dataModel.selection.getPrimPathInstances()
    assert len(instanceSelection) == 1
    assert path in instanceSelection
    instances = instanceSelection[path]
    assert len(instances) == 1
    assert instance in instances

# Test picking a prim.
def _testPickPrims(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Prims)
    pt = (0, 0, 0)
    appController.onPrimSelected(INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, INSTANCER_PATH)
    _checkNoInstancesSelected(appController, INSTANCER_PATH)

# Test picking a model.
def _testPickModels(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Models)
    pt = (0, 0, 0)
    appController.onPrimSelected(INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, FOO_PATH)
    _checkNoInstancesSelected(appController, INSTANCER_PATH)

# Test picking an instance.
def _testPickInstances(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Instances)
    pt = (0, 0, 0)
    appController.onPrimSelected(INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, INSTANCER_PATH)
    _checkInstanceSelection(appController, INSTANCER_PATH, 0)

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testPickPrims(appController)
    _testPickModels(appController)
    _testPickInstances(appController)
