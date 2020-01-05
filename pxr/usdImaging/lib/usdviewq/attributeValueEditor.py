#
# Copyright 2016 Pixar
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
from pxr import Usd
from qt import QtCore, QtWidgets
from attributeValueEditorUI import Ui_AttributeValueEditor
from common import GetPropertyColor, UIPropertyValueSourceColors
from scalarTypes import ToString

# This is the widget that appears when selecting an attribute and
# opening the "Value" tab.

class AttributeValueEditor(QtWidgets.QWidget):
    editComplete = QtCore.Signal('QString')

    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)
        self._ui = Ui_AttributeValueEditor()
        self._ui.setupUi(self)

        self._defaultView = self._ui.valueViewer

        from arrayAttributeView import ArrayAttributeView
        self._extraAttrViews = [
                ArrayAttributeView(self),
                ]

        for attrView in self._extraAttrViews:
            self._ui.stackedWidget.addWidget(attrView)

        self.clear()

    def setAppController(self, appController):
        # pass the appController instance from which to retrieve
        # variable data.
        self._appController = appController

    def populate(self, primPath, propName):
        # called when the selected attribute has changed
        self._primPath = primPath

        try:
            self._attribute = self._appController._propertiesDict[propName]
        except KeyError:
            self._attribute = None

        self._isSet = True  # an attribute is selected

        self.refresh()  # load the value at the current frame

    def _FindView(self, attr):
        # Early-out for CustomAttributes and Relationships
        if not isinstance(attr, Usd.Attribute):
            return None

        for attrView in self._extraAttrViews:
            if attrView.CanView(attr):
                return attrView

        return None

    def refresh(self):
        # usually called upon frame change or selected attribute change
        if not self._isSet:
            return

        # attribute connections and relationship targets have no value to display
        # in the value viewer.
        if self._attribute is None:
            return

        # If the current attribute doesn't belong to the current prim, don't
        # display its value.
        if self._attribute.GetPrimPath() != self._primPath:
            self._ui.valueViewer.setText("")
            return

        frame = self._appController._dataModel.currentFrame

        # get the value of the attribute
        if isinstance(self._attribute, Usd.Relationship):
            self._val = self._attribute.GetTargets()
        else: # Usd.Attribute or CustomAttribute
            self._val = self._attribute.Get(frame)

        whichView = self._FindView(self._attribute)
        if whichView:
            self._ui.stackedWidget.setCurrentWidget(whichView)
            whichView.SetAttribute(self._attribute, frame)
        else:
            self._ui.stackedWidget.setCurrentWidget(self._defaultView)
            txtColor = GetPropertyColor(self._attribute, frame)

            # set text and color in the value viewer
            self._ui.valueViewer.setTextColor(txtColor)

            if isinstance(self._attribute, Usd.Relationship):
                typeName = None
            else:
                typeName = self._attribute.GetTypeName()
            rowText = ToString(self._val, typeName)
            self._ui.valueViewer.setText(rowText)

    def clear(self):
        # set the value editor to 'no attribute selected' mode
        self._isSet = False
        self._ui.valueViewer.setText("")
        # make sure we're showing the default view
        self._ui.stackedWidget.setCurrentWidget(self._defaultView)
