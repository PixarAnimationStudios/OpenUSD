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
from PySide import QtGui, QtCore

class _ArrayAttributeModel(QtCore.QAbstractListModel):
    def __init__(self, attr, frame):
        super(_ArrayAttributeModel, self).__init__()

        self.val = attr.Get(frame)
        from scalarTypes import GetScalarTypeFromAttr
        self._scalarTypeName, _ = GetScalarTypeFromAttr(attr)

    def rowCount(self, index):
        return 0 if self.val is None else len(self.val)

    def data(self, index, role):
        dataVal = self.val[index.row()]

        if role == QtCore.Qt.DisplayRole:
            from scalarTypes import ToString
            return "%d: %s" % (
                    index.row(),
                    ToString(dataVal, self._scalarTypeName))

        elif role == QtCore.Qt.AccessibleTextRole:
            from scalarTypes import ToClipboard
            return ToClipboard(dataVal, self._scalarTypeName)

        return None

class ArrayAttributeView(QtGui.QListView):
    def __init__(self, parent):
        super(ArrayAttributeView, self).__init__(parent)

        # this line makes it so we don't have to query all of the data upfront.
        self.setUniformItemSizes(True)
        self.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)

        self._SetupContextMenu()

    def CanView(self, attr):
        return attr.GetTypeName().isArray

    def keyPressEvent(self, e):
        if e.matches(QtGui.QKeySequence.Copy):
            self.Copy()
        else:
            QtGui.QListView.keyPressEvent(self, e)
    
    def SetAttribute(self, attr, frame):
        # XXX: probably don't need to reconstruct especially if all
        # we're doing is changing the frame
        self.setModel(_ArrayAttributeModel(attr, frame))

    # context menu stuff
    def _SetupContextMenu(self):
        self.setContextMenuPolicy(QtCore.Qt.CustomContextMenu)
        QtCore.QObject.connect(self,
                QtCore.SIGNAL('customContextMenuRequested(QPoint)'),
                self._ShowContextMenu)

    def _ShowContextMenu(self, point):
        menu = QtGui.QMenu(self)
        menu.addAction("Copy", self.Copy)
        menu.addAction("Select All", self.SelectAll)
        menu.exec_(QtGui.QCursor.pos())

    def Copy(self):
        selectedIndexes = self.selectedIndexes()
        vals = ", ".join([self.model().data(idx, QtCore.Qt.AccessibleTextRole)
            for idx in selectedIndexes])

        if len(selectedIndexes) > 1:
            copyText = "[ %s ]" % vals
        else:
            copyText = vals

        QtGui.QApplication.clipboard().setText(copyText)

    def SelectAll(self):
        self.selectAll()

