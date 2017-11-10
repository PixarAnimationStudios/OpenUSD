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

INSTANCER_PATH = "/Foo/Cube/Instancer"
FOO_PATH = "/Foo"
REPLACE_MODE = "replace"

# Remove any unwanted visuals from the view and set complexity.
def _modifySettings(appController):
    appController.showBBoxes = False
    appController.showHUD = False

# Select a pick mode and update the view.
def _setPickModeAction(appController, action):
    action.setChecked(True)
    appController._changePickMode(action)

# Check that the current selection is the expected selection.
def _checkPrimSelection(appController, path):
    primSelection = appController._currentPrims
    assert len(primSelection) == 1
    assert primSelection[0].GetPath() == path

# Check that no instances are selected.
def _checkNoInstancesSelected(appController, path):
    instanceSelection = appController._stageView._selectedInstances
    assert len(instanceSelection) == 0

# Check that the specified instance is the only selected instance.
def _checkInstanceSelection(appController, path, instanceIndex):
    instanceSelection = appController._stageView._selectedInstances
    assert len(instanceSelection) == 1
    assert path in instanceSelection
    instanceIndices = instanceSelection[path]
    assert len(instanceIndices) == 1
    assert instanceIndex in instanceIndices

# Test picking a prim.
def _testPickPrims(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Prims)
    appController.selectPrimByPath(INSTANCER_PATH, 0, REPLACE_MODE, True)

    _checkPrimSelection(appController, INSTANCER_PATH)
    _checkNoInstancesSelected(appController, INSTANCER_PATH)

# Test picking a model.
def _testPickModels(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Models)
    appController.selectPrimByPath(INSTANCER_PATH, 0, REPLACE_MODE, True)

    _checkPrimSelection(appController, FOO_PATH)
    _checkNoInstancesSelected(appController, INSTANCER_PATH)

# Test picking an instance.
def _testPickInstances(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Instances)
    appController.selectPrimByPath(INSTANCER_PATH, 0, REPLACE_MODE, True)

    _checkPrimSelection(appController, INSTANCER_PATH)
    _checkInstanceSelection(appController, INSTANCER_PATH, 0)

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testPickPrims(appController)
    _testPickModels(appController)
    _testPickInstances(appController)
