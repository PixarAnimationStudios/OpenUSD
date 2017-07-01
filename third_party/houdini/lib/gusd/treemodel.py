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

from pxr import Usd, Sdf, UsdGeom

#
# Column headers and ID numbers
#
COL_HEADERS = ("Name", "Import", "Type", "Variants", "")
COL_NAME, COL_IMPORT, COL_TYPE, COL_VARIANT, COL_END = range(len(COL_HEADERS))

# Method for comparing 2 Sdf.Path objects.
def ComparePaths(x, y):
    xPath = Sdf.Path(x).StripAllVariantSelections()
    yPath = Sdf.Path(y).StripAllVariantSelections()
    return xPath.pathElementCount - yPath.pathElementCount

class VariantInfo(object):
    def __init__(self, name, choices, initialSelection, enabled):
        self._name = name
        self._choices = choices
        self._initialSelection = initialSelection
        self._currentSelection = initialSelection
        self._enabled = enabled

class TreeItem(object):
    def __init__(self, data, parent, primPath, hasUnloadedPayload):
        self._data = data
        self._parent = parent
        self._primPath = primPath
        self._hasUnloadedPayload = hasUnloadedPayload
        self._children = []

        if parent:
            parent._children.append(self)

        self._rowAlternator = 0

    def children(self):
        return self._children

    def child(self, row):
        return self._children[row]

    def childCount(self):
        return len(self._children)

    def clearChildren(self):
        self._children = []

    def columnCount(self):
        return len(self._data)

    def data(self, column):
        try:
            return self._data[column]
        except IndexError:
            return None

    def parent(self):
        return self._parent

    def row(self):
        if self._parent:
            return self._parent._children.index(self)
        return 0
    
    def setData(self, column, value):
        if column < 0 or column >= len(self._data):
            return False

        self._data[column] = value
        return True

    def primPath(self):
        return self._primPath

    def hasUnloadedPayload(self):
        return self._hasUnloadedPayload

    def setHasUnloadedPayload(self, hasUnloadedPayload):
        self._hasUnloadedPayload = hasUnloadedPayload

    def setRowAlternator(self, rowAlternator):
        self._rowAlternator = rowAlternator

    def rowAlternator(self):
        return self._rowAlternator

class TreeModel(QAbstractItemModel):
    # Signals
    expandedPrimPathAdded = Signal(QModelIndex)
    expandedPrimPathRemoved = Signal(QModelIndex)
    treeTopologyChanged = Signal(QModelIndex)
    showingVariantsSwitched = Signal(Qt.CheckState)

    # Static members
    parmNameUsdFile = '_parmname_usdfile'
    parmNamePrimPaths = '_parmname_primpaths'
    parmUiExpandState = '_parm_uiexpandstate'

    def __init__(self, headers, node=None):
        super(TreeModel, self).__init__(parent=None)

        self._rootItem = TreeItem(headers,\
                                  parent=None,\
                                  primPath='/',\
                                  hasUnloadedPayload=False)
        self._stage = None
        self._primPathToItemMap = {}

        # Maintain 2 lists of prim paths:
        # 1. _importedPrimPaths is the list of prim paths that are switched on
        #    in the tree view's import column, and matches the import node's
        #    _namePrimPaths parameter.
        # 2. _expandedPrimPaths is the list of prim paths that are expanded in
        #    the tree view, and matches the import node's _parm_uiexpandstate
        #    parameter.
        self._importedPrimPaths = []
        self._expandedPrimPaths = []

        self._sessionId = -1
        self._nameUsdFile = ''
        self._namePrimPaths = ''

        # Flag that determines whether views connected to
        # this model are showing the variants column or not.
        self._showingVariants = Qt.Unchecked

        self.BuildAll(node)

    def BuildAll(self, node, clearExisting=False):
        if clearExisting:
            self.ClearAll(node)

        if node:
            self._sessionId = node.sessionId()
            self._nameUsdFile = node.parm(TreeModel.parmNameUsdFile).eval()
            self._namePrimPaths = node.parm(TreeModel.parmNamePrimPaths).eval()

            usdFile = node.parm(self._nameUsdFile).eval()
            try:
                self._stage = Usd.Stage.Open(usdFile, Usd.Stage.LoadNone)
            except:
                if usdFile != '':
                    print('Error: Tree View failed to open "%s"' % usdFile)

        if self._stage:
            rootPrim = self._stage.GetPseudoRoot()
            primPath = rootPrim.GetPath()

            self._primPathToItemMap[primPath] = self._rootItem
            self.BuildTree(rootPrim)

            # Copy the import state and the expansion state
            # from the node and apply it to the new tree.
            self.CopyImportedPrimPathsFromNode(node)
            self.CopyExpandedPrimPathsFromNode(node)

    def ClearAll(self, node):
        if node:
            # Clear node's list of expanded items.
            node.parm(TreeModel.parmUiExpandState).set('')

        self.beginResetModel()
        self._rootItem.clearChildren()
        self._stage = None
        self._primPathToItemMap.clear()
        self._importedPrimPaths = []
        self._expandedPrimPaths = []
        self._sessionId = -1
        self._nameUsdFile = ''
        self._namePrimPaths = ''
        self._showingVariants = Qt.Unchecked
        self.endResetModel()

    def IsPrimBoundable(self, prim, predicate):
        if prim.IsA(UsdGeom.Boundable):
            return True

        # If this prim has a payload, consider it boundable.
        if prim.HasPayload():
            return True

        if prim.IsInstance():
            return self.IsPrimBoundable(prim.GetMaster(), predicate)

        for child in prim.GetFilteredChildren(predicate):
            if self.IsPrimBoundable(child, predicate):
                return True

        return False

    def BuildTree(self, startPrim):
        startItem = self._primPathToItemMap.get(startPrim.GetPath())
        if not startItem:
            return

        # Add traversal of instances to predicate
        predicate = Usd.TraverseInstanceProxies(
                Usd.PrimIsActive & Usd.PrimIsDefined & ~Usd.PrimIsAbstract)
        childCount = len(startPrim.GetFilteredChildren(predicate))

        first = startItem.childCount()
        last = first + childCount - 1

        self.beginInsertRows(self.indexFromItem(startItem), first, last)

        prims = list(Usd.PrimRange(startPrim, predicate))

        # Iterate through prims, but skip the startPrim
        for prim in prims[1:]:
            if not self.IsPrimBoundable(prim, predicate):
                continue

            parentPrim = prim.GetParent()

            parentItem = self._primPathToItemMap.get(parentPrim.GetPath())
            if parentItem:
                
                primName = prim.GetName()
                primTypeName = prim.GetTypeName()
                primPath = prim.GetPath()
                hasUnloadedPayload = prim.HasPayload()

                # Retrieve variants from the prim.
                variants = []
                variantSets = prim.GetVariantSets()
                for name in variantSets.GetNames():
                    variantSet = variantSets.GetVariantSet(name)
                    variants.append(VariantInfo(\
                        name = name,\
                        choices = variantSet.GetVariantNames(),\
                        initialSelection = variantSet.GetVariantSelection(),\
                        enabled = False))

                data = [primName, Qt.Unchecked, primTypeName, variants]
                childItem = TreeItem(data, parentItem, primPath, hasUnloadedPayload)

                self._primPathToItemMap[primPath] = childItem

        self.endInsertRows()

    def ClearTree(self, item):
        # If the item has no children, there is nothing to do.
        if item.childCount() == 0:
            return

        # Traverse all of item's descendants, removing
        # them from _primPathToItemMap.
        descendants = item.children()

        for descendant in descendants:
            descendants.extend(descendant.children())

            self._primPathToItemMap.pop(descendant.primPath())

        self.beginRemoveRows(self.indexFromItem(item), 0, item.childCount())
        item.clearChildren()
        self.endRemoveRows()

    def columnCount(self, parent):
        if parent.isValid():
            return parent.internalPointer().columnCount()
        else:
            return self._rootItem.columnCount()

    def data(self, index, role):
        if not index.isValid():
            return None

        if role != Qt.DisplayRole and role != Qt.EditRole:
            return None

        item = index.internalPointer()

        return item.data(index.column())

    def flags(self, index):
        if not index.isValid():
            return Qt.NoItemFlags

        return Qt.ItemIsEnabled

    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self._rootItem.data(section)

        return None

    def index(self, row, column, parent):
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        if not parent.isValid():
            parentItem = self._rootItem
        else:
            parentItem = parent.internalPointer()

        childItem = parentItem.child(row)
        if childItem:
            return self.createIndex(row, column, childItem)
        else:
            return QModelIndex()

    def parent(self, index):
        if not index.isValid():
            return QModelIndex()

        childItem = index.internalPointer()
        parentItem = childItem.parent()

        return self.indexFromItem(parentItem)

    def indexFromItem(self, item, column = 0):
        if not item or item == self._rootItem:
            return QModelIndex()

        return self.createIndex(item.row(), column, item)

    def rowCount(self, parent):
        if parent.column() > 0:
            return 0

        if not parent.isValid():
            parentItem = self._rootItem
        else:
            parentItem = parent.internalPointer()
            if parentItem.childCount() == 0 and\
                parentItem.hasUnloadedPayload():
                self.LoadPrimAndBuildTree(self.GetPrim(parentItem), parentItem)

        return parentItem.childCount()

    def setData(self, index, value, role=Qt.EditRole):
        if role != Qt.EditRole:
            return False

        item = None
        if index.isValid():
            item = index.internalPointer()
        
        if not item:
            item = self._rootItem

        changed = False
        column = index.column()

        if column == COL_IMPORT:
            before = item.data(COL_IMPORT)
            item.setData(COL_IMPORT, value)
            after = item.data(COL_IMPORT)

            if before != after:
                variantPrimPath = self.VariantPrimPathFromItem(item)
                if after == Qt.Checked:
                    self.AddImportedPrimPath(variantPrimPath)
                elif after == Qt.Unchecked:
                    self.RemoveImportedPrimPath(variantPrimPath)
                changed = True

        elif column == COL_VARIANT:
            changed = self.ChangeVariants(item, value)

        else:
            changed = item.setData(column, value)

        if changed:
            self.EmitDataChanged(index)
        return changed

    def hasChildren(self, parent):
        item = None
        if parent.isValid():
            item = parent.internalPointer()
        else:
            item = self._rootItem

        if item:
            # Report that this parent has children if either its
            # childCount is above zero, or it has an unloaded payload.
            return item.childCount() > 0 or item.hasUnloadedPayload()
        return False

    # This is just a helper function because these same 3 lines
    # were used together several times throughout this class.
    def SetItemData(self, item, column, value):
        item.setData(column, value)
        index = self.createIndex(item.row(), column, item)
        self.EmitDataChanged(index)

    def GetPrim(self, item):
        if not self._stage:
            return None
        return self._stage.GetPrimAtPath(item.primPath())

    def LoadPrimAndBuildTree(self, prim, item):
        if prim:
            if not prim.IsLoaded():
                prim.Load()
            item.setHasUnloadedPayload(False)
            self.BuildTree(prim)

    def ChangeVariants(self, item, newVariants):
        # Store item's variantPrimPath and variants before they change.
        oldVariantPrimPath = self.VariantPrimPathFromItem(item)
        oldVariants = item.data(COL_VARIANT)

        # Set the new variants.
        item.setData(COL_VARIANT, newVariants)

        variantsEnabledChanged = False
        variantSelectionsChanged = False
        prim = self.GetPrim(item)

        for old, new in zip(oldVariants, newVariants):
            if new._enabled != old._enabled:
                variantsEnabledChanged = True

            if new._currentSelection != old._currentSelection:
                variantSelectionsChanged = True
                variantSet = prim.GetVariantSet(new._name)
                variantSet.SetVariantSelection(new._currentSelection)

        # If any variant selections changed AND if the prim is already loaded,
        # clear and rebuild this prim's sub tree. (If the prim hasn't been
        # loaded yet, then its sub tree hasn't been built yet and still doesn't
        # need to be).
        if variantSelectionsChanged and prim.IsLoaded():
            self.ClearTree(item)
            self.BuildTree(prim)
            self.treeTopologyChanged.emit(self.indexFromItem(item))

        # If any variants have been enabled or changed, iterate through the
        # paths in _importedPrimPaths to update their variant selections.
        if variantsEnabledChanged or variantSelectionsChanged:
            importedPrimPathsChanged = False
            paths = [Sdf.Path(path) for path in self._importedPrimPaths]
            for i, path in enumerate(paths):
                # Non-absolute paths (relative paths) are not supported.
                if not path.IsAbsolutePath():
                    continue

                # If a common prefix exists between oldVariantPrimPath and path
                # that IS oldVariantPrimPath, then this path either represents
                # item or a descendant of item.
                if path.GetCommonPrefix(oldVariantPrimPath) ==\
                    oldVariantPrimPath:
                    pathItem = self.ItemFromVariantPrimPath(path)
                    if pathItem:
                        self._importedPrimPaths[i] =\
                            self.VariantPrimPathFromItem(pathItem)
                        self.SetItemData(pathItem, COL_IMPORT, Qt.Checked)
                        importedPrimPathsChanged = True

            if importedPrimPathsChanged:
                self.CopyImportedPrimPathsToNode()

            return True
        return False

    def MakeTreeIncludePrimPath(self, primPath):
        if not self._stage:
            return None

        # Non-absolute paths (relative paths) are not supported.
        if not primPath.IsAbsolutePath():
            return None

        item = None

        paths = primPath.GetPrefixes()
        i = 0
        while i < len(paths):
            item = self._primPathToItemMap.get(\
                paths[i].StripAllVariantSelections(), None)

            # Not finding this item means this primPath can't be included,
            # so break from this loop and return None.
            if not item:
                break

            prim = self.GetPrim(item)
            newVariants = []
            oldVariants = item.data(COL_VARIANT)

            # Copy each oldVariant into newVariants. However, don't copy the
            # _enabled flag. Instead, start all the newVariants as disabled.
            for oldVariant in oldVariants:
                newVariants.append(VariantInfo(oldVariant._name,\
                                               oldVariant._choices,\
                                               oldVariant._initialSelection,\
                                               enabled = False))
            i += 1
            # Update prim and newVariants from variant selections in primPath.
            while i < len(paths) and paths[i].IsPrimVariantSelectionPath():
                name, selection = paths[i].GetVariantSelection()
                variantSet = prim.GetVariantSet(name)
                variantSet.SetVariantSelection(selection)

                for newVariant in newVariants:
                    if newVariant._name == name:
                        newVariant._enabled = True
                        newVariant._currentSelection = selection
                i += 1

            # Now check for changes between the old and new variants.
            variantsChanged = False
            for old, new in zip(oldVariants, newVariants):
                if old._enabled != new._enabled or\
                    old._currentSelection != new._currentSelection:
                    variantsChanged = True
                    break

            if variantsChanged:
                self.ClearTree(item)
                self.LoadPrimAndBuildTree(prim, item)
                self.treeTopologyChanged.emit(self.indexFromItem(item))
                self.SetItemData(item, COL_VARIANT, newVariants)

            elif item.hasUnloadedPayload():
                self.LoadPrimAndBuildTree(prim, item)

        return item

    def VariantPrimPathFromItem(self, item):
        parentItem = item.parent()
        if parentItem:
            parentPath = self.VariantPrimPathFromItem(parentItem)
            if not parentPath.endswith('}'):
                parentPath += '/'

            variants = item.data(COL_VARIANT)
            variantsString = ''
            for variantInfo in variants:
                if variantInfo._enabled:
                    variantsString += '{'
                    variantsString += variantInfo._name
                    variantsString += '='
                    variantsString += variantInfo._currentSelection
                    variantsString += '}'

            return parentPath + item.data(COL_NAME) + variantsString
        else:
            return ''

    def ItemFromVariantPrimPath(self, variantPrimPath):
        primPath = Sdf.Path(variantPrimPath).StripAllVariantSelections()
        return self._primPathToItemMap.get(primPath, None)

    def AddImportedPrimPath(self, primPath):
        if primPath not in self._importedPrimPaths:
            self._importedPrimPaths.append(primPath)
            self.CopyImportedPrimPathsToNode()

    def RemoveImportedPrimPath(self, primPath):
        if primPath in self._importedPrimPaths:
            self._importedPrimPaths.remove(primPath)
            self.CopyImportedPrimPathsToNode()

    def CopyImportedPrimPathsToNode(self, node=None):
        if not node and self._sessionId != -1:
            node = hou.nodeBySessionId(self._sessionId)

        if node:
            parm = node.parm(self._namePrimPaths)

            src = '\n'.join(self._importedPrimPaths)
            dst = parm.eval()

            if src != dst:
                parm.set(src)

    def CopyImportedPrimPathsFromNode(self, node=None):
        if not node and self._sessionId != -1:
            node = hou.nodeBySessionId(self._sessionId)

        if node:
            parm = node.parm(self._namePrimPaths)

            src = parm.eval()
            dst = '\n'.join(self._importedPrimPaths)

            if src != dst:
                srcImportedPrimPaths = src.split('\n')

                # First un-import any path in self._importedPrimPaths that
                # isn't included in srcImportedPrimPaths.
                for primPath in self._importedPrimPaths[:]:
                    if primPath not in srcImportedPrimPaths:
                        item = self.ItemFromVariantPrimPath(primPath)
                        if item:
                            self.SetItemData(item, COL_IMPORT, Qt.Unchecked)

                # Clear the self._importedPrimPaths list.
                self._importedPrimPaths = []

                # Append each path from srcImportedPrimPaths into
                # self._importedPrimPaths.
                for primPath in srcImportedPrimPaths:
                    self._importedPrimPaths.append(primPath)

                    item = self.MakeTreeIncludePrimPath(Sdf.Path(primPath))
                    if item:
                        self.SetItemData(item, COL_IMPORT, Qt.Checked)

    def ExpandedPrimPathsCount(self):
        return len(self._expandedPrimPaths)

    def GetIndexOfExpandedPrimPath(self, index):
        primPath = self._expandedPrimPaths[index]
        item = self._primPathToItemMap.get(Sdf.Path(primPath), None)
        if item:
            return self.indexFromItem(item)
        return QModelIndex()

    def AddExpandedPrimPath(self, index):
        if not index.isValid():
            return

        item = index.internalPointer()
        primPath = item.primPath().pathString

        if primPath not in self._expandedPrimPaths:
            self._expandedPrimPaths.append(primPath)
            self._expandedPrimPaths.sort(cmp=ComparePaths)
            self.expandedPrimPathAdded.emit(index)
            self.CopyExpandedPrimPathsToNode()

    def RemoveExpandedPrimPath(self, index):
        if not index.isValid():
            return

        item = index.internalPointer()
        primPath = item.primPath().pathString

        if primPath in self._expandedPrimPaths:
            self._expandedPrimPaths.remove(primPath)
            self.expandedPrimPathRemoved.emit(index)
            self.CopyExpandedPrimPathsToNode()

    def CopyExpandedPrimPathsToNode(self, node=None):
        if not node and self._sessionId != -1:
            node = hou.nodeBySessionId(self._sessionId)

        if node:
            parm = node.parm(TreeModel.parmUiExpandState)
            parm.set('\n'.join(self._expandedPrimPaths))

    def CopyExpandedPrimPathsFromNode(self, node=None):
        if not node and self._sessionId != -1:
            node = hou.nodeBySessionId(self._sessionId)

        if node:
            parm = node.parm(TreeModel.parmUiExpandState)
            src = parm.eval()
            if src != '':
                self._expandedPrimPaths = src.split('\n')

    def SetShowingVariants(self, show):
        if self._showingVariants != show:
            self._showingVariants = show
            self.EmitShowingVariants()

    def EmitShowingVariants(self):
        self.showingVariantsSwitched.emit(\
            Qt.CheckState(self._showingVariants))

    def EmitDataChanged(self, index):
        # The PySide2 version of the dataChanged signal requires a
        # 3rd argument. Check the houdini version number to determine
        # whether to use the PySide or PySide2 version.
        if hou.applicationVersion()[0] >= 16:
            self.dataChanged.emit(index, index, [])
        else:
            self.dataChanged.emit(index, index)
