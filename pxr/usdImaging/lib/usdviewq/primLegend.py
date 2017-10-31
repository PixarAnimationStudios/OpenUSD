#
# Copyright 2017 Pixar
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
from qt import QtWidgets

from primLegendUI import Ui_PrimLegend

from common import (HasArcsColor, NormalColor, MasterColor, InstanceColor,
                    ColorizeLabelText, BoldenLabelText, ItalicizeLabelText)

class PrimLegend(QtWidgets.QWidget):
    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self._ui = Ui_PrimLegend()
        self._ui.setupUi(self)

        self._isMinimized = True

        graphicsScene = QtWidgets.QGraphicsScene()

        # Set colors
        self._ui.primLegendColorHasArcs.setScene(graphicsScene)
        self._ui.primLegendColorNormal.setScene(graphicsScene)
        self._ui.primLegendColorInstance.setScene(graphicsScene)
        self._ui.primLegendColorMaster.setScene(graphicsScene)

        self._ui.primLegendColorHasArcs.setForegroundBrush(HasArcsColor)
        self._ui.primLegendColorNormal.setForegroundBrush(NormalColor)
        self._ui.primLegendColorInstance.setForegroundBrush(InstanceColor)
        self._ui.primLegendColorMaster.setForegroundBrush(MasterColor)

        legendTextUpdate = lambda t, c: (('<font color=\"%s\">' % c.color().name())
                                             + t.text() + '</font>')

        normalLegend = self._ui.primLegendLabelNormal
        normalLegend.setText(legendTextUpdate(normalLegend, NormalColor))

        masterLegend = self._ui.primLegendLabelMaster
        masterLegend.setText(legendTextUpdate(masterLegend, MasterColor))

        instanceLegend = self._ui.primLegendLabelInstance
        instanceLegend.setText(legendTextUpdate(instanceLegend, InstanceColor))

        hasArcsLegend = self._ui.primLegendLabelHasArcs
        hasArcsLegend.setText(legendTextUpdate(hasArcsLegend, HasArcsColor))

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
        # This predefined height is determined by the elements that exist in
        # the primLegend. For more information see primLegendUI.ui
        return 120
