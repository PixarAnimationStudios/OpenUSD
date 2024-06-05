#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from .qt import QtWidgets

from .primLegendUI import Ui_PrimLegend

from .common import (UIPrimTypeColors,
                     ColorizeLabelText, BoldenLabelText, ItalicizeLabelText)

class PrimLegend(QtWidgets.QWidget):
    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self._ui = Ui_PrimLegend()
        self._ui.setupUi(self)

        # start out in a minimized/hidden state
        self.setMaximumHeight(0)
        self._isMinimized = True

        graphicsScene = QtWidgets.QGraphicsScene()

        # Set colors
        self._ui.primLegendColorHasArcs.setScene(graphicsScene)
        self._ui.primLegendColorNormal.setScene(graphicsScene)
        self._ui.primLegendColorInstance.setScene(graphicsScene)
        self._ui.primLegendColorPrototype.setScene(graphicsScene)

        self._ui.primLegendColorHasArcs.setForegroundBrush(UIPrimTypeColors.HAS_ARCS)
        self._ui.primLegendColorNormal.setForegroundBrush(UIPrimTypeColors.NORMAL)
        self._ui.primLegendColorInstance.setForegroundBrush(UIPrimTypeColors.INSTANCE)
        self._ui.primLegendColorPrototype.setForegroundBrush(UIPrimTypeColors.PROTOTYPE)

        legendTextUpdate = lambda t, c: (('<font color=\"%s\">' % c.color().name())
                                             + t.text() + '</font>')

        normalLegend = self._ui.primLegendLabelNormal
        normalLegend.setText(legendTextUpdate(normalLegend, UIPrimTypeColors.NORMAL))

        prototypeLegend = self._ui.primLegendLabelPrototype
        prototypeLegend.setText(legendTextUpdate(prototypeLegend, UIPrimTypeColors.PROTOTYPE))

        instanceLegend = self._ui.primLegendLabelInstance
        instanceLegend.setText(legendTextUpdate(instanceLegend, UIPrimTypeColors.INSTANCE))

        hasArcsLegend = self._ui.primLegendLabelHasArcs
        hasArcsLegend.setText(legendTextUpdate(hasArcsLegend, UIPrimTypeColors.HAS_ARCS))

        undefinedFontLegend = self._ui.primLegendLabelFontsUndefined
        undefinedFontLegend.setText(ItalicizeLabelText(undefinedFontLegend.text(),
                                                       undefinedFontLegend.text()))

        definedFontLegend = self._ui.primLegendLabelFontsDefined
        definedFontLegend.setText(BoldenLabelText(definedFontLegend.text(),
                                                  definedFontLegend.text()))

        # Set three individual colors in the text line to indicate
        # the dimmed version of each primary prim color
        dimmedLegend = self._ui.primLegendLabelDimmed
        dimmedLegendText = dimmedLegend.text()
        dimmedLegendText = ColorizeLabelText(
            dimmedLegendText, "Dimmed colors", 148, 105, 30)
        dimmedLegendText = ColorizeLabelText(
            dimmedLegendText, "denote", 78, 91, 145)
        dimmedLegendText = ColorizeLabelText(
            dimmedLegendText, "inactive prims", 151, 151, 151)
        dimmedLegend.setText(dimmedLegendText)

    def IsMinimized(self):
        return self._isMinimized

    def ToggleMinimized(self):
        self._isMinimized = not self._isMinimized

    def GetHeight(self):
        return self.height()

    def GetResetHeight(self):
        return self.sizeHint().height()
