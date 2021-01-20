#!/pxrpythonsubst
#
# Copyright 2019 Pixar
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

# positions and names of our variants
CAPSULE = (1, 'capsule')
CONE = (2, 'cone')
CUBE = (3, 'cube')
CYLINDER = (4, 'cylinder')

VARIANT_INFO_POS = 0
VARIANT_INFO_NAME = 1

# Identifiers for variants in our stage
FIRST_VARIANT = 'a_shapeVariant'
SECOND_VARIANT = 'b_shapeVariant'

from pxr.Usdviewq.qt import QtWidgets

def _setupWidgets(appController):
    # Select our prim with the variant authored
    appController._ui.primViewLineEdit.setText('Shapes')
    appController._primViewFindNext()


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

def _manualSetActive(appController, active, path):
    prim = appController._dataModel.stage.GetPrimAtPath(path)
    assert prim
    prim.SetActive(active)
    QtWidgets.QApplication.processEvents()
        

# Select one or more prim paths, then load or unload those prims.
def _selectAndLoad(appController, load, paths):
    selection = appController._dataModel.selection
    with selection.batchPrimChanges:
        selection.clearPrims()
        for path in paths:
            selection.addPrimPath(path)

    if load:
        appController.loadSelectedPrims()
    else:
        appController.unloadSelectedPrims()
    # We must processEvents after every call to loadSelectedPrims() and
    # unloadSelectedPrims so the loaded PrimViewItems can repopulate. 
    # (See _primViewUpdateTimer in appController.py)
    QtWidgets.QApplication.processEvents()

def _getVariantSelector(appController, whichVariant):
    # Select the metadata tab in the lower right corner
    propertyInspector = appController._ui.propertyInspector
    propertyInspector.setCurrentIndex(1)

    # Grab the rows of our metadata tab and select the set containing
    # our variant selection
    metadataTable = propertyInspector.currentWidget().findChildren(QtWidgets.QTableWidget)[0]

    for i in range(0, metadataTable.rowCount()):
        currentName = metadataTable.item(i,0).text()
        if str(currentName).startswith(whichVariant):
            return metadataTable.cellWidget(i,1) 

    return None

def _selectVariant(appController, variantPos, whichVariant):
    selector = _getVariantSelector(appController, whichVariant)
    selector.setCurrentIndex(variantPos)

    # Variant selection changes the USD stage. Since all stage changes are
    # coalesced using the guiResetTimer, we need to process events here to give
    # the timer a chance to fire.
    QtWidgets.QApplication.processEvents()

# Get a list of all prims expanded in the prim treeview
def _getExpandedPrims(appController):
    return appController._getExpandedPrimViewPrims()

# Expand one or more prim paths
def _expandPrims(appController, paths):
    for path in paths:
        prim = appController._dataModel.stage.GetPrimAtPath(path)
        item = appController._primToItemMap.get(prim)
        item.setExpanded(True)
    
# Test that the expanded tree stays the same after deactivating, unloading, 
# and changing a variant set
def _testAllExpanded(appController):
    # select a variant
    _selectVariant(appController, CAPSULE[VARIANT_INFO_POS], FIRST_VARIANT)
    _expandPrims(appController, ["/spheres", "/A", "/A/B", "/A/B/C"])
    initialExpandedPrims = _getExpandedPrims(appController)
    _selectVariant(appController, CAPSULE[VARIANT_INFO_POS], FIRST_VARIANT)
    _selectVariant(appController, CONE[VARIANT_INFO_POS], FIRST_VARIANT)
    expandedPrims = _getExpandedPrims(appController)
    assert initialExpandedPrims == expandedPrims

    # deactivate and activate a prim
    _expandPrims(appController, ["/spheres", "/A", "/A/B", "/A/B/C"])
    initialExpandedPrims = _getExpandedPrims(appController)
    _selectAndSetActive(appController, False, ["/spheres/a"])
    _selectAndSetActive(appController, True, ["/spheres/a"])
    expandedPrims = _getExpandedPrims(appController)
    assert initialExpandedPrims == expandedPrims

    # load and then unload an unloaded prim
    _selectAndLoad(appController, False, ["/C2"])
    _expandPrims(appController, ["/spheres", "/A", "/A/B", "/A/B/C"])
    initialExpandedPrims = _getExpandedPrims(appController)
    _selectAndLoad(appController, True, ["/C2"])
    _selectAndLoad(appController, False, ["/C2"])
    expandedPrims = _getExpandedPrims(appController)
    assert initialExpandedPrims == expandedPrims
    
    # If we have a selected prim, and the stage mutates such that that prim 
    # no longer exists on the stage, it should be safely pruned from the 
    # selection, which should revert to the pseudoroot iff there are no other
    # remaining selected prims
    _expandPrims(appController, ["/spheres", "/A", "/A/B", "/A/B/C"])
    _selectAndSetActive(appController, True, ["/A/B/C"])
    assert appController._dataModel.selection.getPrimPaths() == ["/A/B/C"]
    # It's non-ideal that RuntimeError exceptions do not cause a test failure...
    # catch and fail explicitly.
    try:
        _manualSetActive(appController, False, "/A")
    except RuntimeError:
        assert False, "RuntimeError deactivating ancestor of selected prim"
    assert appController._dataModel.selection.getPrimPaths() == ["/"]
    
    # Now test with multiple prims selected
    _manualSetActive(appController, True, "/A")
    _expandPrims(appController, ["/spheres", "/A", "/A/B", "/A/B/C"])
    _selectAndSetActive(appController, True, ["/spheres", "/A/B/C"])
    assert appController._dataModel.selection.getPrimPaths() == ["/spheres", "/A/B/C"]
    try:
        _manualSetActive(appController, False, "/A")
    except RuntimeError:
        assert False, "RuntimeError deactivating ancestor of selected prim"
    assert appController._dataModel.selection.getPrimPaths() == ["/spheres"]
 

# Test tree expansion
def testUsdviewInputFunction(appController):
    _setupWidgets(appController)
    _testAllExpanded(appController)
