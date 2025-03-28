#!/pxrpythonsubst
#
# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#


from __future__ import print_function
import sys
from pxr.Usdviewq.qt import QtWidgets


# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Select one or more prim paths, then set active state of those prims.
def _selectAndSetActive(appController, active, paths):
    selection = appController._dataModel.selection
    with selection.batchPrimChanges:
        selection.clearPrims()
        for path in paths:
            selection.addPrimPath(path)

    if active:
        appController.activateSelectedPrims()
        # We must processEvents after every call to activateSelectedPrims() so the
        # activated PrimViewItems can repopulate. (See _primViewUpdateTimer in
        # appController.py)
        QtWidgets.QApplication.processEvents()
    else:
        appController.deactivateSelectedPrims()

# Test deactivating then reactivating a single prim with no children.
def _testSingleDeactivate(appController):
    _selectAndSetActive(appController, False, ["/spheres/a"])
    appController._takeShot("singleDeactivate.png")
    _selectAndSetActive(appController, True, ["/spheres/a"])

# Test deactivating then reactivating a single prim with some children.
def _testParentDeactivate(appController):
    _selectAndSetActive(appController, False, ["/spheres"])
    appController._takeShot("parentDeactivate.png")
    _selectAndSetActive(appController, True, ["/spheres"])

# Test deactivating then reactivating a parent prim and one of its children.
def _testParentChildDeactivate(appController):
    _selectAndSetActive(appController, False, ["/spheres", "/spheres/a"])
    appController._takeShot("parentChildDeactivate1.png")

    # Reactivation is a two-part process because we must activate the parent
    # before we can even select the child. Take a snapshot in-between to verify
    # this is working.
    _selectAndSetActive(appController, True, ["/spheres"])
    appController._takeShot("parentChildDeactivate2.png")
    _selectAndSetActive(appController, True, ["/spheres/a"])

# In this case, the child prim has a shorter path than the parent due to a
# reference. If we deactivate the prims through Usd in sorted order where longer
# paths are deactivated first then this case fails.
def _testReferenceChildDeactivate(appController):
    _selectAndSetActive(appController, False, ["/C2/D", "/A/B/C"])

# Test that instance proxies cannot be deactivated. The call does not raise an
# error, but prints a warning and does not perform the deactivation.
def _testInstanceProxyDeactivate(appController):
    _selectAndSetActive(appController, False, ["/X/Y"])
    prim = appController._dataModel.stage.GetPrimAtPath("/X/Y")
    assert prim.IsActive() # Activation state should not have changed.

# Test that the complexity setting works properly in usdview.
def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _testSingleDeactivate(appController)
    _testParentDeactivate(appController)
    _testParentChildDeactivate(appController)
    _testReferenceChildDeactivate(appController)
    _testInstanceProxyDeactivate(appController)
