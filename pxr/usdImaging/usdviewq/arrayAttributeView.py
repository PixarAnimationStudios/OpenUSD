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
from .qt import QtCore, QtGui, QtWidgets


def _GetLengthOfRange(start, stop, step):
    if step == 0:
        return 0

    k = 1 if step > 0 else -1
    num = 1 + (stop - k - start) / step

    # note, sign(stop - start) != sign(step) when we have an empty range.  In those
    # cases, "num" ends up non-positive, so we can just take the max(num, 0)
    return max(0, num)


class _ArrayAttributeModel(QtCore.QAbstractListModel):
    '''This is a data model that represents a slice into some array data.
    '''

    RawDataRole = QtCore.Qt.UserRole + 0

    def __init__(self):
        super(_ArrayAttributeModel, self).__init__()
        self._arrayData = None
        self._scalarTypeName = ""
        self._slice = slice(None)
        self._fetchMoreTimer = QtCore.QTimer()
        self._fetchMoreTimer.setInterval(1000)
        self._fetchMoreTimer.timeout.connect(self.TryToFetchMore)
        self._fetchMoreTimer.start()

        self._rowCount = 0
        self._publishedRows = 0

    def SetArrayDataAndTypeName(self, arrayData, scalarTypeName):
        self._arrayData = arrayData
        self._scalarTypeName = scalarTypeName
        self._Reset()

    def SetSlice(self, slice_):
        if self._slice == slice_:
            return
        self._slice = slice_
        self._Reset()

    def _Reset(self):
        self._publishedRows = 0
        self._rowCount = 0
        if self._arrayData is not None:
            self._rowCount = _GetLengthOfRange(*self._slice.indices(len(self._arrayData)))
        self.modelReset.emit()

    def GetArrayData(self):
        return self._arrayData

    def GetScalarTypeName(self):
        return self._scalarTypeName

    def index(self, row, col, parent=QtCore.QModelIndex()):
        return self.createIndex(row, col)

    def parent(self, index):
        return QtCore.QModelIndex()

    def rowCount(self, parent=QtCore.QModelIndex()):
        if not parent.isValid():
            return self._publishedRows
        return 0

    def columnCount(self, parent=QtCore.QModelIndex()):
        return 1

    def data(self, index, role=QtCore.Qt.DisplayRole):
        start, _, step = self._slice.indices(len(self._arrayData))
        idx = start + index.row() * step
        dataVal = self._arrayData[idx]

        if role == QtCore.Qt.DisplayRole:
            from .scalarTypes import ToString
            return str(idx) + ": " + ToString(
                dataVal, self._scalarTypeName)

        elif role == _ArrayAttributeModel.RawDataRole:
            return dataVal

        return None

    def fetchMore(self, index):
        left = self._rowCount - self._publishedRows
        toFetch = min(5000 if self._publishedRows == 0 else 100, left)
        self.beginInsertRows(
            index, self._publishedRows, self._publishedRows + toFetch - 1)
        self._publishedRows += toFetch
        self.endInsertRows()

    def canFetchMore(self, index):
        return self._publishedRows < self._rowCount

    def TryToFetchMore(self):
        rootIndex = QtCore.QModelIndex()
        if self.canFetchMore(rootIndex):
            self.fetchMore(rootIndex)


class ArrayAttributeView(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super(ArrayAttributeView, self).__init__(parent)

        layout = QtWidgets.QVBoxLayout()
        self.setLayout(layout)

        sliceLayout = QtWidgets.QHBoxLayout()
        layout.addLayout(sliceLayout)

        sliceLayout.addWidget(QtWidgets.QLabel("Slice:"))
        self._lineEdit = _SliceLineEdit()
        self._lineEdit.setPlaceholderText('e.g.: "0:5000", "::-1"')
        sliceLayout.addWidget(self._lineEdit)

        self._arrayAttrModel = _ArrayAttributeModel()
        self._listView = QtWidgets.QListView()
        self._listView.setUniformItemSizes(True)
        self._listView.setViewMode(QtWidgets.QListView.ListMode)
        self._listView.setSelectionMode(QtWidgets.QAbstractItemView.ExtendedSelection)
        self._listView.setModel(self._arrayAttrModel)
        layout.addWidget(self._listView)

        self._lineEdit.SliceChanged.connect(self._arrayAttrModel.SetSlice)

        self._SetupContextMenu()

    def SetAttribute(self, attr, frame):
        from .scalarTypes import GetScalarTypeFromAttr

        arrayData = attr.Get(frame)
        scalarTypeName, _ = GetScalarTypeFromAttr(attr)
        self._arrayAttrModel.SetArrayDataAndTypeName(arrayData, scalarTypeName)

    def CanView(self, attr):
        return attr.GetTypeName().isArray

    def keyPressEvent(self, e):
        # XXX note, this is extremely finicky.  it does not really
        # have keyboard focus.
        if e.matches(QtGui.QKeySequence.Copy):
            self.Copy()
        else:
            return super(ArrayAttributeView, self).keyPressEvent(e)

    # context menu stuff
    def _SetupContextMenu(self):
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        self.customContextMenuRequested.connect(self._ShowContextMenu)

    def _ShowContextMenu(self, point):
        menu = QtWidgets.QMenu(self)
        if self._listView.selectedIndexes():
            menu.addAction("Copy Selected", self.CopySelected)
        else:
            menu.addAction("Copy All", self.CopyAll)
        menu.addAction("Select All", self.SelectAll)
        menu.exec_(QtGui.QCursor.pos())

    def CopyAll(self):
        self._CopyValsToClipboard(self._arrayAttrModel.GetArrayData())

    def CopySelected(self):
        selectedIndexes = self._listView.selectedIndexes()
        self._CopyValsToClipboard([
            self._listView.model().data(idx, _ArrayAttributeModel.RawDataRole)
            for idx in selectedIndexes])

    def _CopyValsToClipboard(self, vals):
        from .scalarTypes import ToClipboard
        scalarTypeName = self._arrayAttrModel.GetScalarTypeName()
        copyText = "[ %s ]" % (", ".join(
            ToClipboard(val, scalarTypeName)
            for val in vals))
        QtWidgets.QApplication.clipboard().setText(copyText)

    def SelectAll(self):
        self._listView.selectAll()


def _IntOrNone(s):
    if not s:
        return None
    return int(s)


def _GetSliceFromString(s):
    s = slice(*map(_IntOrNone, s.split(':')))
    if s.step == 0:
        raise ValueError("Slice cannot be 0")
    return s


class _SliceLineEdit(QtWidgets.QLineEdit):
    """LineEdit for inputing strings that represent slices"""

    SliceChanged = QtCore.Signal(object)

    def __init__(self, parent=None):
        super(_SliceLineEdit, self).__init__(parent)
        self.setValidator(_SliceLineEdit.Validator())
        self.editingFinished.connect(self._OnEditingFinished)

    class Validator(QtGui.QValidator):
        def validate(self, s, pos):
            s = s.strip()
            try:
                _GetSliceFromString(s)
                return (QtGui.QValidator.Acceptable, s, pos)
            except:
                return (QtGui.QValidator.Intermediate, s, pos)

    def setText(self, t):
        super(_SliceLineEdit, self).setText(t)
        self._OnEditingFinished()

    def _OnEditingFinished(self):
        self.SliceChanged.emit(_GetSliceFromString(self.text()))
