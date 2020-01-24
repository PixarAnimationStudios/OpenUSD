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
from usdviewContextMenuItem import UsdviewContextMenuItem

#
# Specialized context menu for adding and removing columns
# in the prim browser and attribute inspector.
#
class HeaderContextMenu(QtWidgets.QMenu):

    def __init__(self, parent):
        QtWidgets.QMenu.__init__(self, "Columns", parent)
        self._menuItems = _GetContextMenuItems(parent)

        for menuItem in self._menuItems:
            if menuItem.isValid():
                # create menu actions.  Associate them with each Item, so that
                # we can update them when we show, since column visibility can
                # change.
                action = self.addAction(menuItem.GetText(), menuItem.RunCommand)
                action.setCheckable(True)
                menuItem.action = action

        self.aboutToShow.connect(self._prepForShow)

    def _prepForShow(self):
        for menuItem in self._menuItems:
            if menuItem.action:
                menuItem.action.setChecked(menuItem.IsChecked())
                menuItem.action.setEnabled(menuItem.IsEnabled())


def _GetContextMenuItems(parent):
    # create a list of HeaderContextMenuItem classes
    # initialized with the parent object and column
    itemList = []

    return [HeaderContextMenuItem(parent, column) \
                for column in range(parent.columnCount())]

# The base class for header context menus
class HeaderContextMenuItem(UsdviewContextMenuItem):

    def __init__(self, parent, column):
        self._parent = parent
        self._column = column
        self._action = None

        if isinstance(parent, QtWidgets.QTreeWidget):
            self._text = parent.headerItem().text(column)
        else:
            self._text = parent.horizontalHeaderItem(column).text()

    def GetText(self):
        # returns the text to be displayed in menu
        return self._text

    def IsEnabled(self):
        # Enable context menu item for columns except the "Name" column.
        return 'Name' not in self.GetText()

    def IsChecked(self):
        # true if the column is visible, false otherwise
        return not self._parent.isColumnHidden(self._column)

    def RunCommand(self):
        # show or hide the column depending on its previous state
        self._parent.setColumnHidden(self._column, self.IsChecked())

    @property
    def action(self):
        return self._action

    @action.setter
    def action(self, action):
        self._action = action

