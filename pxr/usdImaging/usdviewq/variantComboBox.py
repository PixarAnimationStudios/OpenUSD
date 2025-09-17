#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from .qt import QtCore, QtGui, QtWidgets

from .common import Timer

class VariantComboBox(QtWidgets.QComboBox):
    def __init__(self, parent, prim, variantSetName, mainWindow):
        QtWidgets.QComboBox.__init__(self, parent)
        self.prim = prim
        self.variantSetName = variantSetName

    def updateVariantSelection(self, index, timer):
        variantSet = self.prim.GetVariantSet(self.variantSetName)
        currentVariantSelection = variantSet.GetVariantSelection()
        newVariantSelection = str(self.currentText())
        if currentVariantSelection != newVariantSelection:
            with timer("change variantSet %s to %s" %
                       (variantSet.GetName(), newVariantSelection)):
                variantSet.SetVariantSelection(newVariantSelection)

