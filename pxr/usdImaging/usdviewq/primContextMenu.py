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
from qt import QtWidgets
from primContextMenuItems import _GetContextMenuItems

#
# Specialized context menu for prim selection.
#
# It uses the per-prim context menus referenced by the _GetContextMenuItems
# function in primContextMenuItems. To add a new context menu item,
# see comments in that file.
#
class PrimContextMenu(QtWidgets.QMenu):

    def __init__(self, parent, item, appController):
        QtWidgets.QMenu.__init__(self, parent)
        self._menuItems = _GetContextMenuItems(appController, item)

        for menuItem in self._menuItems:
            if menuItem.IsSeparator():
                self.addSeparator()
                continue

            elif not menuItem.isValid():
                continue

            action = self.addAction(menuItem.GetText(),
                                    menuItem.RunCommand)

            if not menuItem.IsEnabled():
                action.setEnabled( False )
