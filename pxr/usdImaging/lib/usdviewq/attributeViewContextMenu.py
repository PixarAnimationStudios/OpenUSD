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
from qt import QtGui, QtWidgets
from usdviewContextMenuItem import UsdviewContextMenuItem
from common import INDEX_PROPNAME, INDEX_PROPTYPE, INDEX_PROPVAL

#
# Specialized context menu for running commands in the attribute viewer.
#
class AttributeViewContextMenu(QtWidgets.QMenu):

    def __init__(self, parent, item):
        QtWidgets.QMenu.__init__(self, parent)
        self._menuItems = _GetContextMenuItems(parent, item)

        for menuItem in self._menuItems:
            # create menu actions
            if menuItem.isValid():
                action = self.addAction(menuItem.GetText(), menuItem.RunCommand)

                # set enabled
                if not menuItem.IsEnabled():
                    action.setEnabled(False)


def _GetContextMenuItems(mainWindow, item):
    return [CopyAttributeNameMenuItem(mainWindow, item)]

#
# The base class for layer stack context menu items.
#
class AttributeViewContextMenuItem(UsdviewContextMenuItem):

    def __init__(self, mainWindow, item):
        self._mainWindow = mainWindow
        self._item = item

    def IsEnabled(self):
        return True

    def GetText(self):
        return ""

    def RunCommand(self):
        pass

#
# Copy the attribute's name / value to clipboard.
#
class CopyAttributeNameMenuItem(AttributeViewContextMenuItem):

    def GetText(self):
        if self._item.column() == INDEX_PROPNAME:
            return "Copy Property Name"
        elif self._item.column() == INDEX_PROPVAL:
            return "Copy Property Value"

    def RunCommand(self):
        if not self._item:
            return

        # Copy item text
        txt = self._item.text()
        cb = QtWidgets.QApplication.clipboard()
        cb.setText(txt, QtGui.QClipboard.Selection)
        cb.setText(txt, QtGui.QClipboard.Clipboard)

