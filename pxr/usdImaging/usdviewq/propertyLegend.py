#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
""" This module provides the help dialog(also known as the property legend)
    in Usdview's MainWindow. This provides a key describing the items displayed
    in the property browser.
"""

from .qt import QtWidgets

from .propertyLegendUI import Ui_PropertyLegend

from .common import UIBaseColors, UIPropertyValueSourceColors, ItalicizeLabelText, PropertyViewIcons

class PropertyLegend(QtWidgets.QWidget):
    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self._ui = Ui_PropertyLegend()
        self._ui.setupUi(self)

        # The property legend always starts off collapsed.
        self.setMaximumHeight(0)
        self._isMinimized = True
        self._iconDisplaySize = (16, 16)
        
        graphicsScene = QtWidgets.QGraphicsScene()

        self._ui.propertyLegendColorFallback.setScene(graphicsScene)
        self._ui.propertyLegendColorDefault.setScene(graphicsScene)
        self._ui.propertyLegendColorTimeSample.setScene(graphicsScene)
        self._ui.propertyLegendColorNoValue.setScene(graphicsScene)
        self._ui.propertyLegendColorValueClips.setScene(graphicsScene)
        self._ui.propertyLegendColorCustom.setScene(graphicsScene)

        # set color of attribute viewer legend boxes
        self._ui.propertyLegendColorFallback.setForegroundBrush(
            UIPropertyValueSourceColors.FALLBACK)
        self._ui.propertyLegendColorDefault.setForegroundBrush(
            UIPropertyValueSourceColors.DEFAULT)
        self._ui.propertyLegendColorTimeSample.setForegroundBrush(
            UIPropertyValueSourceColors.TIME_SAMPLE)
        self._ui.propertyLegendColorNoValue.setForegroundBrush(
            UIPropertyValueSourceColors.NONE)
        self._ui.propertyLegendColorValueClips.setForegroundBrush(
            UIPropertyValueSourceColors.VALUE_CLIPS)
        self._ui.propertyLegendColorCustom.setForegroundBrush(
            UIBaseColors.RED)

        # set color of attribute viewer text items
        legendTextUpdate = lambda t, c: (
            ('<font color=\"%s\">' % c.color().name()) + t.text() + '</font>')
        timeSampleLegend = self._ui.propertyLegendLabelTimeSample
        timeSampleLegend.setText(
            legendTextUpdate(timeSampleLegend, UIPropertyValueSourceColors.TIME_SAMPLE))

        fallbackLegend = self._ui.propertyLegendLabelFallback
        fallbackLegend.setText(
            legendTextUpdate(fallbackLegend, UIPropertyValueSourceColors.FALLBACK))

        valueClipLegend = self._ui.propertyLegendLabelValueClips
        valueClipLegend.setText(
            legendTextUpdate(valueClipLegend, UIPropertyValueSourceColors.VALUE_CLIPS))

        noValueLegend = self._ui.propertyLegendLabelNoValue
        noValueLegend.setText(
            legendTextUpdate(noValueLegend, UIPropertyValueSourceColors.NONE))

        defaultLegend = self._ui.propertyLegendLabelDefault
        defaultLegend.setText(
            legendTextUpdate(defaultLegend, UIPropertyValueSourceColors.DEFAULT))

        customLegend = self._ui.propertyLegendLabelCustom
        customLegend.setText(
            legendTextUpdate(customLegend, UIBaseColors.RED))

        interpolatedStr = 'Interpolated'
        tsLabel = self._ui.propertyLegendLabelTimeSample
        tsLabel.setText(ItalicizeLabelText(tsLabel.text(), interpolatedStr))

        vcLabel = self._ui.propertyLegendLabelValueClips
        vcLabel.setText(ItalicizeLabelText(vcLabel.text(), interpolatedStr))

        # Load up and set the icons for the property legend
        self._ui.propertyLegendTargetIcon.setPixmap(
            PropertyViewIcons.TARGET().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendConnIcon.setPixmap(
            PropertyViewIcons.CONNECTION().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendAttrPlainIcon.setPixmap(
            PropertyViewIcons.ATTRIBUTE().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendRelPlainIcon.setPixmap(
            PropertyViewIcons.RELATIONSHIP().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendAttrWithConnIcon.setPixmap(
            PropertyViewIcons.ATTRIBUTE_WITH_CONNECTIONS().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendRelWithTargetIcon.setPixmap(
            PropertyViewIcons.RELATIONSHIP_WITH_TARGETS().pixmap(*self._iconDisplaySize))
        self._ui.propertyLegendCompIcon.setPixmap(
            PropertyViewIcons.COMPOSED().pixmap(*self._iconDisplaySize))

    def IsMinimized(self):
        return self._isMinimized

    def ToggleMinimized(self):
        self._isMinimized = not self._isMinimized

    def GetHeight(self):
        return self.height()

    def GetResetHeight(self):
        return self.sizeHint().height()
