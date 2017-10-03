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

from propertyLegendUI import Ui_PropertyLegend

from common import (HasArcsColor, NormalColor, MasterColor, InstanceColor, DefaultTextColor,
                    TimeSampleTextColor, NoValueTextColor, ValueClipsTextColor, RedColor,
                    FallbackTextColor, ColorizeLabelText, BoldenLabelText, ItalicizeLabelText,
                    ATTR_PLAIN_TYPE_ICON, ATTR_WITH_CONN_TYPE_ICON, REL_PLAIN_TYPE_ICON, 
                    REL_WITH_TARGET_TYPE_ICON, TARGET_TYPE_ICON, CONN_TYPE_ICON, CMP_TYPE_ICON)


class PropertyLegend(QtWidgets.QWidget):
    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self._ui = Ui_PropertyLegend()
        self._ui.setupUi(self)

        self._isMinimized = True
        self._iconDisplaySize = (16,16)

        graphicsScene = QtWidgets.QGraphicsScene()

        self._ui.propertyLegendColorFallback.setScene(graphicsScene)
        self._ui.propertyLegendColorDefault.setScene(graphicsScene)
        self._ui.propertyLegendColorTimeSample.setScene(graphicsScene)
        self._ui.propertyLegendColorNoValue.setScene(graphicsScene)
        self._ui.propertyLegendColorValueClips.setScene(graphicsScene)
        self._ui.propertyLegendColorCustom.setScene(graphicsScene)
        
        # set color of attribute viewer legend boxes
        self._ui.propertyLegendColorFallback.setForegroundBrush(FallbackTextColor)
        self._ui.propertyLegendColorDefault.setForegroundBrush(DefaultTextColor)
        self._ui.propertyLegendColorTimeSample.setForegroundBrush(TimeSampleTextColor)
        self._ui.propertyLegendColorNoValue.setForegroundBrush(NoValueTextColor)
        self._ui.propertyLegendColorValueClips.setForegroundBrush(ValueClipsTextColor)
        self._ui.propertyLegendColorCustom.setForegroundBrush(RedColor)

        # set color of attribute viewer text items
        legendTextUpdate = lambda t, c: (('<font color=\"%s\">' % c.color().name())
                                         + t.text() + '</font>') 
        timeSampleLegend = self._ui.propertyLegendLabelTimeSample
        timeSampleLegend.setText(legendTextUpdate(timeSampleLegend, TimeSampleTextColor))
        
        fallbackLegend = self._ui.propertyLegendLabelFallback
        fallbackLegend.setText(legendTextUpdate(fallbackLegend, FallbackTextColor))

        valueClipLegend = self._ui.propertyLegendLabelValueClips
        valueClipLegend.setText(legendTextUpdate(valueClipLegend, ValueClipsTextColor))

        noValueLegend = self._ui.propertyLegendLabelNoValue
        noValueLegend.setText(legendTextUpdate(noValueLegend, NoValueTextColor))

        defaultLegend = self._ui.propertyLegendLabelDefault
        defaultLegend.setText(legendTextUpdate(defaultLegend, DefaultTextColor))

        customLegend = self._ui.propertyLegendLabelCustom 
        customLegend.setText(legendTextUpdate(customLegend, RedColor))

        interpolatedStr = 'Interpolated'
        tsLabel = self._ui.propertyLegendLabelTimeSample
        tsLabel.setText(ItalicizeLabelText(tsLabel.text(), interpolatedStr))

        vcLabel = self._ui.propertyLegendLabelValueClips
        vcLabel.setText(ItalicizeLabelText(vcLabel.text(), interpolatedStr))

        # Load up and set the icons for the property legend
        self._ui.propertyLegendTargetIcon.setPixmap(
            TARGET_TYPE_ICON().pixmap(*self._iconDisplaySize)) 
        self._ui.propertyLegendConnIcon.setPixmap(
            CONN_TYPE_ICON().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendAttrPlainIcon.setPixmap(
            ATTR_PLAIN_TYPE_ICON().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendRelPlainIcon.setPixmap(
            REL_PLAIN_TYPE_ICON().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendAttrWithConnIcon.setPixmap(
            ATTR_WITH_CONN_TYPE_ICON().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendRelWithTargetIcon.setPixmap(
            REL_WITH_TARGET_TYPE_ICON().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendCompIcon.setPixmap(
            CMP_TYPE_ICON().pixmap(*self._iconDisplaySize))
    
    def IsMinimized(self):
        return self._isMinimized

    def ToggleMinimized(self):
        self._isMinimized = not self._isMinimized

    def GetHeight(self):
        return self.height()

    def GetResetHeight(self):
        # This predefined height is determined by the elements that exist in
        # the propertyLegend. For more information see propertyLegendUI.ui
        return 120 
