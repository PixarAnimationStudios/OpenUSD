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
NodeTypeLowerCase = 'usdimport'

#
# Return a list of hou.NodeType that are considered the only
# compatible node types that this plugin will work with.
#
def CompatibleNodeTypes():
    nodeTypes = []

    for category in [hou.objNodeTypeCategory, hou.sopNodeTypeCategory]:
        for key,value in category().nodeTypes().items():
            if NodeTypeLowerCase not in key.lower():
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

    grpBG = '([\s]+background(-color)?: )'
    grpRgb = '(?P<rgb>rgb(a)?\(\d{1,3}(, \d{1,3}){2,3}\))'

    # For the TreeView's "editing" base color (the color inside
    # checkboxes, inside text editing fields, etc) extract the
    # background color of the QTextEdit widget.
    grpQTextEdit = '(QTextEdit\n\{[\s\S]*?)'
    matchBG = re.search(grpQTextEdit + grpBG + grpRgb, qtStyleSheet)
    if matchBG:
        # Convert it from a string to a QColor.
        baseColor = matchBG.groupdict()['rgb'].strip('rgba()')
        c = [int(channel) for channel in baseColor.split(',')]
        if len(c) == 3:
            TreeView.editingBaseColor = QColor(c[0], c[1], c[2])
        elif len(c) == 4:
            TreeView.editingBaseColor = QColor(c[0], c[1], c[2], c[3])

class CheckBoxStyled(QCheckBox):
    def paintEvent(self, event):
        opt = QStyleOptionButton()
        self.initStyleOption(opt)

        style = self.style()
        opt.rect = style.subElementRect(QStyle.SE_CheckBoxClickRect, opt, self)

        # Set 'base' color (the color inside the box).
        opt.palette.setColor(QPalette.Base, TreeView.editingBaseColor)

        painter = QStylePainter(self)
        painter.drawControl(QStyle.CE_CheckBox, opt)

    def nextCheckState(self):
        if self.checkState() == Qt.Checked:
            self.setCheckState(Qt.Unchecked)
        else:
            # Not Checked, which means either Unchecked or PartiallyChecked.
            # The next state for either of these is Checked.
            self.setCheckState(Qt.Checked)

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

class FilterMenu(ComboBoxStyled):
    # Signals
    textEntered = Signal('QString')

    def __init__(self, parent):
        super(FilterMenu, self).__init__(parent)

        self.setEditable(True)

        self.addItem("Clear Filter")
        self.clearEditText()
        self.text = self.currentText()

        self.activated[int].connect(self.OnActivated)
        self.lineEdit().returnPressed.connect(self.EnterText)

    def EnterText(self):
        if self.text != self.currentText():
            self.text = self.currentText()
        self.textEntered.emit(self.text)

    def OnActivated(self, index):
        if index == 0:
            self.clearEditText()
        self.EnterText()

    def OnStyleChanged(self):
        styleSheet = 'QComboBox { padding: 2px;'
        channels = [str(c) for c in TreeView.editingBaseColor.getRgb()]
        if len(channels) == 3:
            styleSheet += ' background: rgb(%s);' % ','.join(channels)
        elif len(channels) == 4:
            styleSheet += ' background: rgba(%s);' % ','.join(channels)
        styleSheet += '}'
        self.setStyleSheet(styleSheet)

class ActiveNodeMenu(ComboBoxStyled):
    # Signals
    activeNodeChanged = Signal('QString')

    def __init__(self, parent):
        super(ActiveNodeMenu, self).__init__(parent)

        # Set minimum contents length to 16 characters.
        self.setMinimumContentsLength(16)
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
                label.setStyleSheet("background: transparent;")
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

    def displayText(self, value, locale):
        if not isinstance(value, basestring):
            return ''
        return super(TreeItemDelegate, self).displayText(value, locale)

    def createEditor(self, parent, option, index):
        editor = TreeItemEditor(parent, index)
        editor.edited.connect(self.commitAndCloseEditor)
        return editor

    def setEditorData(self, editor, index):
        editor.SetData(index)

    def setModelData(self, editor, model, index):
        model.setData(index, editor.GetData(index))

    def sizeHint(self, option, index):
        width = 0
        if index.column() == COL_NAME:
            (style, opt) = self.SetupStyleOption(option, index)
            margin = style.pixelMetric(QStyle.PM_FocusFrameHMargin,\
                                       opt, opt.widget) + 1
            width = QFontMetrics(opt.font).width(opt.text) + margin
        return QSize(width, TreeItemEditor.ItemHeight(index))

    def commitAndCloseEditor(self):
        editor = self.sender()
        self.commitData.emit(editor)
        self.closeEditor.emit(editor, QAbstractItemDelegate.NoHint)

    def paint(self, painter, option, index):
        if index.column() == COL_NAME:
            (style, opt) = self.SetupStyleOption(option, index)
            (widget, text) = opt.widget, opt.text

            # Draw the expand/collapse control for this item. Clear the opt's
            # text so that no text will be drawn yet. Text will be drawn next.
            opt.text = ''
            style.drawControl(QStyle.CE_ItemViewItem, opt, painter, widget)

            # Prepare an empty FormatRange, and grab the filterString from
            # the TreeView widget.
            formatRange = QTextLayout.FormatRange()
            filterString = '' if widget is None\
                            else widget.parent().filterMenu.currentText()
            # If filterString matches this item, set up the formatRange
            # to draw the matching section as highlighted text.
            if len(filterString) > 1:
                match = index.internalPointer().matchesFilter(filterString)
                if match is not None:
                    formatRange.start, formatRange.length = match

                    # Make highlight color a bit more opaque than the default.
                    highlight = opt.palette.color(QPalette.Highlight)
                    highlight.setAlphaF(0.7)
                    opt.palette.setColor(QPalette.Highlight, highlight)
                    formatRange.format.setBackground(opt.palette.highlight())

            # Make the text elided (replace end with '...') if it is too wide.
            rect = style.subElementRect(QStyle.SE_ItemViewItemText, opt, widget)
            metrics = QFontMetrics(opt.font)
            if metrics.width(text) > rect.width():
                text = metrics.elidedText(text, opt.textElideMode, rect.width())

            # Draw the text (with the section that matches filter highlighted).
            textLayout = QTextLayout(text, opt.font)
            textLayout.beginLayout()
            textLayout.createLine()
            textLayout.endLayout()
            textLayout.draw(painter, rect.topLeft(), [formatRange], rect)
        else:
            super(TreeItemDelegate, self).paint(painter, option, index)

    # Helper function for setting up a QStyle and QStyleOptionViewItem.
    def SetupStyleOption(self, option, index):
        opt = QStyleOptionViewItem(option)
        self.initStyleOption(opt, index)
        style = qApp.style() if opt.widget is None else opt.widget.style()
        return (style, opt)

class TreeView(QFrame):
    # Signals
    styleChanged = Signal()

    # Declare nodeTypes as a static member of TreeView.
    nodeTypes = CompatibleNodeTypes()

    # Static model to use for views with no "Active Node".
    emptyModel = TreeModel(COL_HEADERS)

    # This initial default will be updated by ExtractColorsFromStyleSheet()
    editingBaseColor = QColor(19, 19, 19)

    def __init__(self):
        QFrame.__init__(self)

        # Houdini styling is not automatically applied to custom widgets.
        # Set the "houdiniStyle" property to True on this top-level widget
        # so it matches the style of other houdini widgets.
        self.setProperty("houdiniStyle", True)

        # Keep a list of prim paths that are expanded in this tree view.
        self.expandedPrimPaths = []

        expandToImported = QPushButton("+", self)
        expandToImported.setMaximumSize(QSize(20,20))
        expandToImported.setToolTip("Expand tree to show all imported prims.")
        expandToImported.clicked.connect(self.OnExpandToImported)

        self.filterMenu = FilterMenu(self)
        self.filterMenu.textEntered['QString'].connect(self.OnFilterApplied)
        self.styleChanged.connect(self.filterMenu.OnStyleChanged)

        self.switchShowVariants = CheckBoxStyled()
        self.switchShowVariants.stateChanged.connect(self.ShowVariants)

        self.activeNodeMenu = ActiveNodeMenu(self)
        self.activeNodeMenu.activeNodeChanged.connect(\
            self.OnActiveNodeChanged)

        toolbarLayout = QHBoxLayout()
        toolbarLayout.setSpacing(4)
        toolbarLayout.addWidget(expandToImported)
        toolbarLayout.addSpacing(8)
        toolbarLayout.addWidget(QLabel("Filter"))
        toolbarLayout.addWidget(self.filterMenu, 1)
        toolbarLayout.addSpacing(20)
        toolbarLayout.addWidget(self.switchShowVariants)
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

        self.view.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.view.setContextMenuPolicy(Qt.CustomContextMenu)
        self.view.customContextMenuRequested.connect(self.PopupRightClickMenu)
        self.view.installEventFilter(self)

        self.view.setAlternatingRowColors(True)
        self.view.verticalScrollBar().valueChanged.connect(\
            self.view.updateGeometries)
        self.view.horizontalScrollBar().valueChanged.connect(\
            self.view.updateGeometries)

        self.view.expanded.connect(self.OnExpanded)
        self.view.collapsed.connect(self.OnCollapsed)

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
            ExtractColorsFromStyleSheet()
            self.styleChanged.emit()
        super(TreeView, self).changeEvent(event)

    def closeEvent(self, event):
        try:
            hou.ui.removeEventLoopCallback(self.CheckNodeSelection)
        except hou.Error:
            pass
        self.UnsyncViewWithModel()
        event.accept()

    def keyPressEvent(self, event):
        if event.key() == Qt.Key_Escape and\
            self.view.selectionModel() is not None:
            self.view.selectionModel().clearSelection()
        super(TreeView, self).keyPressEvent(event)

    def eventFilter(self, source, event):
        if event.type() == QEvent.KeyPress and\
            event.matches(QKeySequence.Copy):
            self.OnSelectionCopied()
            return True
        return super(TreeView, self).eventFilter(source, event)

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

    def UnsyncViewWithModel(self):
        # Attempt to disconnect the current model.
        model = self.view.model()
        try:
            node = model.GetNode()
            if node is not None:
                node.removeEventCallback(\
                    (hou.nodeEventType.BeingDeleted,), self.OnNodeDeleted)
                node.removeEventCallback(\
                    (hou.nodeEventType.NameChanged,), self.OnNodeRenamed)
                node.removeEventCallback(\
                    (hou.nodeEventType.ParmTupleChanged,), self.OnParmChanged)

        except RuntimeError:
            # This error happens the first time this method is called
            # because none of these connections have been made yet.
            pass

        # Clear the expandedPrimPaths stored in the view
        self.expandedPrimPaths = []

    def SyncViewWithModel(self, model):
        # First unsync from the existing model.
        self.UnsyncViewWithModel()

        # Now actually set the new model as the view's model.
        self.view.setModel(model)
        self.view.setSelectionModel(model.GetSelectionModel())

        # Open persistent editors for the top index.
        topIndex = model.index(0, 0, QModelIndex())
        if topIndex.isValid():
            self.OpenPersistentEditors(topIndex)

        expandState = ''
        node = model.GetNode()
        if node is not None:
            node.addEventCallback(\
                (hou.nodeEventType.BeingDeleted,), self.OnNodeDeleted)
            node.addEventCallback(\
                (hou.nodeEventType.NameChanged,), self.OnNodeRenamed)
            node.addEventCallback(\
                (hou.nodeEventType.ParmTupleChanged,), self.OnParmChanged)

            expandState = node.parm(TreeModel.parmUiExpandState).eval()

        # Get the list of indexes that should be expanded
        # from expandState, then expand each of them.
        if expandState != '':
            self.expandedPrimPaths = expandState.split()
            # Sort the expandedPrimPaths using the ComparePaths
            # method (which is defined in the treemodel module).
            self.expandedPrimPaths.sort(cmp=ComparePaths)
            for primPath in self.expandedPrimPaths:
                index = model.GetIndexFromPrimPath(primPath)
                if index.isValid():
                    self.view.expand(index)
        else:
            # If there are no paths specified to be expanded, the
            # default is to expand the first 2 levels of the tree.
            self.view.expandToDepth(2)

        # Apply the current filter to this tree view.
        self.OnFilterApplied(self.filterMenu.currentText())

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

    def OnExpanded(self, index):
        model = self.view.model()
        item = model.itemFromIndex(index)

        primPath = item.primPath().pathString
        if primPath not in self.expandedPrimPaths:
            self.expandedPrimPaths.append(primPath)
            self.CopyExpandedPrimPathsToNode()

        # If the item being expanded has no children AND
        # has an unloaded payload, load that payload now.
        if item.childCount() == 0 and item.hasUnloadedPayload():
            model.LoadPrimAndBuildTree(model.GetPrim(item), item)

        # Now that this item is expanded, open
        # persistent editors for its children.
        for row in range(model.rowCount(index)):
            self.OpenPersistentEditors(index.child(row, 0))

        # Increase the columnWidth, if needed.
        columnWidth = self.view.sizeHintForColumn(COL_NAME)
        if self.view.columnWidth(COL_NAME) < columnWidth:
            self.view.setColumnWidth(COL_NAME, columnWidth)

    def OnCollapsed(self, index):
        model = self.view.model()
        item = model.itemFromIndex(index)

        primPath = item.primPath().pathString
        if primPath in self.expandedPrimPaths:
            self.expandedPrimPaths.remove(primPath)
            self.CopyExpandedPrimPathsToNode()

    def CopyExpandedPrimPathsToNode(self):
        node = self.view.model().GetNode()
        if node is not None:
            parm = node.parm(TreeModel.parmUiExpandState)
            # Note this parm never gets set to the empty string by this
            # function. There will always be at least one '\n'. This is to
            # help differentiate between cases when the parm is still set to
            # its initial empty value and cases when all former entries have
            # been removed (collapsed).
            parm.set('\n'.join(self.expandedPrimPaths) + '\n')

    def OpenPersistentEditors(self, index):
        row = index.row()
        self.view.openPersistentEditor(index.sibling(row, COL_IMPORT))
        self.view.openPersistentEditor(index.sibling(row, COL_VARIANT))

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

    def PopupRightClickMenu(self, pos):
        if self.focusWidget() == self.view:
            menu = QMenu(self)
            importAction = menu.addAction("Import")
            unimportAction = menu.addAction("Unimport")
            action = menu.exec_(self.view.mapToGlobal(pos))
            if action is not None:
                state = Qt.Checked if action == importAction else Qt.Unchecked
                for index in self.view.selectedIndexes():
                    # Skip items from every column except the import column.
                    if index.column() == COL_IMPORT:
                        self.view.model().setData(index, state, Qt.EditRole)

    def OnSelectionCopied(self):
        indexes = self.view.selectionModel().selectedRows()
        if len(indexes) == 0:
            return

        # If there is more than one selected index, sort them.
        if len(indexes) > 1:
            profiles = []
            for index in indexes:
                profile = ''
                while index.isValid():
                    profile = str(index.row()) + profile
                    index = index.parent()
                profiles.append(profile)
            zipped = zip(profiles, indexes)
            zipped.sort()
            profiles, indexes = zip(*zipped)

        selection = ''
        for i, index in enumerate(indexes):
            item = index.internalPointer()
            if i > 0:
                selection += '\n'
            selection += self.view.model().VariantPrimPathFromItem(item)

        qApp.clipboard().setText(selection)

    def OnExpandToImported(self):
        for index in self.view.model().GetImportedIndexes():
            parent = index.parent()
            while parent.isValid():
                if self.view.isRowHidden(index.row(), parent):
                    self.view.setRowHidden(index.row(), parent, False)
                self.view.expand(parent)
                index = parent
                parent = index.parent()

    def OnFilterApplied(self, filterString):
        # Helper method to traverse the tree and hide all rows that don't
        # match filterString.
        def Hide(index):
            rowHidden = []
            rowCount = self.view.model().rowCount(index)
            for row in range(rowCount):
                rowHidden.append(Hide(index.child(row, 0)))

            if False in rowHidden:
                # This index has at least one row to be unhidden, so expand
                # the index to make sure its unhidden row(s) will be showing.
                self.view.expand(index)

                # Now go through each row to actually hide or unhide it. (Note
                # that only siblings of unhidden rows are ever truly hidden.
                # Rows without unhidden siblings are just collapsed instead).
                for row in range(rowCount):
                    self.view.setRowHidden(row, index, rowHidden[row])

                # Return False to indicate this index will NOT be hidden.
                return False
            else:
                # This index has only hidden rows. Instead of hiding them
                # (which essentially removes them), just collapse this index
                # so the rows aren't immediately visible.
                self.view.collapse(index)

                # Now go through each row and actually *unhide* any that may
                # be currently hidden. This may seem counter-intuitive, but
                # this is done to have as few removed rows as possible. Having
                # them collapsed is enough to hide them.
                for row in range(rowCount):
                    if self.view.isRowHidden(row, index):
                        self.view.setRowHidden(row, index, False)

                # Check if this index itself matches filterString so its parent
                # can be notified whether to hide or unhide it.
                match = index.internalPointer().matchesFilter(filterString)
                return (match == None)

        # Helper method to traverse the tree and unhide all items.
        def Unhide(index):
            if self.view.isRowHidden(index.row(), index.parent()):
                self.view.setRowHidden(index.row(), index.parent(), False)
            for row in range(self.view.model().rowCount(index)):
                Unhide(index.child(row, 0))

        # If filterString is only 1 char, do nothing. (A single char would
        # match far too many paths to be useful and it could get pretty slow.)
        if len(filterString) == 1:
            return

        topIndex = self.view.model().index(0, 0, QModelIndex())
        # topIndex will be invalid when model is the emptyModel.
        if not topIndex.isValid():
            return

        # If filterString is empty, unhide all items.
        if filterString == '':
            Unhide(topIndex)
            return

        # Recursively hide items that don't match filterString.
        hide = Hide(topIndex)
        self.view.setRowHidden(topIndex.row(), topIndex.parent(), hide)
