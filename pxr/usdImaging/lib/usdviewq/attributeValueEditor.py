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
from qt import QtCore, QtWidgets
from attributeValueEditorUI import Ui_AttributeValueEditor
from pythonExpressionPrompt import PythonExpressionPrompt
from common import GetAttributeColor, TimeSampleTextColor

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

        self._ui.editButton.clicked.connect(self._edit)

        self._ui.revertButton.clicked.connect(self._revert)

        self._ui.revertAllButton.clicked.connect(self._revertAll)

    def setMainWindow(self, mainWindow):
        # pass the mainWindow instance from which to retrieve 
        # variable data.
        self._mainWindow = mainWindow
        self._propertyView = mainWindow._ui.propertyView

    def populate(self, name, node):
        # called when the selected attribute has changed
        # gets the attribute object and the source node
        self._attribute = self._mainWindow._attributeDict[name]
        self._isSet = True  # an attribute is selected
        self._node = node

        # determine if the attribute is editable, enable or disable buttons
        # XXX USD DETERMINE EDITABILITY
        #editable = self._name[0] != ' '
        #self._ui.editButton.setEnabled(editable)
        #self._ui.revertAllButton.setEnabled(editable)

        self.refresh()  # load the value at the current frame

    def _FindView(self, attr):
        from customAttributes import CustomAttribute
        if isinstance(attr, CustomAttribute):
            return None

        for attrView in self._extraAttrViews:
            if attrView.CanView(attr):
                return attrView

        return None

    def refresh(self):
        # usually called upon frame change or selected attribute change
        if not self._isSet:
            return

        frame = self._mainWindow._currentFrame

        # get the value of the attribute
        self._val = self._attribute.Get(frame)

        whichView = self._FindView(self._attribute)
        if whichView:
            self._ui.stackedWidget.setCurrentWidget(whichView)
            whichView.SetAttribute(self._attribute, frame)
        else:
            self._ui.stackedWidget.setCurrentWidget(self._defaultView)
            txtColor = GetAttributeColor(self._attribute, frame)

            # set text and color in the value viewer
            self._ui.valueViewer.setTextColor(txtColor)

            from scalarTypes import ToString
            rowText = ToString(self._val, self._attribute.GetTypeName())
            self._ui.valueViewer.setText(rowText)

        #XXX USD Determine whether the revert button should be on
        self._ui.revertButton.setEnabled(False)

    def clear(self):
        # set the value editor to 'no attribute selected' mode
        self._isSet = False
        self._ui.valueViewer.setText("")

        self._ui.editButton.setEnabled(False)
        self._ui.revertButton.setEnabled(False)
        self._ui.revertAllButton.setEnabled(False)

    # XXX USD EDITING DISABLED
    def _edit(self, exception = None):
        # opens the interpreter to receive user input
        frame = self._mainWindow._currentFrame
        type = 'member' if self._isMember else 'attribute'

        # this call opens the interpreter window and has it return the value of
        # "_" when done.
        value = PythonExpressionPrompt.getValueFromPython(self, exception, self._val)
        if value is None:   # Cancelled
            return

        try:
            # value successfully retrieved. set it.
            if self._isMember:
                self._node.SetMember(frame, self._name, value)
            else:
                self._node.SetAttribute(frame, self._name, value)
            
            # send a signal to the mainWindow confirming the edit
            msg = 'Successfully edited %s "%s" at frame %s.' \
                    %(type, self._name, frame)
            self.editComplete.emit(msg)

        except Exception as e:
            # if an error occurs, reopen the interpreter and display it
            self._edit(e)

    def _revert(self, all=False):
        from pxr import Usd

        # revert one or all overrides on a member
        type = 'member' if self._isMember else 'attribute'
        frame = Usd.Object.FRAME_INVALID if all else \
                self._mainWindow._currentFrame
        frameStr = 'all frames' if all else \
                   'frame %s' %(self._mainWindow._currentFrame)
        section = Usd.USD_SECTION_ALL if all else \
                  Usd.USD_SECTION_INVALID

        # ask for confirmation before reverting overrides
        reply = QtWidgets.QMessageBox.question(self, "Confirm Revert",
                    "Are you sure you want to revert the %s "
                    "<font color='%s'><b>%s</b></font> at %s?"
                        %(type, TimeSampleTextColor.color().name(), self._name, frameStr),
                    QtWidgets.QMessageBox.Cancel | QtWidgets.QMessageBox.Yes,
                    QtWidgets.QMessageBox.Cancel)
    
        msg = ""
        if reply == QtWidgets.QMessageBox.Yes and self._node is not None:
            # we got 'yes' as an answer
            try:
                # this removes only the value written in the top level stage
                # which is the writable reference.
                if self._isMember:
                    self._node.RemoveMember(frame, section, self._name)
                else:
                    self._node.RemoveAttribute(frame, section, self._name)

                msg = 'Reverted %s "%s" at %s.' %(type, self._name, frameStr)
                self._mainWindow._refreshVars()

            except RuntimeError:
                msg = 'Failed to revert the %s "%s" at %s. Perhaps this '\
                      '%s is not authored in the override stage.'\
                            %(type, self._name, frameStr, type)
        
        # display status message
        self._mainWindow.statusMessage(msg, 12)

    def _revertAll(self):
        self._revert(True)

