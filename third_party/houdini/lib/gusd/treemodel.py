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

from pxr import Usd, Sdf, UsdGeom, Ar

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

    def matchesFilter(self, filterString):
        if len(filterString) == 0:
            return None
        #
        # In order for primPath to be considered a match with filterString,
        # filterString has to match at least part of the actual name shown
        # on the row in the treeview. In other words, the match can't be
        # completetly from the ancestor part of primPath; it has to overlap
        # the piece that follows the final '/'.
        #
        filterHead, slash, filterTail =\
            filterString.lower().rpartition('/')

        ancestorPath, slash, name =\
            self._primPath.pathString.lower().rpartition('/')

        start = name.find(filterTail)
        if start == -1 or\
            (start == 0 and not ancestorPath.endswith(filterHead)) or\
            (start > 0 and len(filterTail) < len(filterString)):
            return None

        # Return the position in name where the match begins,
        # and also the length of the matching section.
        return (start, len(filterTail))

class TreeModel(QAbstractItemModel):
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

        # _importedPrimPaths is the list of prim paths that are switched on
        # in the tree view's import column, and matches the import node's
        # _namePrimPaths parameter.
        self._importedPrimPaths = []

        self._sessionId = -1
        self._nameUsdFile = ''
        self._namePrimPaths = ''

        # This model will own a QItemSelectionModel so it can
        # be shared by all views that operate on this model.
        self._selectionModel = QItemSelectionModel(self)

        self.BuildAll(node)

    def BuildAll(self, node, clearExisting=False):
        if clearExisting:
            self.ClearAll(node)

        if node:
            self._sessionId = node.sessionId()
            self._nameUsdFile = node.parm(TreeModel.parmNameUsdFile).eval()
            self._namePrimPaths = node.parm(TreeModel.parmNamePrimPaths).eval()

            usdFile = node.parm(self._nameUsdFile).eval()
            resolverContext = Ar.GetResolver().GetCurrentContext()
            try:
                self._stage = Usd.Stage.Open(usdFile, resolverContext,\
                                             Usd.Stage.LoadNone)
            except:
                if usdFile != '':
                    print('Error: Tree View failed to open "%s"' % usdFile)

        if self._stage:
            rootPrim = self._stage.GetPseudoRoot()
            primPath = rootPrim.GetPath()

            self._primPathToItemMap[primPath] = self._rootItem
            self.BuildTree(rootPrim)

            # Copy the import state from the node and apply it to the new tree.
            self.CopyImportedPrimPathsFromNode(node)

    def ClearAll(self, node):
        if node:
            # Clear node's list of expanded items.
            node.parm(TreeModel.parmUiExpandState).set('')

        self.beginResetModel()
        self._rootItem.clearChildren()
        self._stage = None
        self._primPathToItemMap.clear()
        self._importedPrimPaths = []
        self._sessionId = -1
        self._nameUsdFile = ''
        self._namePrimPaths = ''
        self._selectionModel.reset()
        self.endResetModel()

    def SyncModelWithNode(self, node):
        # Check if the node's usdFile parameter resolves to a different
        # path than that stored in self._stage's rootLayer. If they differ,
        # clear and rebuild this model.
        paramPath = node.parm(self._nameUsdFile).eval()
        storedPath = self._stage.GetRootLayer().realPath if self._stage else ''
        if Ar.GetResolver().Resolve(paramPath) != storedPath:
            self.BuildAll(node, clearExisting=True)
        else:
            # The node's usdFile hasn't changed, but its primPaths may
            # have, so copy them from the node to the this model.
            self.CopyImportedPrimPathsFromNode(node)

    def IsPrimBoundable(self, prim, predicate):
        if prim.IsA(UsdGeom.Boundable):
            return True

        # If this prim has a payload, consider it boundable.
        if prim.HasPayload():
            return True

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

                # Use parentItem's import state to determine its child's
                # import state. (Note it is intentional that the parentItem's
                # data is tested to be equal to Checked or PartiallyChecked,
                # instead of just testing that it's *not* equal to Unchecked.
                # This is because when parentItem is the top-most root item,
                # its data is a header string instead of a CheckState).
                importState = Qt.Unchecked
                if parentItem.data(COL_IMPORT) == Qt.Checked or\
                    parentItem.data(COL_IMPORT) == Qt.PartiallyChecked:
                        importState = Qt.PartiallyChecked

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

                data = [primName, importState, primTypeName, variants]
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
        return self.itemFromIndex(parent).columnCount()

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

        return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return self._rootItem.data(section)

        return None

    def index(self, row, column, parent):
        if not self.hasIndex(row, column, parent):
            return QModelIndex()

        parentItem = self.itemFromIndex(parent)

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

    def itemFromIndex(self, index):
        if index.isValid():
            return index.internalPointer()
        else:
            return self._rootItem

    def indexFromItem(self, item, column = 0):
        if not item or item == self._rootItem:
            return QModelIndex()

        return self.createIndex(item.row(), column, item)

    def rowCount(self, parent):
        if parent.column() > 0:
            return 0

        return self.itemFromIndex(parent).childCount()

    def setData(self, index, value, role=Qt.EditRole):
        if role != Qt.EditRole:
            return False

        column = index.column()

        if column == COL_IMPORT:
            return self.ChangeImport(index, value)

        elif column == COL_VARIANT:
            return self.ChangeVariants(index, value)

        else:
            item = self.itemFromIndex(index)
            if item.setData(column, value):
                self.EmitDataChanged(index)
                return True
            return False

    def hasChildren(self, parent):
        item = self.itemFromIndex(parent)

        # Report that this parent has children if either its
        # childCount is above zero, or it has an unloaded payload.
        return item.childCount() > 0 or item.hasUnloadedPayload()

    def SetImportState(self, item, state):
        # If attempting to unimport an item that has an imported parent,
        # set the item's state to PartiallyChecked instead of Unchecked.
        if state == Qt.Unchecked:
            parent = item.parent()
            if parent is not None and parent.data(COL_IMPORT) != Qt.Unchecked:
                state = Qt.PartiallyChecked

        item.setData(COL_IMPORT, state)
        index = self.createIndex(item.row(), COL_IMPORT, item)
        self.EmitDataChanged(index)

        for child in [item.child(i) for i in range(item.childCount())]:
            # Only continue unimporting children that are PartiallyChecked.
            # (So, if child is Checked or Unchecked, the recursion stops).
            if state == Qt.Unchecked and\
                child.data(COL_IMPORT) == Qt.PartiallyChecked:
                self.SetImportState(child, Qt.Unchecked)

            # Only continue importing children that are Unchecked. (So, if
            # child is Checked or PartiallyChecked, the recursion stops).
            elif state != Qt.Unchecked and\
                child.data(COL_IMPORT) == Qt.Unchecked:
                self.SetImportState(child, Qt.PartiallyChecked)

    def GetPrim(self, item):
        if not self._stage:
            return None
        return self._stage.GetPrimAtPath(item.primPath())

    def GetNode(self):
        if self._sessionId != -1:
            return hou.nodeBySessionId(self._sessionId)
        else:
            return None

    def LoadPrimAndBuildTree(self, prim, item):
        if prim:
            if not prim.IsLoaded():
                prim.Load()
            item.setHasUnloadedPayload(False)
            self.BuildTree(prim)

    def ChangeImport(self, index, value):
        indexesToChange = []
        indexIsSelected = False

        # Get all selected indexes that are in the 'Import' column.
        for selected in self._selectionModel.selectedIndexes():
            if selected.column() == COL_IMPORT:
                indexesToChange.append(selected)

            # If the provided index is part of the selection, set a flag.
            if selected == index:
                indexIsSelected = True

        # If the provided index is not part of the selection, then replace
        # the indexesToChange list with only the provided index.
        if not indexIsSelected:
            indexesToChange = [index]

        # Helper function for items that get unchecked as a result
        # of a selected item changing its import/unimport state.
        def UncheckNonSelected(item):
            # First make sure the item is imported. If not, do nothing.
            if item.data(COL_IMPORT) != Qt.Checked:
                return
            # Now, if the item is NOT currently selected, unimport it.
            if self.indexFromItem(item, COL_IMPORT) not in indexesToChange:
                self.RemoveImportedPrimPath(self.VariantPrimPathFromItem(item))
                self.SetImportState(item, Qt.Unchecked)

        changed = False
        for item in [self.itemFromIndex(i) for i in indexesToChange]:
            # If new value is same as current value, skip this item.
            if value == item.data(COL_IMPORT):
                continue

            if value == Qt.Unchecked:
                self.RemoveImportedPrimPath(self.VariantPrimPathFromItem(item))

            elif value == Qt.Checked:
                self.AddImportedPrimPath(self.VariantPrimPathFromItem(item))

                # When an item gets Checked, also Uncheck all of its ancestors.
                # (Since any Checked ancestor would mean this item is already
                # in an imported state, the assumption is that the user means
                # to keep only this item imported, and unimport its siblings).
                # This behavior is overridden by including in the current
                # selection any parents that are meant to remain Checked.
                parent = item.parent()
                while parent is not None:
                    UncheckNonSelected(parent)
                    parent = parent.parent()

                # Likewise, when an item gets Checked, also Uncheck all of its
                # children. (This is meant to prevent having duplicate items
                # imported, since importing an item will also import all of its
                # children). Again, this behavior is overridden by including in
                # the current selection any children that are meant to remain
                # Checked.
                children = [item.child(i) for i in range(item.childCount())]
                for child in children:
                    UncheckNonSelected(child)
                    children.extend(\
                        [child.child(i) for i in range(child.childCount())])

            self.SetImportState(item, value)
            changed = True
        return changed

    def ChangeVariants(self, index, newVariants):
        item = self.itemFromIndex(index)

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
                        self.SetImportState(pathItem, Qt.Checked)
                        importedPrimPathsChanged = True

            if importedPrimPathsChanged:
                self.CopyImportedPrimPathsToNode()

            self.EmitDataChanged(index)
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

                item.setData(COL_VARIANT, newVariants)
                self.EmitDataChanged(self.indexFromItem(item))

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
        if not node:
            node = self.GetNode()

        if node:
            parm = node.parm(self._namePrimPaths)

            src = '\n'.join(self._importedPrimPaths)
            dst = parm.eval()

            if src != dst:
                parm.set(src)

    def CopyImportedPrimPathsFromNode(self, node=None):
        if not node:
            node = self.GetNode()

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
                            self.SetImportState(item, Qt.Unchecked)

                # Clear the self._importedPrimPaths list.
                self._importedPrimPaths = []

                # Append each path from srcImportedPrimPaths into
                # self._importedPrimPaths.
                for primPath in srcImportedPrimPaths:
                    self._importedPrimPaths.append(primPath)

                    item = self.MakeTreeIncludePrimPath(Sdf.Path(primPath))
                    if item:
                        self.SetImportState(item, Qt.Checked)

    def GetImportedIndexes(self):
        indexes = []
        for primPath in self._importedPrimPaths:
            item = self.ItemFromVariantPrimPath(primPath)
            indexes.append(self.indexFromItem(item))
        return indexes

    def GetIndexFromPrimPath(self, primPath):
        item = self._primPathToItemMap.get(Sdf.Path(primPath), None)
        if item:
            return self.indexFromItem(item)
        return QModelIndex()

    def EmitDataChanged(self, index):
        # The PySide2 version of the dataChanged signal requires a
        # 3rd argument. Check the houdini version number to determine
        # whether to use the PySide or PySide2 version.
        if hou.applicationVersion()[0] >= 16:
            self.dataChanged.emit(index, index, [])
        else:
            self.dataChanged.emit(index, index)

    def GetSelectionModel(self):
        return self._selectionModel
