#
# Copyright 2022 Pixar
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

'''
Module that implements python version of HydraSceneBrowser.

NOTE: Source code for a C++ version of this widget is available (without open
      source build support) in extras/imaging/examples/hdui.

      That version (even wrapped in python as a whole) is strongly recommended
      as the basis for any application integration as it is higher performance.
'''

from .qt import QtCore, QtWidgets, QtGui
from pxr import Sdf
from ._usdviewq import (
    HydraObserver,
    DataSourceLocator,
    DataSourceLocatorSet,
    ContainerDataSource,
    VectorDataSource,
    SampledDataSource,
)

# -----------------------------------------------------------------------------

class HydraSceneBrowser(QtWidgets.QWidget):
    def __init__(self, parent=None):
        QtWidgets.QWidget.__init__(self, parent)
        self.setWindowTitle("Hydra Scene Browser")

        self.__mainLayout = QtWidgets.QVBoxLayout(self)
        self.__toolbarLayout = QtWidgets.QHBoxLayout()
        self.__mainLayout.addLayout(self.__toolbarLayout)
        self.__siChooser = _RegisteredSceneIndexChooser(self)
        self.__siChooser.sceneIndexSelected.connect(self.__SceneIndexSelected)

        self.__goToInputButton = QtWidgets.QPushButton("Inputs")
        self.__goToInputButtonMenu = QtWidgets.QMenu()
        self.__goToInputButton.setMenu(self.__goToInputButtonMenu)

        self.__goToInputButtonMenu.aboutToShow.connect(
            self.__FillGoToInputMenu)

        self.__nameLabel = QtWidgets.QLabel()

        self.__loggerButton = QtWidgets.QPushButton("Show Notice Logger")
        self.__loggerWidget = None

        self.__toolbarLayout.addWidget(self.__siChooser)
        self.__toolbarLayout.addWidget(self.__goToInputButton)
        self.__toolbarLayout.addWidget(self.__nameLabel, 10)
        self.__toolbarLayout.addStretch()
        self.__toolbarLayout.addWidget(self.__loggerButton)

        self.__observer = HydraObserver()


        self.__splitter = QtWidgets.QSplitter(QtCore.Qt.Horizontal)
        self.__mainLayout.addWidget(self.__splitter, 10)

        self.__siTreeWidget = _SceneIndexTreeWidget(self.__observer)
        self.__splitter.addWidget(self.__siTreeWidget)

        self.__dsTreeWidget = _DataSourceTreeWidget(self)
        self.__splitter.addWidget(self.__dsTreeWidget)

        self.__valueTreeView = _DataSourceValueTreeView(self)
        self.__splitter.addWidget(self.__valueTreeView)

        self.__siTreeWidget.primSelected.connect(self.__PrimSelected)
        self.__dsTreeWidget.dataSourceSelected.connect(
            self.__DataSourceSelected)

        self.__siTreeWidget.primDirtied.connect(self.__PrimDirtied)

        self.__loggerButton.clicked.connect(self.__ShowLoggerWindow)

        self.__noticePollingTimer = QtCore.QTimer()
        self.__noticePollingTimer.setInterval(16)
        self.__noticePollingTimer.timeout.connect(self.__PollForNotices)
        self.__noticePollingTimer.start()

    def __PollForNotices(self):
        if not self.__observer.HasPendingNotices():
            return

        notices = self.__observer.GetPendingNotices()
        if self.__loggerWidget:
            self.__loggerWidget.AddNotices(notices)

        for noticeType, entries in notices:
            if noticeType == 'added':
                self.__siTreeWidget.PrimsAdded(entries)
            elif noticeType == 'removed':
                self.__siTreeWidget.PrimsRemoved(entries)
            elif noticeType == 'dirtied':
                self.__siTreeWidget.PrimsDirtied(entries)

    def __SceneIndexSelected(self, name):
        self.__observer.TargetToNamedSceneIndex(name)
        self.__UpdateNameLabel(name)
        self.__SceneIndexChanged()

    def __SceneIndexChanged(self):
        # XXX for initial testing, clear and build /root
        self.__siTreeWidget.Requery(self.__observer)


    def __PrimSelected(self, primPath, dataSourceProxy):
        self.__valueTreeView.SetDataSource(None)
        self.__dsTreeWidget.SetPrimDataSource(primPath, dataSourceProxy)

    def __PrimDirtied(self, primPath, locators):
        primType, dataSource = self.__observer.GetPrim(primPath)
        self.__dsTreeWidget.PrimDirtied(primPath, dataSource, locators)

    def __DataSourceSelected(self, dataSourceProxy):
        self.__valueTreeView.SetDataSource(dataSourceProxy.value)

    def __FillGoToInputMenu(self):
        menu = self.__goToInputButtonMenu;
        menu.clear()
        menuTreeWidget = QtWidgets.QTreeWidget()
        menuTreeWidget.setHeaderHidden(True);
        menuTreeWidget.setAllColumnsShowFocus(True);
        menuTreeWidget.setMouseTracking(True);
        menuTreeWidget.setSizeAdjustPolicy(
            QtWidgets.QAbstractScrollArea.SizeAdjustPolicy.\
                AdjustToContentsOnFirstShow)

        def _ItemEntered(item):
            menuTreeWidget.setCurrentItem(item, 0,
                QtCore.QItemSelectionModel.Select |
                    QtCore.QItemSelectionModel.Clear)

        menuTreeWidget.itemEntered.connect(_ItemEntered)

        def _ItemClicked(item, column):
            if hasattr(item, 'indices'):
                self.__observer.TargetToInputSceneIndex(item.indices)
                self.__UpdateNameLabel()
                self.__SceneIndexChanged()
            menu.close()

        menuTreeWidget.itemClicked.connect(_ItemClicked)

        queue = [([], menuTreeWidget)]

        while queue:
            indices, itemParent = queue.pop()
            names = self.__observer.GetInputDisplayNames(indices)
            for i, name in enumerate(names):
               item = QtWidgets.QTreeWidgetItem(itemParent, [name])
               item.indices = indices + [i]
               item.setExpanded(True)
               queue.append((item.indices, item))

        widgetAction = QtWidgets.QWidgetAction(menu)
        widgetAction.setDefaultWidget(menuTreeWidget)

        menu.addAction(widgetAction)

    def __UpdateNameLabel(self, prefix=None):
        value = "<b><i>( %s )</i></b> %s" % (self.__observer.GetDisplayName(),
            '' if prefix is None else prefix)
        self.__nameLabel.setText(value)

        if self.__loggerWidget:
            self.__loggerWidget.SetText(value)

        self.__goToInputButton.setEnabled(
            bool(self.__observer.GetInputDisplayNames([])))

    def __ShowLoggerWindow(self):
        if not self.__loggerWidget:
            self.__loggerWidget = _NoticeLoggingWidget(None)
            self.__loggerWidget.SetText(self.__nameLabel.text())

        self.__loggerWidget.show()


# -----------------------------------------------------------------------------

class _RegisteredSceneIndexChooser(QtWidgets.QPushButton):
    sceneIndexSelected = QtCore.Signal(str)

    def __init__(self, parent=None):
        QtWidgets.QPushButton.__init__(self, "Choose Scene Index", parent)

        self.__menu = QtWidgets.QMenu()
        self.setMenu(self.__menu)


        self.__menu.aboutToShow.connect(self.__AboutToShow)
        self.__menu.triggered.connect(self.__Triggered)

    def __AboutToShow(self):
        self.__menu.clear()
        names = HydraObserver.GetRegisteredSceneIndexNames()
        if names:
            for name in names:
                self.__menu.addAction(name)
        else:
            self.__menu.addAction("No Registered Names").setEnabled(False)

    def __Triggered(self, action):
        self.sceneIndexSelected.emit(str(action.text()))

# -----------------------------------------------------------------------------

# holds a container data source or None
class _DataSourceProxy:
    def __init__(self, value=None):
        self.value = value

class _SceneIndexTreeWidget(QtWidgets.QTreeWidget):
    primSelected = QtCore.Signal(Sdf.Path, _DataSourceProxy)
    primDirtied = QtCore.Signal(Sdf.Path, DataSourceLocatorSet)

    def __init__(self, observer, parent=None):
        QtWidgets.QTreeWidget.__init__(self, parent)
        self.__observer = observer
        self.__expandedSet = set()
        self.__primItems = {}

        self.setHeaderLabels(["Name", "Type"])

        header = self.header()
        header.setSectionResizeMode(0, header.ResizeMode.Stretch);
        header.setSectionResizeMode(1, header.ResizeMode.Fixed);
        header.resizeSection(1, self.fontMetrics().averageCharWidth() * 10);
        header.setStretchLastSection(False);

        self.itemSelectionChanged.connect(self.__ItemSelectionChanged)
        self.itemExpanded.connect(self.__ItemExpanded)
        self.itemCollapsed.connect(self.__ItemCollapsed)

    def contextMenuEvent(self, event):
        item = self.itemAt(event.pos())

        if item:
            globalPos = QtCore.QPoint(event.pos().x(),
                self.visualItemRect(item).bottom())

            if self.header().isVisible():
                globalPos = QtCore.QPoint(
                    globalPos.x(), globalPos.y() + self.header().height())

            if isinstance(item, _SceneIndexPrimTreeWidgetItem):
                menu = QtWidgets.QMenu()
                menu.addAction("type: " + str(item.text(1))).setEnabled(False)
                menu.addSeparator()
                menu.exec_(self.mapToGlobal(globalPos))

    def PrimsAdded(self, entries):
        for primPath, primType in entries:
            item = self._GetPrimItem(primPath)
            item.setText(1, primType)
            if item.isSelected():
                self.itemSelectionChanged.emit()

    def PrimsRemoved(self, entries):
        sortState = self.isSortingEnabled()

        try:
            for primPath in entries:
                item = self._GetPrimItem(primPath, False)
                if item:
                    parentItem = item.parent()
                    if parentItem is not None:
                        parentItem.takeChild(parentItem.indexOfChild(item))
                    
                    # XXX Items are currently stored flat so this loop will not
                    #     scale for lots of items
                    self._RemoveSubtree(primPath)

        finally:
            self.setSortingEnabled(sortState)

    def PrimsDirtied(self, entries):
        items = self.selectedItems()
        if not items:
            return

        item = items[0]
        if not isinstance(item, _SceneIndexPrimTreeWidgetItem):
            return

        selectedPath = item.GetPrimPath()

        selectedItemLocators = DataSourceLocatorSet()
        for primPath, dirtiedLocators in entries:
            if primPath == selectedPath:
                selectedItemLocators.insert(dirtiedLocators)

        self.primDirtied.emit(selectedPath, selectedItemLocators)

    def _GetPrimItem(self, primPath, createIfNecessary=True):
        if primPath in self.__primItems:
            return self.__primItems[primPath]

        if not createIfNecessary:
            return None

        if primPath == Sdf.Path.absoluteRootPath:
            parentItem = self.invisibleRootItem()
        else:
            parentItem = self._GetPrimItem(primPath.GetParentPath(), True)

        if parentItem is None:
            return None

        item = _SceneIndexPrimTreeWidgetItem(parentItem, primPath)
        self.__primItems[primPath] = item
        return item

    def _AddPrimItem(self, primPath, item):
        self.__primItems[primPath] = item

    def _RemoveSubtree(self, primPath):
        item = self._GetPrimItem(primPath, createIfNecessary=False)
        if item is None:
            return

        itemParent = item.parent()
        if itemParent is not None:
            itemParent.takeChild(itemParent.indexOfChild(item))

        self._RemoveSubtreeEntries(primPath)

    def _RemoveSubtreeEntries(self, primPath):
        toRemove = []
        for itemPrimPath in self.__primItems:
            if itemPrimPath.HasPrefix(primPath):
                toRemove.append(primPath)

        for p in toRemove:
            if p in self.__primItems:
                del self.__primItems[p]

    def Requery(self, observer, lazy=True):
        self.clear()
        self.__primItems.clear()
        self.__primItems[Sdf.Path.absoluteRootPath] = (
            _SceneIndexPrimTreeWidgetItem(self.invisibleRootItem(),
                Sdf.Path.absoluteRootPath, True))

    def PrimIsExpanded(self, primPath):
        return primPath in self.__expandedSet

    def AddToExpandedSet(self, primPath):
        self.__expandedSet.add(primPath)

    def RemoveFromExpandedSet(self, primPath):
        self.__expandedSet.discard(primPath)

    def __ItemSelectionChanged(self):
        items = self.selectedItems()
        if not items:
            self.primSelected.emit(Sdf.Path(), _DataSourceProxy())
            return

        item = items[0]
        if isinstance(item, _SceneIndexPrimTreeWidgetItem):
            primPath = item.GetPrimPath()
            primType, dataSource = self.__observer.GetPrim(primPath)
            self.primSelected.emit(item.GetPrimPath(),
                _DataSourceProxy(dataSource))

    def __ItemExpanded(self, item):
        if isinstance(item, _SceneIndexPrimTreeWidgetItem):
            item.WasExpanded(self, self.__observer)

    def __ItemCollapsed(self, item):
        if isinstance(item, _SceneIndexPrimTreeWidgetItem):
            self.RemoveFromExpandedSet(item.GetPrimPath())

# -----------------------------------------------------------------------------

class _SceneIndexPrimTreeWidgetItem(QtWidgets.QTreeWidgetItem):
    def __init__(self, parent, primPath, queryOnExpansion=False):
        QtWidgets.QTreeWidgetItem.__init__(self, parent)
        self.__primPath = primPath
        self.__queryOnExpansion = queryOnExpansion

        if queryOnExpansion:
            self.setChildIndicatorPolicy(
                self.ChildIndicatorPolicy.ShowIndicator)

        if primPath.IsPropertyPath():
            name = "." + primPath.name
        else:
            name = primPath.name

        self.setText(0, name)

        if self.treeWidget().PrimIsExpanded(primPath):
            self.setExpanded(True)

    def GetPrimPath(self):
        return self.__primPath

    def WasExpanded(self, treeWidget, observer):
        treeWidget.AddToExpandedSet(self.__primPath)

        if not self.__queryOnExpansion:
            return

        self.__queryOnExpansion = False

        count = self.childCount()
        if count:
            for i in range(0, count):
                childItem = self.child(i)
                if isinstance(childItem, _SceneIndexPrimTreeWidgetItem):
                    treeWidget._RemoveSubtree(childItem)

        for childPath in observer.GetChildPrimPaths(self.GetPrimPath()):
            childPrimType, childPrimDataSource = observer.GetPrim(childPath)
            childItem = _SceneIndexPrimTreeWidgetItem(self, childPath, True)
            childItem.setText(1, childPrimType)
            treeWidget._AddPrimItem(childPath, childItem)

        if not self.childCount():
            self.setChildIndicatorPolicy(
                self.ChildIndicatorPolicy.DontShowIndicator)

# -----------------------------------------------------------------------------

class _DataSourceTreeWidget(QtWidgets.QTreeWidget):
    dataSourceSelected = QtCore.Signal(_DataSourceProxy)

    def __init__(self, parent):
        QtWidgets.QTreeWidget.__init__(self, parent)

        self.setHeaderLabels(['Name'])
        self.setAllColumnsShowFocus(True)
        self.__expandedSet = set()

        self.itemExpanded.connect(self.__ItemExpanded)
        self.itemCollapsed.connect(self.__ItemCollapsed)
        self.itemSelectionChanged.connect(self.__ItemSelectionChanged)

    def SetPrimDataSource(self, primPath, dataSource):
        self.clear()
        if dataSource.value is not None:
            item = _DataSourceTreeWidgetItem(
                DataSourceLocator(),
                self.invisibleRootItem(),
                dataSource.value)
            item.setText(0, '<%s>' % (primPath.name,))

    def PrimDirtied(self, primPath, dataSource, locatorSet):
        # loop over existing items to determine which require data source
        # updates

        taskQueue = [self.topLevelItem(0)]

        while taskQueue:
            item = taskQueue[-1]
            del taskQueue[-1]

            if not isinstance(item, _DataSourceTreeWidgetItem):
                continue

            loc = item.GetLocator()

            addChildren = False

            if not loc.IsEmpty():
                if locatorSet.Contains(loc):
                    # dirty here, we'll need a new data source
                    # no need to add children as SetDirty will handle that
                    item.SetDirty(dataSource.Get(loc))
                elif locatorSet.Intersects(loc):
                    addChildren = True
            else:
                addChildren = True

            if addChildren:
                for i in range(item.childCount()):
                    taskQueue.append(item.child(i))


        # Force a selection change on the current item so that the value column
        # re-pulls on the data source
        items = self.selectedItems()
        if items:
            item = items[0]
            if isinstance(item, _DataSourceTreeWidgetItem):
                if locatorSet.Intersects(item.GetLocator()):
                    self.itemSelectionChanged.emit()


    def _GetExpandedSet(self):
        return self.__expandedSet

    def __ItemExpanded(self, item):
        if isinstance(item, _DataSourceTreeWidgetItem):
            item.WasExpanded()

    def __ItemCollapsed(self, item):
        if isinstance(item, _DataSourceTreeWidgetItem):
            item.WasCollapsed()
    
    def __ItemSelectionChanged(self):
        items = self.selectedItems()
        if not items:
            return

        item = items[0]
        if not isinstance(item, _DataSourceTreeWidgetItem):
            return

        self.dataSourceSelected.emit(_DataSourceProxy(item.GetDataSource()))

# -----------------------------------------------------------------------------

class _DataSourceTreeWidgetItem(QtWidgets.QTreeWidgetItem):
    def __init__(self, locator, parentItem, dataSource):
        QtWidgets.QTreeWidgetItem.__init__(self, parentItem)
        self.__locator = locator
        self.__dataSource = dataSource
        self.__childrenBuilt = False

        if not locator.IsEmpty():
            self.setText(0, locator.GetLastElement())

        if isinstance(self.__dataSource, (ContainerDataSource,
                VectorDataSource,)):
            self.setChildIndicatorPolicy(
                self.ChildIndicatorPolicy.ShowIndicator)
        else:
            self.setChildIndicatorPolicy(
                self.ChildIndicatorPolicy.DontShowIndicator)

        if self.__IsInExpandedSet():
            #TODO, determine whether this is necessary to defer anymore
            self.setExpanded(True)

    def __IsInExpandedSet(self):
        return self.__locator in self.treeWidget()._GetExpandedSet()

    def __SetIsInExpandedSet(self, state):
        if state:
            self.treeWidget()._GetExpandedSet().add(self.__locator)
        else:
            self.treeWidget()._GetExpandedSet().discard(self.__locator)

    def WasExpanded(self):
        self.__SetIsInExpandedSet(True)

        if self.__childrenBuilt:
            return

        self.__BuildChildren()

    def WasCollapsed(self):
        self.__SetIsInExpandedSet(False)

    def SetDirty(self, dataSource):
        if self.__childrenBuilt:
            if isinstance(dataSource, ContainerDataSource):
                # confirm that existing data source is also a container
                # if not, rebuild entirely
                if not isinstance(self.__dataSource, ContainerDataSource):
                    self.__dataSource = dataSource
                    self.__RebuildChildren()
                    return

                usedNames = set()
                itemsToRemove = []

                for i in range(self.childCount()):
                    childItem = self.child(i)
                    if not isinstance(childItem, _DataSourceTreeWidgetItem):
                        continue

                    childName = childItem.GetLocator().GetLastElement()
                    childDs = dataSource.Get(childName)
                    usedNames.add(childName)

                    if childDs is not None:
                        childItem.SetDirty(childDs)
                    else:
                        itemsToRemove.append(childItem)

                # add any new items
                for childName in dataSource.GetNames():
                    if childName not in usedNames:
                        childDs = dataSource.Get(childName)
                        if childDs is not None:
                            _DataSourceTreeWidgetItem(
                                self.__locator.Append(childName),
                                self,
                                childDs)

                for item in itemsToRemove:
                    self.takeChild(self.indexOfChild(item))
            elif isinstance(dataSource, VectorDataSource):

                # confirm that existing data source is also a vector
                # of the same length (could reuse items but probably not
                # worth the extra complexity)
                if (not isinstance(self.__dataSource, VectorDataSource) or
                        self.childCount() != dataSource.GetNumElements()):
                    self.__dataSource = dataSource
                    self.__RebuildChildren()
                    return


                for i in range(dataSource.GetNumElements()):
                    childItem = self.child(i)
                    if isinstance(childItem, _DataSourceTreeWidgetItem):
                        childItem.SetDirty(dataSource.GetElement(i))

        self.__dataSource = dataSource

    def GetDataSource(self):
        return self.__dataSource

    def GetLocator(self):
        return self.__locator

    def __RebuildChildren(self):
        self.takeChildren()
        self.__BuildChildren()
        
    def __BuildChildren(self):
        self.__childrenBuilt = True
        
        if isinstance(self.__dataSource, ContainerDataSource):
            usedNames = set()
            for childName in self.__dataSource.GetNames():
                if childName in usedNames:
                    continue
                usedNames.add(childName)
                childDataSource = self.__dataSource.Get(childName)
                if childDataSource is not None:
                    _DataSourceTreeWidgetItem(
                        self.__locator.Append(childName),
                        self,
                        childDataSource)

        elif isinstance(self.__dataSource, VectorDataSource):
            for i in range(self.__dataSource.GetNumElements()):
                childName = 'i%i' % (i,)
                _DataSourceTreeWidgetItem(
                    self.__locator.Append(childName),
                    self,
                    self.__dataSource.GetElement(i))

# -----------------------------------------------------------------------------

class _ValueItemModel(QtCore.QAbstractItemModel):
    def __init__(self, value, typeStr, parent=None):
        QtCore.QAbstractItemModel.__init__(self, parent)
        self.__value = value
        self.__isArray = hasattr(value, 'AsList')
        self.__typeStr = typeStr

    # base is good for scalars we'll use str() on the held type
    def data(self, modelIndex, role=QtCore.Qt.DisplayRole):
        if role != QtCore.Qt.DisplayRole:
            return None

        if self.__isArray:
            if  modelIndex.row() <= len(self.__value):
                if modelIndex.column() == 0:
                    return str(self.__value[modelIndex.row()])
                else:
                    return modelIndex.row()
        else:
            if modelIndex.row() == 0 and modelIndex.column() == 0:
                return str(self.__value)

        return None

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):

        if role == QtCore.Qt.DisplayRole:
            if section == 0:
                return self.__typeStr
            elif section == 1 and self.__isArray:
                return "Index"

        return None

    def parent(self, model):
        return QtCore.QModelIndex()

    def columnCount(self, parent=QtCore.QModelIndex()):
        if self.__isArray:
            return 2
        return 1

    def rowCount(self, parent=QtCore.QModelIndex()):
        if parent.isValid() or parent.column() > 0:
            return 0

        if self.__isArray:
            return len(self.__value)

        return 1

    def index(self, row, column, parent=None):
        return self.createIndex(row, column)

# -----------------------------------------------------------------------------

class _DataSourceValueTreeView(QtWidgets.QTreeView):
    def __init__(self, parent):
        QtWidgets.QTreeView.__init__(self, parent)
        self.__dataSource = None
        self.setUniformRowHeights(True)
        self.setItemsExpandable(False)


    def SetDataSource(self, dataSource):
        self.__dataSource = dataSource
        if isinstance(dataSource, SampledDataSource):

            # NOTE: GetTypeString internally has to call GetValue so this
            #       is happening twice. If that's a performance issue ever,
            #       we can add GetValueAndTypeString to get both at once.
            self.setModel(_ValueItemModel(dataSource.GetValue(0.0),
                dataSource.GetTypeString(), self))

            header = self.header()
            header.setSectionResizeMode(0, header.ResizeMode.Stretch)
            if header.count() > 1:
                header.setSectionResizeMode(1, header.ResizeMode.Fixed)
                header.resizeSection(1,
                    self.fontMetrics().averageCharWidth() * 10)
                header.setStretchLastSection(False)
            else:
                header.setStretchLastSection(True)
        else:
            self.setModel(None)

    def Refresh(self):
        self.SetDataSource(self.__dataSource)

# -----------------------------------------------------------------------------

class _NoticeLoggingWidget(QtWidgets.QWidget):
    def __init__(self, parent):
        QtWidgets.QWidget.__init__(self, parent)

        self.__mainLayout = QtWidgets.QVBoxLayout(self)
        self.__toolbarLayout = QtWidgets.QHBoxLayout()
        self.__mainLayout.addLayout(self.__toolbarLayout)

        self.__startStopButton = QtWidgets.QPushButton("Start Recording")
        self.__clearButton = QtWidgets.QPushButton("Clear")

        self.__label = QtWidgets.QLabel('', self)

        self.__toolbarLayout.addWidget(self.__startStopButton)
        self.__toolbarLayout.addWidget(self.__label, 10)
        self.__toolbarLayout.addStretch()
        self.__toolbarLayout.addWidget(self.__clearButton)

        self.__recording = False

        self.__treeView = _ObserverLoggingTreeView(self)
        self.__mainLayout.addWidget(self.__treeView)


        self.__clearButton.clicked.connect(self.__Clear)

        self.__startStopButton.clicked.connect(self.__ToggleRecordingState)

    def AddNotices(self, entries):
        if not self.__recording:
            return
        self.__treeView.GetModel().AddEntries(entries)

    def SetText(self, value):
        self.__label.setText(value)

    def __Clear(self):
        self.__treeView.GetModel().Clear()

    def __ToggleRecordingState(self):
        self.__recording = not self.__recording
        self.__startStopButton.setText(
            "Start Recording" if not self.__recording else "Stop Recording")


# -----------------------------------------------------------------------------

class _ObserverLoggingTreeView(QtWidgets.QTreeView):
    def __init__(self, parent=None):
        QtWidgets.QTreeView.__init__(self, parent)
        self.__model = _ObserverLoggingItemModel(self)
        self.setMinimumWidth(512)
        self.setUniformRowHeights(True)
        self.setModel(self.__model)
        self.header().resizeSection(0, 384)

    def GetModel(self):
        return self.__model

# -----------------------------------------------------------------------------


class _ObserverLoggingItemModel(QtCore.QAbstractItemModel):

    def __init__(self, parent):
        QtCore.QAbstractItemModel.__init__(self, parent)
        self.__entries = []

    def AddEntries(self, newEntries):
        if not newEntries:
            return

        beforeLength = len(self.__entries)
        self.beginInsertRows(QtCore.QModelIndex(), beforeLength,
            beforeLength + len(newEntries) - 1)
        
        for i, newEntry in enumerate(newEntries):
            # stow the index into the entry in service of modelIndex
            # "internalPointer"
            self.__entries.append((i + beforeLength, newEntry))

        self.endInsertRows()

    def Clear(self):
        self.beginResetModel()
        self.__entries.clear()
        self.endResetModel()

    def data(self, modelIndex, role=QtCore.Qt.DisplayRole):
        if role != QtCore.Qt.DisplayRole:
            return None

        internalPtr = modelIndex.internalPointer()
        if internalPtr:
            parentIndex, (entryType, entryValues) = internalPtr
            entryValue = entryValues[modelIndex.row()]

            if modelIndex.column() == 0:
                if entryType == 'removed':
                    return str(entryValue)
                return str(entryValue[0])

            elif modelIndex.column() == 1:
                if entryType == 'dirtied':
                    return entryValue[1].AsString()
                elif entryType == 'added':
                    return str(entryValue[1])

        else:
            if modelIndex.column() == 0:
                # return the notice type string
                return self.__entries[modelIndex.row()][1][0]

        return None

    def headerData(self, section, orientation, role=QtCore.Qt.DisplayRole):
        if role == QtCore.Qt.DisplayRole:
            if section == 0:
                return "Notice Type/ Prim Path"
            elif section == 1:
                return "Value"

        return None

    def index(self, row, column, parent=None):
        if parent.isValid():
            # safeguard against child of child

            if parent.internalPointer():
                return QtCore.QModelIndex()

            return self.createIndex(row, column, self.__entries[parent.row()])

        if row >= 0 and row < len(self.__entries):
            return self.createIndex(row, column, None)

        return QtCore.QModelIndex()

    def parent(self, modelIndex):
        internalPtr = modelIndex.internalPointer()
        # top-level items have no pointer so won't have a parent
        if not internalPtr:
            return QtCore.QModelIndex()

        return self.createIndex(internalPtr[0], modelIndex.row(), None)

    def columnCount(self, parent=QtCore.QModelIndex()):
        return 2

    def rowCount(self, parent=QtCore.QModelIndex()):
        if parent.column() > 0:
            return 0

        if parent.isValid():
            internalPtr = parent.internalPointer()
            if internalPtr:
                return 0
            else:
                return len(self.__entries[parent.row()][1][1])

        return len(self.__entries)




