#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.qt import QtWidgets
from pxr.Usdviewq.common import SelectionHighlightModes

def _waitForRefresh():
    import time
    time.sleep(0.5)
    QtWidgets.QApplication.processEvents()

# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False
    appController._dataModel.viewSettings.selHighlightMode = (
        SelectionHighlightModes.NEVER)


#
# Test a case where we move an instanced root that contains strictly
# non-instancable-by-Hydra prims (i.e., no rprims).
#
def _testInstancingEdits6146(appController):
    from pxr import Sdf, Usd

    testALayer = Sdf.Layer.FindOrOpen("usd-6146/testA.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testALayer)
    _waitForRefresh()

    testBLayer = Sdf.Layer.FindOrOpen("usd-6146/testB.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testBLayer)
    _waitForRefresh()

    # If we get this far without crashing, we're good for now.

#
# Test a case where we deactivate the parent prim of a native instance.
#   
def _testDeactivatingInstanceParent11237(appController):
    from pxr import Sdf, Usd

    testLayer = Sdf.Layer.FindOrOpen("usd-11237/instanceWithParent.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testLayer)
    appController._takeShot("instanceWithParent.png")

    instance = appController._dataModel.stage.GetPrimAtPath("/World/Parent")
    instance.SetActive(False)
    appController._takeShot("instanceWithParentDeactivated.png")

#
# Tests where we force a resync by changing subLayerPaths in a shot that has
# native instances.
#
def _testCompleteResyncWithNativeInstances(appController):
    from pxr import Sdf, Usd

    appController._dataModel.stage.GetRootLayer().Clear()

    appController._dataModel.stage.GetRootLayer().subLayerPaths = ["usd-11280/skel_1.usda"]
    appController._dataModel._viewSettingsDataModel.cameraPath = Sdf.Path('/main_cam')
    appController._takeShot("completeResyncWithNativeInstances1.png")

    appController._dataModel.stage.GetRootLayer().subLayerPaths = ["usd-11280/skel_2.usda"]
    appController._dataModel._viewSettingsDataModel.cameraPath = Sdf.Path('/main_cam')
    appController._takeShot("completeResyncWithNativeInstances2.png")

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testInstancingEdits6146(appController)
    _testDeactivatingInstanceParent11237(appController)
    _testCompleteResyncWithNativeInstances(appController)
