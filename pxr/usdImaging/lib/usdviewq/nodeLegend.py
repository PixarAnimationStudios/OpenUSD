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
from qt import QtCore, QtWidgets

from nodeLegendUI import Ui_NodeLegend

from common import (HasArcsColor, NormalColor, MasterColor, InstanceColor,
                    ColorizeLabelText, BoldenLabelText, ItalicizeLabelText)

class NodeLegend(QtWidgets.QWidget):
    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self._ui = Ui_NodeLegend()
        self._ui.setupUi(self)

        graphicsScene = QtWidgets.QGraphicsScene()

        # Set colors
        self._ui.nodeLegendColorHasArcs.setScene(graphicsScene)
        self._ui.nodeLegendColorNormal.setScene(graphicsScene)
        self._ui.nodeLegendColorInstance.setScene(graphicsScene)
        self._ui.nodeLegendColorMaster.setScene(graphicsScene)

        self._ui.nodeLegendColorHasArcs.setForegroundBrush(HasArcsColor)
        self._ui.nodeLegendColorNormal.setForegroundBrush(NormalColor)
        self._ui.nodeLegendColorInstance.setForegroundBrush(InstanceColor)
        self._ui.nodeLegendColorMaster.setForegroundBrush(MasterColor)

        legendTextUpdate = lambda t, c: (('<font color=\"%s\">' % c.color().name())
                                             + t.text() + '</font>') 
 
        normalLegend = self._ui.nodeLegendLabelNormal
        normalLegend.setText(legendTextUpdate(normalLegend, NormalColor))

        masterLegend = self._ui.nodeLegendLabelMaster
        masterLegend.setText(legendTextUpdate(masterLegend, MasterColor))

        instanceLegend = self._ui.nodeLegendLabelInstance
        instanceLegend.setText(legendTextUpdate(instanceLegend, InstanceColor))

        hasArcsLegend = self._ui.nodeLegendLabelHasArcs
        hasArcsLegend.setText(legendTextUpdate(hasArcsLegend, HasArcsColor))

        undefinedFontLegend = self._ui.nodeLegendLabelFontsUndefined
        undefinedFontLegend.setText(ItalicizeLabelText(undefinedFontLegend.text(), 
                                                       undefinedFontLegend.text()))

        definedFontLegend = self._ui.nodeLegendLabelFontsDefined
        definedFontLegend.setText(BoldenLabelText(definedFontLegend.text(), 
                                                  definedFontLegend.text()))

        # Set three individual colors in the text line to indicate
        # the dimmed version of each primary node color
        dimmedLegend = self._ui.nodeLegendLabelDimmed
        dimmedLegendText = dimmedLegend.text()
        dimmedLegendText = ColorizeLabelText(dimmedLegendText, "Dimmed colors", 148, 105, 30)
        dimmedLegendText = ColorizeLabelText(dimmedLegendText, "denote", 78,91,145)
        dimmedLegendText = ColorizeLabelText(dimmedLegendText, "inactive prims", 151,151,151)
        dimmedLegend.setText(dimmedLegendText)

