#
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the terms set forth in the LICENSE.txt file available
# at the root of this repository.
#

from enum import auto, Enum

from pxr.Usdviewq.qt import QtGui, QtWidgets


class M(Enum):
    SEPARATOR = auto()
    ACTION = auto()
    CHECK = auto()
    SUBMENU = auto()
    SUBMENU_GROUP = auto()
    GROUP_ITEM = auto()


class MenuBuilder:
    def __init__(self, owner):
        self._owner = owner

    def buildMenuBar(self, menuBar, menuConf):
        for title, items in menuConf:
            menu = menuBar.addMenu(title)
            self.buildMenuItems(menu, items)

    def buildContextMenu(self, menu, menuConf):
        self.buildMenuItems(menu, menuConf)

    def buildContextMenuWithActions(self, menuConf, menu=None):
        if menu is None:
            menu = QtWidgets.QMenu(self._owner)

        actionMap = {}
        self._buildMenuItemsWithActions(menu, menuConf, actionMap)
        return menu, actionMap

    def _buildMenuItemsWithActions(self, menu, items, actionMap, actionGroup=None):
        for item in items:
            if not item or len(item) == 0:
                continue

            itemType = item[0]

            if itemType == M.SEPARATOR:
                menu.addSeparator()

            elif itemType == M.ACTION:
                action = self._buildActionWithMap(menu, item, actionMap)

            elif itemType == M.CHECK:
                action = self._buildCheckActionWithMap(menu, item, actionMap)

            elif itemType == M.SUBMENU:
                self._buildSubmenuWithActions(menu, item, actionMap)

            elif itemType == M.SUBMENU_GROUP:
                self._buildSubmenuGroupWithActions(menu, item, actionMap)

            elif itemType == M.GROUP_ITEM:
                if actionGroup is not None:
                    action = self._buildGroupItemWithMap(
                        menu, item, actionGroup, actionMap
                    )

    def _buildActionWithMap(self, menu, item, actionMap):
        _, label, shortcut, handler = item[:4]
        action = menu.addAction(label)
        if shortcut:
            action.setShortcut(QtGui.QKeySequence(shortcut))
        if handler:
            action.triggered.connect(handler)
        else:
            action.setEnabled(False)
        actionMap[label] = action
        return action

    def _buildCheckActionWithMap(self, menu, item, actionMap):
        if len(item) >= 6:
            _, label, shortcut, handler, checked, storeAs = item[:6]
        else:
            _, label, shortcut, handler, checked = item[:5]
            storeAs = None

        action = menu.addAction(label)
        if shortcut:
            action.setShortcut(QtGui.QKeySequence(shortcut))
        action.setCheckable(True)
        action.setChecked(checked)
        if handler:
            action.triggered.connect(handler)
        if storeAs and self._owner:
            setattr(self._owner, storeAs, action)
        actionMap[label] = action
        return action

    def _buildSubmenuWithActions(self, menu, item, actionMap):
        _, label, subItems = item
        submenu = menu.addMenu(label)
        self._buildMenuItemsWithActions(submenu, subItems, actionMap)
        return submenu

    def _buildSubmenuGroupWithActions(self, menu, item, actionMap):
        _, label, subItems, groupStore, groupHandler = item
        submenu = menu.addMenu(label)
        group = QtGui.QActionGroup(self._owner)
        group.setExclusive(True)
        if groupStore and self._owner:
            setattr(self._owner, groupStore, group)
        if groupHandler:
            group.triggered.connect(groupHandler)
        self._buildMenuItemsWithActions(submenu, subItems, actionMap, group)
        return submenu

    def _buildGroupItemWithMap(self, menu, item, actionGroup, actionMap):
        _, label, data, checked = item
        action = menu.addAction(label)
        action.setCheckable(True)
        action.setChecked(checked)
        action.setData(data)
        actionGroup.addAction(action)
        actionMap[label] = action
        return action

    @staticmethod
    def updateContextMenuState(actionMap, menuConf):
        MenuBuilder._updateItemsState(actionMap, menuConf)

    @staticmethod
    def _updateItemsState(actionMap, items):
        for item in items:
            if not item or len(item) == 0:
                continue

            itemType = item[0]

            if itemType == M.ACTION:
                _, label, shortcut, handler = item[:4]
                if label in actionMap:
                    actionMap[label].setEnabled(handler is not None)

            elif itemType == M.CHECK:
                if len(item) >= 6:
                    _, label, shortcut, handler, checked, storeAs = item[:6]
                else:
                    _, label, shortcut, handler, checked = item[:5]
                if label in actionMap:
                    actionMap[label].setChecked(checked)
                    actionMap[label].setEnabled(handler is not None)

            elif itemType == M.SUBMENU:
                _, label, subItems = item
                MenuBuilder._updateItemsState(actionMap, subItems)

            elif itemType == M.SUBMENU_GROUP:
                _, label, subItems, groupStore, groupHandler = item
                MenuBuilder._updateItemsState(actionMap, subItems)

            elif itemType == M.GROUP_ITEM:
                _, label, data, checked = item
                if label in actionMap:
                    actionMap[label].setChecked(checked)

    def buildMenuItems(self, menu, items, actionGroup=None):
        for item in items:
            if not item or len(item) == 0:
                continue

            itemType = item[0]

            if itemType == M.SEPARATOR:
                menu.addSeparator()

            elif itemType == M.ACTION:
                self._buildAction(menu, item)

            elif itemType == M.CHECK:
                self._buildCheckAction(menu, item)

            elif itemType == M.SUBMENU:
                self._buildSubmenu(menu, item)

            elif itemType == M.SUBMENU_GROUP:
                self._buildSubmenuGroup(menu, item)

            elif itemType == M.GROUP_ITEM:
                if actionGroup is not None:
                    self._buildGroupItem(menu, item, actionGroup)

    def _buildAction(self, menu, item):
        _, label, shortcut, handler = item
        action = menu.addAction(label)
        if shortcut:
            action.setShortcut(QtGui.QKeySequence(shortcut))
        if handler:
            action.triggered.connect(handler)
        else:
            action.setEnabled(False)
        return action

    def _buildCheckAction(self, menu, item):
        if len(item) >= 6:
            _, label, shortcut, handler, checked, storeAs = item[:6]
        else:
            _, label, shortcut, handler, checked = item[:5]
            storeAs = None

        action = menu.addAction(label)
        if shortcut:
            action.setShortcut(QtGui.QKeySequence(shortcut))
        action.setCheckable(True)
        action.setChecked(checked)
        if handler:
            action.triggered.connect(handler)
        if storeAs and self._owner:
            setattr(self._owner, storeAs, action)
        return action

    def _buildSubmenu(self, menu, item):
        _, label, subItems = item
        submenu = menu.addMenu(label)
        self.buildMenuItems(submenu, subItems)
        return submenu

    def _buildSubmenuGroup(self, menu, item):
        _, label, subItems, groupStore, groupHandler = item
        submenu = menu.addMenu(label)
        group = QtGui.QActionGroup(self._owner)
        group.setExclusive(True)
        if groupStore and self._owner:
            setattr(self._owner, groupStore, group)
        if groupHandler:
            group.triggered.connect(groupHandler)
        self.buildMenuItems(submenu, subItems, group)
        return submenu

    def _buildGroupItem(self, menu, item, actionGroup):
        _, label, data, checked = item
        action = menu.addAction(label)
        action.setCheckable(True)
        action.setChecked(checked)
        action.setData(data)
        actionGroup.addAction(action)
        return action


def buildMenuBar(owner, menuBar, menuConf):
    builder = MenuBuilder(owner)
    builder.buildMenuBar(menuBar, menuConf)


def buildContextMenu(owner, menu, menuConf):
    builder = MenuBuilder(owner)
    builder.buildContextMenu(menu, menuConf)
