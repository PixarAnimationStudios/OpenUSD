#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Sdf
from pxr.Usdviewq.selectionDataModel import ALL_INSTANCES
from pxr.Usdviewq.qt import QtCore

INSTANCER_PATH = "/Foo/Cube/Instancer"
PROTO_PATH = "/Foo/Cube/Instancer/Protos/Proto1/cube"
FOO_PATH = "/Foo"
NI_PATH = "/Foo/Cube2"
INSTANCER2_PATH = "/Foo/Cube2/Instancer"
PROTO2_PATH = "/Foo/Cube2/Instancer/Protos/Proto1/cube"

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
    appController.onPrimSelected(PROTO_PATH, 0, INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, INSTANCER_PATH)
    _checkNoInstancesSelected(appController, INSTANCER_PATH)

# Test picking a model.
def _testPickModels(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Models)
    pt = (0, 0, 0)
    appController.onPrimSelected(PROTO_PATH, 0, INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, FOO_PATH)
    _checkNoInstancesSelected(appController, FOO_PATH)

# Test picking an instance.
def _testPickInstances(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Instances)

    # Pick an instance of a point instancer
    pt = (0, 0, 0)
    appController.onPrimSelected(PROTO_PATH, 0, INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, INSTANCER_PATH)
    _checkInstanceSelection(appController, INSTANCER_PATH, 0)

    # Pick an instance of a native instance
    appController.onPrimSelected(PROTO2_PATH, 0, INSTANCER2_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, NI_PATH)
    _checkNoInstancesSelected(appController, NI_PATH)

# Test picking a prototype.
def _testPickPrototypes(appController):
    _setPickModeAction(appController, appController._ui.actionPick_Prototypes)
    pt = (0, 0, 0)
    appController.onPrimSelected(PROTO_PATH, 0, INSTANCER_PATH, 0, pt, QtCore.Qt.LeftButton, 0)

    _checkPrimSelection(appController, PROTO_PATH)
    _checkInstanceSelection(appController, PROTO_PATH, 0)

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testPickPrims(appController)
    _testPickModels(appController)
    _testPickInstances(appController)
    _testPickPrototypes(appController)
