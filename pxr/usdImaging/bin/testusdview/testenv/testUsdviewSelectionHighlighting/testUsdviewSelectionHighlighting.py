#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Sdf
from pxr.Usdviewq.qt import QtWidgets

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    # processEvents needed so gui can dynamically resize due to viewer menu bar
    QtWidgets.QApplication.processEvents()

# Make a single selection.
def _testSingleSelection(appController):
    appController._dataModel.selection.setPrimPath("/backSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight.png", "PNG")

# Make a single selection with selection highlighting disabled.
def _testNoHighlightSelection(appController):
    appController._ui.actionNever.setChecked(True)
    appController._changeSelHighlightMode(appController._ui.actionNever)

    appController._dataModel.selection.setPrimPath("/backSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_none.png", "PNG")

    appController._ui.actionOnly_when_paused.setChecked(True)
    appController._changeSelHighlightMode(appController._ui.actionOnly_when_paused)

# Make two selections.
def _testDoubleSelection(appController):
    with appController._dataModel.selection.batchPrimChanges:
        appController._dataModel.selection.clearPrims()
        appController._dataModel.selection.addPrimPath("/backSphere")
        appController._dataModel.selection.addPrimPath("/frontSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_double.png", "PNG")

# Make two instance selections, one using authored instance ids and one using
# instance indices.
def _testInstanceSelection(appController):
    with appController._dataModel.selection.batchPrimChanges:
        appController._dataModel.selection.clearPrims()
        appController._dataModel.selection.addPrimPath("/Instancer", 1)
        appController._dataModel.selection.addPrimPath("/Instancer2", 6)
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_instance.png", "PNG")

# Make a single selection with a non-default selection color.
def _testColorSelection(appController):
    appController._ui.actionSelCyan.setChecked(True)
    appController._changeHighlightColor(appController._ui.actionSelCyan)

    appController._dataModel.selection.setPrimPath("/backSphere")
    QtWidgets.QApplication.processEvents()

    viewportShot = appController.GrabViewportShot()
    viewportShot.save("sel_highlight_color.png", "PNG")

    appController._ui.actionSelYellow.setChecked(True)
    appController._changeHighlightColor(appController._ui.actionSelYellow)

def _testNestedInstanceSelectionWithVisibility(appController):
    from pxr import Sdf

    testALayer = Sdf.Layer.FindOrOpen("USD-6687/nestedInstances.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testALayer)
    appController._dataModel._viewSettingsDataModel.cameraPath = Sdf.Path('/main_cam')

    appController._dataModel.selection.clearPrims()
    appController._dataModel.selection.addPrimPath("/root/CubesB/Cube2/Cube")
    appController._dataModel.selection.addPrimPath("/root/CubesC/Cube4/Cube")

    appController._takeShot("nestedInstanceSelection1.png")

    appController._dataModel.stage.GetPropertyAtPath("/root/CubesA.visibility").Set('invisible')
    appController._dataModel.stage.GetPropertyAtPath("/Cubes/Cube2.visibility").Set('invisible')


    appController._dataModel.selection.clearPrims()
    appController._dataModel.selection.addPrimPath("/root/CubesB/Cube2/Cube")
    appController._dataModel.selection.addPrimPath("/root/CubesC/Cube4/Cube")

    appController._takeShot("nestedInstanceSelection2.png")

    appController._dataModel.selection.clearPrims()
    appController._dataModel.selection.addPrimPath("/root/CubesC")

    appController._takeShot("nestedInstanceSelection3.png")

# Test that selection highlighting works properly in usdview
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testSingleSelection(appController)
    _testNoHighlightSelection(appController)
    _testDoubleSelection(appController)
    _testInstanceSelection(appController)
    _testColorSelection(appController)
    _testNestedInstanceSelectionWithVisibility(appController)
