#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from .qt import QtWidgets
from .primContextMenuItems import _GetContextMenuItems

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
