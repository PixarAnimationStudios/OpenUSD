#!/pxrpythonsubst
#
# Copyright 2018 Pixar
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


from __future__ import print_function
import sys
from pxr.Usdviewq.qt import QtWidgets


# Remove any unwanted visuals from the view.
def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

# Take a shot of the viewport and save it to a file.
def _takeShot(appController, fileName):

    QtWidgets.QApplication.processEvents()
    appController._mainWindow.update()
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

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
    _takeShot(appController, "singleDeactivate.png")
    _selectAndSetActive(appController, True, ["/spheres/a"])

# Test deactivating then reactivating a single prim with some children.
def _testParentDeactivate(appController):
    _selectAndSetActive(appController, False, ["/spheres"])
    _takeShot(appController, "parentDeactivate.png")
    _selectAndSetActive(appController, True, ["/spheres"])

# Test deactivating then reactivating a parent prim and one of its children.
def _testParentChildDeactivate(appController):
    _selectAndSetActive(appController, False, ["/spheres", "/spheres/a"])
    _takeShot(appController, "parentChildDeactivate1.png")

    # Reactivation is a two-part process because we must activate the parent
    # before we can even select the child. Take a snapshot in-between to verify
    # this is working.
    _selectAndSetActive(appController, True, ["/spheres"])
    _takeShot(appController, "parentChildDeactivate2.png")
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
