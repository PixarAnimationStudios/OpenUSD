#!/pxrpythonsubst
#
# Copyright 2020 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr.Usdviewq.qt import QtWidgets
from pxr.Usdviewq.common import SelectionHighlightModes
from pxr import UsdGeom

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
    from pxr import Sdf

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
    from pxr import Sdf

    testLayer = Sdf.Layer.FindOrOpen("usd-11237/instanceWithParent.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testLayer)
    appController._takeShot("instanceWithParent.png")

    instance = appController._dataModel.stage.GetPrimAtPath("/World/Parent")
    instance.SetActive(False)
    appController._takeShot("instanceWithParentDeactivated.png")

#
# Tests whether visibility authored on a PointInstancer is respected.
#
def _testInstancerVisibilityEdits(appController):
    from pxr import Sdf

    testLayer = Sdf.Layer.FindOrOpen("usd-11149/instancerVisibility.usda")
    appController._dataModel.stage.GetRootLayer().TransferContent(testLayer)
    appController._takeShot("instancerInvisible.png")

    vis = appController._dataModel.stage.GetPropertyAtPath("/PointInstancer.visibility")
    vis.Set(UsdGeom.Tokens.inherited)
    appController._takeShot("instancerVisible.png")

# Tests that when a prim goes from being a "prototype" to no longer, we image
# it properly.
# 
# Since we will sometimes use a type-less "over" to store prototypes, that is
# included here.
def _testMakePrototypeNotAPrototype(appController):
    from pxr import Sdf

    # The scene has 4 cubes:
    # "left", "Over", "PointInstancer", "right"
    stageRootLayer = appController._dataModel.stage.GetRootLayer()
    stageRootLayer.TransferContent(Sdf.Layer.FindOrOpen("usd-12098/test.usda"))

    # This image should contain just the other "left" and "right" cubes.
    # The cubes under the "Over" and "PointInstancer" prims do not draw
    # since they are considered prototypes (without any instances).
    appController._takeShot("unprototyping-before.png")

    # Turn the over to a def.  This should result in the prim underneath
    # getting added.
    stageRootLayer.GetPrimAtPath("/Over").specifier = Sdf.SpecifierDef

    # Turn the prototype into a Scope.  Ensure it's children get imaged.
    stageRootLayer.GetPrimAtPath("/PointInstancer").typeName = "Scope"

    # This image should contain all 4 cubes.
    appController._takeShot("unprototyping-after.png")


#
# Tests where we force a resync by changing subLayerPaths in a shot that has
# native instances.
#
def _testCompleteResyncWithNativeInstances(appController):
    from pxr import Sdf

    appController._dataModel.stage.GetRootLayer().Clear()

    appController._dataModel.stage.GetRootLayer().subLayerPaths = ["usd-11280/skel_1.usda"]
    appController._dataModel._viewSettingsDataModel.cameraPath = Sdf.Path('/main_cam')
    appController._takeShot("completeResyncWithNativeInstances1.png")

    appController._dataModel.stage.GetRootLayer().subLayerPaths = ["usd-11280/skel_2.usda"]
    appController._dataModel._viewSettingsDataModel.cameraPath = Sdf.Path('/main_cam')
    appController._takeShot("completeResyncWithNativeInstances2.png")

#
# Tests whether making prim instanceable does not cause duplicate due to
# https://github.com/PixarAnimationStudios/OpenUSD/issues/3563.
#
def _testMakePrimInstanceable(appController):

    appController._dataModel.stage.GetRootLayer().Clear()

    appController._dataModel.stage.GetRootLayer().subLayerPaths = ["usd-10752/cube.usda"]
    appController._takeShot("primNotInstanceable.png")

    appController._dataModel.stage.GetPrimAtPath('/instance').SetInstanceable(True)
    appController._takeShot("primInstanceable.png")

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testInstancingEdits6146(appController)
    _testDeactivatingInstanceParent11237(appController)
    _testInstancerVisibilityEdits(appController)
    _testMakePrimInstanceable(appController)
    _testMakePrototypeNotAPrototype(appController)
    # Last since it changes the camera.
    _testCompleteResyncWithNativeInstances(appController)
