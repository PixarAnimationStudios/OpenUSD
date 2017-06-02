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
import hou

if hou.applicationVersion()[0] >= 16:
    from PySide2.QtGui import *
    from PySide2.QtCore import *
    from PySide2.QtWidgets import *
else:
    from PySide.QtGui import *
    from PySide.QtCore import *

import re
from treemodel import *

#
# Global variables
#
nodeTypeLowerCase = 'usdimport'

#
# Return a list of hou.NodeType that are considered the only
# compatible node types that this plugin will work with.
#
def CompatibleNodeTypes():
    nodeTypes = []

    for category in [hou.objNodeTypeCategory, hou.sopNodeTypeCategory]:
        for key,value in category().nodeTypes().items():
            if nodeTypeLowerCase not in key.lower():
                continue

            parms = value.parmTemplateGroup()

            nameUsdFile = parms.find(TreeModel.parmNameUsdFile)
            if not nameUsdFile or\
               not parms.find(nameUsdFile.defaultValue()[0]):
                continue

            namePrimPaths = parms.find(TreeModel.parmNamePrimPaths)
            if not namePrimPaths or\
               not parms.find(namePrimPaths.defaultValue()[0]):
                continue

            if not parms.find(TreeModel.parmUiExpandState):
                continue

            nodeTypes.append(value)

    return nodeTypes

#
# Use regular expressions to extract colors from houdini's StyleSheet.
#
def ExtractColorsFromStyleSheet():
    qtStyleSheet = hou.ui.qtStyleSheet()

    grpQTreeView = '(QTreeView\n\{[\s\S]*?)'
    grpBG = '([\s]+background(-color)?: )'
    grpAltBG = '([\s]+alternate-background(-color)?: )'
    grpRgb = '(?P<rgb>rgb(a)?\(\d{1,3}(, \d{1,3}){2,3}\))'

    matchBG = re.search(grpQTreeView + grpBG + grpRgb, qtStyleSheet)
    matchAltBG = re.search(grpQTreeView + grpAltBG + grpRgb, qtStyleSheet)

    # Update TreeView.rgbRows colors.
    if matchBG and matchAltBG:
        TreeView.rgbRows = (matchBG.groupdict()['rgb'],\
                            matchAltBG.groupdict()['rgb'])

    # For the checkbox 'window' color (the color around the box), use
    # TreeView's alt bg color. Convert it from a string to a QColor.
    windowColor = TreeView.rgbRows[1].strip('rgba()')
    c = [int(channel) for channel in windowColor.split(',')]
    if len(c) == 3:
        CheckBoxStyled.windowColor = QColor(c[0], c[1], c[2])
    elif len(c) == 4:
        CheckBoxStyled.windowColor = QColor(c[0], c[1], c[2], c[3])

    # For the checkbox 'base' color (the color inside the box), extract
    # the background color of the QTextEdit widget.
    grpQTextEdit = '(QTextEdit\n\{[\s\S]*?)'
    matchBG = re.search(grpQTextEdit + grpBG + grpRgb, qtStyleSheet)
    if matchBG:
        # Convert it from a string to a QColor.
        baseColor = matchBG.groupdict()['rgb'].strip('rgba()')
        c = [int(channel) for channel in baseColor.split(',')]
        if len(c) == 3:
            CheckBoxStyled.baseColor = QColor(c[0], c[1], c[2])
        elif len(c) == 4:
            CheckBoxStyled.baseColor = QColor(c[0], c[1], c[2], c[3])

class CheckBoxStyled(QCheckBox):
    # Custom colors for checkbox widgets. These colors will be
    # updated by the ExtractColorsFromStyleSheet method above.
    windowColor = QColor(45, 45, 45)
    baseColor = QColor(19, 19, 19)

    def paintEvent(self, event):
        opt = QStyleOptionButton()
        self.initStyleOption(opt)

        style = self.style()
        opt.rect = style.subElementRect(QStyle.SE_CheckBoxClickRect, opt, self)

        # Set 'window' color (the color around the box).
        opt.palette.setColor(QPalette.Window, CheckBoxStyled.windowColor)
        # Set 'base' color (the color inside the box).
        opt.palette.setColor(QPalette.Base, CheckBoxStyled.baseColor)

        painter = QStylePainter(self)
        painter.drawControl(QStyle.CE_CheckBox, opt)

class ComboBoxStyled(QComboBox):
    def __init__(self, parent = None):
        super(ComboBoxStyled, self).__init__(parent)

        # Set 'combobox-popup' to 0 on self (the QComboBox itself) to prevent
        # the maxVisibleItems member of the QComboBox from being ignored. When
        # ignored, the popup displays as one long list that gets clipped when
        # too long. By not ignoring it, the size of the popup will be clamped
        # and will provide a vertical scroll bar for browsing items.
        self.setStyleSheet('QComboBox { combobox-popup: 0 };')

        # Create a QListView to replace the QComboBox's view. This allows the
        # item padding and border to be customized.
        listview = QListView()
        listview.setStyleSheet('QListView::item { padding: 2px; }\
                                QListView::item::selected { border: 0; }')
        self.setView(listview)

class ActiveNodeMenu(ComboBoxStyled):
    # Signals
    activeNodeChanged = Signal('QString')

    def __init__(self, parent):
        super(ActiveNodeMenu, self).__init__(parent)

        # Set minimum contents length to 20 characters.
        self.setMinimumContentsLength(20)
        self.setDuplicatesEnabled(False)

        self.ResetMenu()

        # Store a string to hold the path of the last selected node.
        self.lastSelected = ""

        self.ignoreCurrentIndexChanged = False
        self.currentIndexChanged[int].connect(self.OnCurrentIndexChanged)

    def showPopup(self):
        # The choices in this popup menu need to be updated. Turn on the
        # ignoreCurrentIndexChanged flag so that calls to setCurrentIndex
        # will not trigger the "activeNodeChanged" signal.
        self.ignoreCurrentIndexChanged = True

        # Store whatever the current selection is, then clear the menu.
        selected = self.currentText()
        self.ResetMenu()

        # Rebuild the menu.
        for nodeType in TreeView.nodeTypes:
            for instance in nodeType.instances():
                if TreeView.IsNodeCompatible(instance):
                    # Each menu item stores both the node's path
                    # and its session id.
                    self.addItem(instance.path(), instance.sessionId())

        # Restore whatever the selection was before the menu was cleared.
        index = self.findText(selected)
        self.setCurrentIndex(index if index > -1 else 0)

        # Disable the ignoreCurrentIndexChanged flag so that calls to
        # setCurrentIndex will again trigger the "activeNodeChanged" signal.
        self.ignoreCurrentIndexChanged = False

        # Now show the popup.
        super(ComboBoxStyled, self).showPopup()

    def ResetMenu(self):
        self.clear()
        # The 1st choice (at index 0) is named "Last Selected".
        self.addItem("Last Selected")

    def OnCurrentIndexChanged(self, index):
        # Only emit "activeNodeChanged" signal if the
        # ignoreCurrentIndexChanged flag is off.
        if self.ignoreCurrentIndexChanged:
            return

        if index < 0:
            self.activeNodeChanged.emit('')
        elif index == 0:
            self.activeNodeChanged.emit(self.lastSelected)
        else:
            self.activeNodeChanged.emit(self.itemText(index))

    def RemoveNodeChoice(self, node):
        # If the node choice to remove is currently
        # selected, change current to "Last Selected".
        current = self.itemData(self.currentIndex())
        if current == node.sessionId():
            self.setCurrentIndex(0) # "Last Selected" is always 0th item.

        if self.lastSelected == node.path():
            self.lastSelected = ""

    def RenameNodeChoice(self, node):
        # Use the node's sessionId to find a menu choice, which
        # has the name that the node had before it was renamed.
        index = self.findData(node.sessionId())
        if index != -1:
            oldPath = self.itemText(index)
            newPath = node.path()

            if self.currentIndex() == index:
                self.setItemText(index, newPath)

            if self.lastSelected == oldPath:
                self.lastSelected = newPath

    def GetLastSelected(self):
        return self.lastSelected

    def SetLastSelected(self, nodePath):
        if self.lastSelected != nodePath:
            self.lastSelected = nodePath

            if self.currentIndex() == 0:
                self.activeNodeChanged.emit(nodePath)

class TreeItemEditor(QFrame):
    # Signals
    edited = Signal()

    # Static members used for computing size hint.
    margins = QMargins(2, 2, 2, 2)
    checkBoxHeight = CheckBoxStyled().sizeHint().height()
    comboBoxHeight = ComboBoxStyled().sizeHint().height()

    @staticmethod
    def ItemHeight(index):
        height = TreeItemEditor.margins.top() + TreeItemEditor.margins.bottom()

        if index.column() == COL_IMPORT:
            height += TreeItemEditor.checkBoxHeight
        elif index.column() == COL_VARIANT:
            variants = index.data()
            height += TreeItemEditor.comboBoxHeight * len(variants)

        return height

    def __init__(self, parent, index):
        super(TreeItemEditor, self).__init__(parent)

        layout = QGridLayout()
        layout.setContentsMargins(TreeItemEditor.margins)

        if index.column() == COL_IMPORT:
            self._checkBox = CheckBoxStyled(self)
            self._checkBox.setCheckState(Qt.CheckState(index.data()))
            self._checkBox.stateChanged.connect(self.DataChanged)
            layout.addWidget(self._checkBox, 0, 0, Qt.AlignHCenter)

        elif index.column() == COL_VARIANT:
            variants = index.data()

            # Only the 3rd column should stretch.
            layout.setColumnStretch(0, 0)
            layout.setColumnStretch(1, 0)
            layout.setColumnStretch(2, 1)

            layout.setColumnMinimumWidth(1, 100)
            layout.setHorizontalSpacing(layout.spacing())
            layout.setVerticalSpacing(0)
            
            self._checkBoxes = []
            self._comboBoxes = []

            for row, item in enumerate(variants):            
                self._checkBoxes.append(CheckBoxStyled(self))
                self._comboBoxes.append(ComboBoxStyled(self))

                checkBox = self._checkBoxes[-1]
                checkBox.setCheckState(Qt.Checked if item._enabled\
                                                  else Qt.Unchecked)
                checkBox.stateChanged.connect(self.DataChanged)

                label = QLabel(item._name, self)
                label.setEnabled(item._enabled)
                checkBox.stateChanged.connect(label.setEnabled)

                comboBox = self._comboBoxes[-1]
                comboBox.addItems(item._choices)
                index = comboBox.findText(item._initialSelection)
                if index != -1:
                    comboBox.setCurrentIndex(index)
                comboBox.setEnabled(item._enabled)
                comboBox.currentIndexChanged.connect(self.DataChanged)
                checkBox.stateChanged.connect(comboBox.setEnabled)

                layout.addWidget(checkBox, row, 0, Qt.AlignHCenter)
                layout.addWidget(label, row, 1, Qt.AlignLeft)
                layout.addWidget(comboBox, row, 2)

        self.setLayout(layout)

    def DataChanged(self):
        self.edited.emit()

    def GetData(self, index):
        if index.column() == COL_IMPORT:
            return self._checkBox.checkState()

        elif index.column() == COL_VARIANT:
            variants = index.data()

            dstItems = []
            for row, srcItem in enumerate(variants):
                dstItem = VariantInfo(srcItem._name,\
                                      srcItem._choices,\
                                      srcItem._initialSelection,\
                                      bool(self._checkBoxes[row].checkState()))
                if dstItem._enabled:
                    dstItem._currentSelection =\
                        str(self._comboBoxes[row].currentText())
                else:
                    dstItem._currentSelection = dstItem._initialSelection
                dstItems.append(dstItem)
            return dstItems

    def SetData(self, index):
        if index.column() == COL_IMPORT:
            self._checkBox.setCheckState(Qt.CheckState(index.data()))

        elif index.column() == COL_VARIANT:
            variants = index.data()
            for row, item in enumerate(variants):

                checkBox = self._checkBoxes[row]
                comboBox = self._comboBoxes[row]
                index = -1
                if item._enabled:
                    checkBox.setCheckState(Qt.Checked)
                    index = comboBox.findText(item._currentSelection)
                else:
                    checkBox.setCheckState(Qt.Unchecked)
                    index = comboBox.findText(item._initialSelection)
                if index != -1:
                    comboBox.setCurrentIndex(index)

class TreeItemDelegate(QStyledItemDelegate):
    def __init__(self):
        QStyledItemDelegate.__init__(self)

    def createEditor(self, parent, option, index):
        editor = TreeItemEditor(parent, index)
        editor.edited.connect(self.commitAndCloseEditor)
        return editor

    def setEditorData(self, editor, index):
        editor.SetData(index)

    def setModelData(self, editor, model, index):
        model.setData(index, editor.GetData(index))

    def sizeHint(self, option, index):
        return QSize(0, TreeItemEditor.ItemHeight(index))

    def commitAndCloseEditor(self):
        editor = self.sender()
        self.commitData.emit(editor)
        self.closeEditor.emit(editor, QAbstractItemDelegate.NoHint)

class TreeView(QFrame):
    # Declare nodeTypes as a static member of TreeView.
    nodeTypes = CompatibleNodeTypes()

    # Static model to use for views with no "Active Node".
    emptyModel = TreeModel(COL_HEADERS)

    # Background row colors for custom widgets. These initial default
    # colors will be updated by the OnStyleChanged method below.
    rgbRows = ('rgb(45, 45, 45)', 'rgb(58, 58, 58)')

    def __init__(self):
        QFrame.__init__(self)

        # Houdini styling is not automatically applied to custom widgets.
        # Set the "houdiniStyle" property to True on this top-level widget
        # so it matches the style of other houdini widgets.
        self.setProperty("houdiniStyle", True)

        self.switchShowVariants = CheckBoxStyled()
        self.switchShowVariants.stateChanged.connect(self.ShowVariants)

        self.activeNodeMenu = ActiveNodeMenu(self)
        self.activeNodeMenu.activeNodeChanged.connect(\
            self.OnActiveNodeChanged)

        toolbarLayout = QHBoxLayout()
        toolbarLayout.setSpacing(2)
        toolbarLayout.addWidget(self.switchShowVariants, 1, Qt.AlignRight)
        toolbarLayout.addWidget(QLabel("Show Variants"))
        toolbarLayout.addSpacing(20)
        toolbarLayout.addWidget(QLabel("Active Node"))
        toolbarLayout.addWidget(self.activeNodeMenu)

        self.view = QTreeView()
        self.view.setModel(TreeView.emptyModel)

        self.view.setItemDelegate(TreeItemDelegate())

        self.view.setColumnWidth(COL_NAME, 300)
        self.view.resizeColumnToContents(COL_IMPORT)
        self.view.setColumnWidth(COL_TYPE, 60)
        self.view.setColumnWidth(COL_VARIANT, 340)
        self.view.resizeColumnToContents(COL_END)

        self.view.setAlternatingRowColors(True)
        self.view.verticalScrollBar().valueChanged.connect(\
            self.view.updateGeometries)
        self.view.horizontalScrollBar().valueChanged.connect(\
            self.view.updateGeometries)

        self.view.expanded.connect(self.BecameVisible)
        self.view.collapsed.connect(self.ResetBackgroundColors)

        mainLayout = QVBoxLayout()
        mainLayout.setSpacing(0)
        mainLayout.setContentsMargins(2, 2, 2, 2)
        mainLayout.addLayout(toolbarLayout)
        mainLayout.addWidget(self.view)

        self.setLayout(mainLayout)

        self.ShowVariants(self.switchShowVariants.checkState())

        hou.ui.addEventLoopCallback(self.CheckNodeSelection)

    def changeEvent(self, event):
        if event.type() == QEvent.StyleChange:
            self.OnStyleChanged()
        super(TreeView, self).changeEvent(event)

    def closeEvent(self, event):
        try:
            hou.ui.removeEventLoopCallback(self.CheckNodeSelection)
        except hou.Error:
            pass
        self.UnsyncViewWithModel()
        event.accept()

    @staticmethod
    def IsNodeCompatible(node, ignoreNodeType = True):
        if not ignoreNodeType and node.type() not in TreeView.nodeTypes:
            return False

        namePrimPaths = node.parm(TreeModel.parmNamePrimPaths).eval()
        parm = node.parm(namePrimPaths)

        # If the primpaths parm is channel-referenced to another parameter, it
        # won't work with this plugin. This case happens when there is a usd
        # import SOP embedded in a usd import OTL. Only one of the two should
        # be used by the plugin, not both. This check allows the OTL to pass,
        # and causes the embedded SOP to fail.
        if parm.getReferencedParm().path() != parm.path():
            return False
        return True

    def CheckNodeSelection(self):
        selectedNodes = hou.selectedNodes()

        # Exit early if there are no nodes selected OR if there is only one
        # node selected and it matches activeNodeMenu's "Last Selected" node.
        if len(selectedNodes) == 0 or (len(selectedNodes) == 1 and\
            selectedNodes[0].path() == self.activeNodeMenu.GetLastSelected()):
            return

        # Look for the first usd import node in houdini's current selection
        # to update the activeNodeMenu's "Last Selected" node path.
        for node in selectedNodes:            
            if TreeView.IsNodeCompatible(node, ignoreNodeType=False):
                self.activeNodeMenu.SetLastSelected(node.path())
                break

    def OnActiveNodeChanged(self, nodePath):
        node = hou.node(str(nodePath))
        if not node:
            self.SyncViewWithModel(TreeView.emptyModel)
            return

        key = node.sessionId()
        model = hou.session.UsdImportDict.get(key, None)

        if model:
            if model != self.view.model():
                self.SyncViewWithModel(model)
        else:
            model = TreeModel(COL_HEADERS, node)
            self.SyncViewWithModel(model)

            hou.session.UsdImportDict[key] = model

            node.addEventCallback(\
                (hou.nodeEventType.BeingDeleted,), self.OnNodeDeleted)
            node.addEventCallback(\
                (hou.nodeEventType.NameChanged,), self.OnNodeRenamed)
            node.addEventCallback(\
                (hou.nodeEventType.ParmTupleChanged,), self.OnParmChanged)

    def UnsyncViewWithModel(self):
        # Attempt to disconnect the current model.
        model = self.view.model()
        try:
            model.treeTopologyChanged.disconnect(self.OnTreeTopologyChanged)
            model.expandedPrimPathAdded.disconnect(self.view.expand)
            model.expandedPrimPathRemoved.disconnect(self.view.collapse)
            model.showingVariantsSwitched.disconnect(\
                self.switchShowVariants.setCheckState)

            self.view.expanded.disconnect(model.AddExpandedPrimPath)
            self.view.collapsed.disconnect(model.RemoveExpandedPrimPath)
            self.switchShowVariants.stateChanged.disconnect(\
                model.SetShowingVariants)

        except RuntimeError:
            # This error happens the first time this method is called
            # because none of these connections have been made yet.
            pass


    def SyncViewWithModel(self, model):
        # First unsync from the existing model.
        self.UnsyncViewWithModel()

        # Now actually set the new model as the view's model.
        self.view.setModel(model)

        # Get the index of each item that should be expanded in
        # this view and expand it.
        for i in range(model.ExpandedPrimPathsCount()):
            index = model.GetIndexOfExpandedPrimPath(i)
            if index:
                self.view.expand(index)

        # Set up connections between the model and the view.
        model.treeTopologyChanged.connect(self.OnTreeTopologyChanged)
        model.expandedPrimPathAdded.connect(self.view.expand)
        model.expandedPrimPathRemoved.connect(self.view.collapse)
        model.showingVariantsSwitched.connect(\
            self.switchShowVariants.setCheckState)

        self.view.expanded.connect(model.AddExpandedPrimPath)
        self.view.collapsed.connect(model.RemoveExpandedPrimPath)
        self.switchShowVariants.stateChanged.connect(\
            model.SetShowingVariants)

        # Emit the model's ShowingVariants state to update this view.
        model.EmitShowingVariants()

        # Update the new model's items as they're just becoming visible.
        topIndex = model.index(0, 0, QModelIndex())
        if topIndex.isValid():
            self.BecameVisible(topIndex)

    def BecameVisible(self, index):
        self.OpenPersistentEditors(index)
        self.ResetBackgroundColors(index)

    def OnTreeTopologyChanged(self, index):
        if self.view.isExpanded(index):
            self.BecameVisible(index)

    def OnNodeDeleted(self, **kwargs):
        node = kwargs['node']
        self.activeNodeMenu.RemoveNodeChoice(node)

        key = node.sessionId()
        model = hou.session.UsdImportDict.get(key, None)
        if model:
            if model == self.view.model():
                self.SyncViewWithModel(TreeView.emptyModel)
            model.ClearAll(node)
            hou.session.UsdImportDict.pop(key)

    def OnNodeRenamed(self, **kwargs):
        self.activeNodeMenu.RenameNodeChoice(kwargs['node'])

    def OnParmChanged(self, **kwargs):
        parm = kwargs['parm_tuple']
        if not parm:
            return

        parmName = parm.name()
        model = self.view.model()

        if parmName == model._nameUsdFile:
            node = kwargs['node']
            model.BuildAll(node, clearExisting=True)
            self.SyncViewWithModel(model)

        elif parmName == model._namePrimPaths:
            node = kwargs['node']
            model.CopyImportedPrimPathsFromNode(node)

    def OpenPersistentEditors(self, index):
        row = index.row()
        self.view.openPersistentEditor(index.sibling(row, COL_IMPORT))
        self.view.openPersistentEditor(index.sibling(row, COL_VARIANT))

        # If this item is expanded, recursively open
        # persistent editors for its children.
        if self.view.isExpanded(index):
            rowCount = self.view.model().rowCount(index)
            for row in range(rowCount):
                self.OpenPersistentEditors(index.child(row, 0))

    #
    # This method is needed for setting the background color on widgets
    # whose display is governed by the custom delegate.
    #
    # The native (non-delegate) widgets have their background colors
    # applied by calling setAlternatingRowColors(True) on the view.
    # This method mimics that behavior so that all widgets in a row
    # are displayed with matching color.
    #
    def ResetBackgroundColors(self, firstIndex):
        if not firstIndex.isValid():
            return

        # Local method def for recursively selecting color.
        def SelectBackgroundColor(item, select):
            index = self.view.model().indexFromItem(item)
            item.setRowAlternator(select)
            rgb = TreeView.rgbRows[select]

            row = index.row()
            widget = self.view.indexWidget(index.sibling(row, COL_IMPORT))
            if widget:
                widget.setStyleSheet('QFrame{background-color: %s;}' % rgb)
            widget = self.view.indexWidget(index.sibling(row, COL_VARIANT))
            if widget:
                widget.setStyleSheet('QFrame{background-color: %s;}' % rgb)
            select = 1 - select

            if self.view.isExpanded(index):
                for i in range(item.childCount()):
                    select = SelectBackgroundColor(item.child(i), select)

            return select

        firstItem = firstIndex.internalPointer()
        select = firstItem.rowAlternator()
        row = firstItem.row()
        parentItem = firstItem.parent()
        while parentItem:
            for i in range(row, parentItem.childCount()):
                select = SelectBackgroundColor(parentItem.child(i), select)
            row = parentItem.row() + 1
            parentItem = parentItem.parent()

    def ShowVariants(self, state):
        if state == Qt.Checked:
            self.view.showColumn(COL_VARIANT)
        elif state == Qt.Unchecked:
            self.view.hideColumn(COL_VARIANT)

        topIndex = self.view.model().index(0, 0, QModelIndex())
        if topIndex.isValid():
            self.UpdateSizeHints(topIndex)

    def UpdateSizeHints(self, index):
        # Only items that have variants will ever
        # need to change their size hints.
        if len(index.internalPointer().data(COL_VARIANT)) > 0:
            self.view.itemDelegate(index).sizeHintChanged.emit(index)

        # If this item is expanded, recursively
        # update size hints for its children.
        if self.view.isExpanded(index):
            rowCount = self.view.model().rowCount(index)
            for row in range(rowCount):
                self.UpdateSizeHints(index.child(row, 0))

    def OnStyleChanged(self):
        ExtractColorsFromStyleSheet()

        # Update the custom widgets in each row of the view.
        topIndex = self.view.model().index(0, 0, QModelIndex())
        if topIndex.isValid():
            self.ResetBackgroundColors(topIndex)
