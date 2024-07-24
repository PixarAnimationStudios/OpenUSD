#!/pxrpythonsubst
#
# Copyright 2022 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr.Usdviewq.qt import QtWidgets

def _selectPrims(appController, paths):
    selection = appController._dataModel.selection
    with selection.batchPrimChanges:
        selection.clearPrims()
        for path in paths:
            selection.addPrimPath(path)

    appController.activateSelectedPrims()
    # We must processEvents after every call to activateSelectedPrims() so the
    # activated PrimViewItems can repopulate. (See _primViewUpdateTimer in
    # appController.py)
    QtWidgets.QApplication.processEvents()

def testUsdviewInputFunction(appController):
    _selectPrims(appController, ["/NoExtentButPoints"])
    _selectPrims(appController, ["/NoExtentNoPoints"])
    _selectPrims(appController, ["/NoExtentEmptyPoints"])
