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
import json

from pxr import UsdGeom

from maya import cmds
from maya.app.general import mayaMixin

from PySide import QtCore
from PySide import QtGui

EXPORTED_ATTRS_MAYA_ATTR_NAME = 'USD_UserExportedAttributesJson'

USD_ATTR_TYPE_USD = 'USD'
USD_ATTR_TYPE_PRIMVAR = 'primvar'
USD_ATTR_TYPE_USDRI = 'usdRi'

USD_ATTR_TYPE_OPTIONS = [
    USD_ATTR_TYPE_USD,
    USD_ATTR_TYPE_PRIMVAR,
    USD_ATTR_TYPE_USDRI
]

# The first empty string option here allows the interpolation to be un-specified.
PRIMVAR_INTERPOLATION_OPTIONS = [
    '',
    UsdGeom.Tokens.constant,
    UsdGeom.Tokens.uniform,
    UsdGeom.Tokens.vertex,
    UsdGeom.Tokens.faceVarying
]

RESERVED_ATTRIBUTES = set([EXPORTED_ATTRS_MAYA_ATTR_NAME])

ITEM_MIME_TYPE = 'application/x-maya-usdmaya-user-exported-attributes'


class ExportedAttribute(object):
    """
    This class represents a particular Maya attribute tagged by the user for
    export to USD. It may also contain additional metadata about how to export
    that attribute, such as an alternate attribute name to use in USD.
    """

    def __init__(self, mayaAttrName):
        self._mayaAttrName = mayaAttrName
        self._usdAttrType = None
        self._usdAttrName = None
        self._primvarInterpolation = None

    def __eq__(self, other):
        # Note that _primvarInterpolation does not factor in here.
        return (self._mayaAttrName == other._mayaAttrName and
                self._usdAttrType == other._usdAttrType and
                self._usdAttrName == other._usdAttrName)

    @property
    def mayaAttrName(self):
        return self._mayaAttrName

    @property
    def usdAttrType(self):
        if self._usdAttrType is None:
            return USD_ATTR_TYPE_USD

        return self._usdAttrType

    @usdAttrType.setter
    def usdAttrType(self, value):
        exportableAttrTypes = [
            USD_ATTR_TYPE_PRIMVAR,
            USD_ATTR_TYPE_USDRI
        ]
        if value not in exportableAttrTypes:
            self._usdAttrType = None
        else:
            self._usdAttrType = value

        # Clear out interpolation as well if this is not a primvar.
        if value != USD_ATTR_TYPE_PRIMVAR:
            self._primvarInterpolation = None

    @property
    def usdAttrName(self):
        return self._usdAttrName

    @usdAttrName.setter
    def usdAttrName(self, value):
        if not value or value == self._mayaAttrName:
            self._usdAttrName = None
        else:
            self._usdAttrName = value

    @property
    def primvarInterpolation(self):
        return self._primvarInterpolation

    @primvarInterpolation.setter
    def primvarInterpolation(self, value):
        exportableInterpolations = [
            UsdGeom.Tokens.constant,
            UsdGeom.Tokens.uniform,
            UsdGeom.Tokens.vertex,
            UsdGeom.Tokens.faceVarying
        ]
        if (self._usdAttrType != USD_ATTR_TYPE_PRIMVAR or
                value not in exportableInterpolations):
            self._primvarInterpolation = None
        else:
            self._primvarInterpolation = value

    def GetJsonDict(self):
        """
        This method returns a dictionary representation of this object
        that can be dumped to JSON.
        """
        result = { self._mayaAttrName : {} }
        if self._usdAttrType:
            result[self._mayaAttrName]['usdAttrType'] = self._usdAttrType
        if self._usdAttrName:
            result[self._mayaAttrName]['usdAttrName'] = self._usdAttrName
        if self._primvarInterpolation:
            result[self._mayaAttrName]['interpolation'] = self._primvarInterpolation
        return result

    @staticmethod
    def GetExportedAttributesFromNode(nodeName):
        """
        Given a Maya node name, this method returns a list of ExportedAttribute
        objects for all of the attributes tagged for export on the node.
        """
        result = []

        if not cmds.attributeQuery(EXPORTED_ATTRS_MAYA_ATTR_NAME,
                node=nodeName, exists=True):
            return result

        jsonString = cmds.getAttr('%s.%s' % (nodeName, EXPORTED_ATTRS_MAYA_ATTR_NAME))
        if not jsonString:
            return result

        jsonDict = json.loads(jsonString)
        for mayaAttrName in sorted(jsonDict.keys()):
            exportedAttr = ExportedAttribute(mayaAttrName)
            attrMetadata = jsonDict[mayaAttrName]
            exportedAttr.usdAttrType = attrMetadata.get('usdAttrType')
            exportedAttr.usdAttrName = attrMetadata.get('usdAttrName')
            exportedAttr.primvarInterpolation = attrMetadata.get('interpolation')
            result.append(exportedAttr)

        return result

    @staticmethod
    def RemoveExportedAttributesForNode(nodeName, mayaAttrNames):
        """
        Given a Maya node name, this method removes the attributes in
        mayaAttrNames as attributes that should be exported. The list of
        remaining exported attributes after the removal is returned.
        """
        exportedAttrs = ExportedAttribute.GetExportedAttributesFromNode(nodeName)

        # Filter out the attrs whose names are in mayaAttrNames.
        exportedAttrs = filter(
            lambda x: x.mayaAttrName not in mayaAttrNames, exportedAttrs)

        ExportedAttribute._WriteExportedAttributesToNode(nodeName, exportedAttrs)

        return exportedAttrs

    @staticmethod
    def UpdateExportedAttributesForNode(nodeName, exportedAttrs):
        """
        Given a Maya node name, this method adds or updates the attributes for
        export using the list of ExportedAttribute objects in exportedAttrs.
        Other existing attributes tagged for export are preserved.
        """
        # Exported attributes in exportedAttrs will replace any that already
        # exist with the same mayaAttrName, so remove those from the node first.
        updatedMayaAttrNames = {x.mayaAttrName for x in exportedAttrs}
        currExportedAttrs = ExportedAttribute.RemoveExportedAttributesForNode(
            nodeName, updatedMayaAttrNames)

        # Add the updated/added attributes. Order doesn't matter since we'll
        # be writing it out as a dictionary.
        currExportedAttrs.extend(exportedAttrs)
        ExportedAttribute._WriteExportedAttributesToNode(nodeName, currExportedAttrs)

    @staticmethod
    def _WriteExportedAttributesToNode(nodeName, exportedAttrs):
        """
        Given a Maya node name, this method records the given list of
        ExportedAttribute objects in exportedAttrs in the Maya attribute. the
        passed in list replaces any previous data.

        NOTE: This method will likely only ever be called by the Remove and
        Update methods above since multiple nodes may be selected and only the
        common attributes between them will be displayed.
        """
        jsonDict = {}
        for exportedAttr in exportedAttrs:
            jsonDict.update(exportedAttr.GetJsonDict())

        exportAttrExists = cmds.attributeQuery(EXPORTED_ATTRS_MAYA_ATTR_NAME,
            node=nodeName, exists=True)

        if not jsonDict:
            if exportAttrExists:
                # The export attribute exists but we'd be emptying it, so just
                # remove the attribute instead.
                cmds.deleteAttr('%s.%s' % (nodeName, EXPORTED_ATTRS_MAYA_ATTR_NAME))
            return

        if not exportAttrExists:
            cmds.addAttr(nodeName, ln=EXPORTED_ATTRS_MAYA_ATTR_NAME, dt='string')

        jsonString = json.dumps(jsonDict)
        cmds.setAttr('%s.%s' % (nodeName, EXPORTED_ATTRS_MAYA_ATTR_NAME),
            jsonString, type='string')


class ExportedAttributesModel(QtCore.QAbstractTableModel):
    """
    This class provides a model suitable for displaying ExportedAttribute objects
    in a table view. It updates the selected Maya nodes as the data in the
    model changes.
    """
    MAYA_ATTR_NAME_COLUMN = 0
    USD_ATTR_TYPE_COLUMN = 1
    USD_ATTR_NAME_COLUMN = 2
    PRIMVAR_INTERPOLATION_COLUMN = 3
    NUM_COLUMNS = 4

    def __init__(self, exportedAttrs=None, parent=None):
        super(ExportedAttributesModel, self).__init__(parent=parent)
        if exportedAttrs is None:
            exportedAttrs = []
        self._exportedAttrs = exportedAttrs

    @property
    def exportedAttributes(self):
        return self._exportedAttrs

    @exportedAttributes.setter
    def exportedAttributes(self, exportedAttrs):
        self._exportedAttrs = exportedAttrs
        self.reset()

    def rowCount(self, parent=QtCore.QModelIndex()):
        if not self._exportedAttrs:
            return 0
        return len(self._exportedAttrs)

    def columnCount(self, parent=QtCore.QModelIndex()):
        if not self.rowCount(parent):
            return 0
        return ExportedAttributesModel.NUM_COLUMNS

    def headerData(self, section, orientation, role):
        COLUMN_HEADERS = [
            'Maya Attribute Name',
            'USD Attribute Type',
            'USD Attribute Name',
            'Interpolation']

        COLUMN_TOOLTIPS = [
            'The name of the Maya node attribute to be exported to USD',
            'The type of attribute to create in USD',
            'Which name to use for the attribute in USD\n' +
                '(If empty, the Maya Attribute Name will be used, and for USD-type\n' +
                'attributes, they will be exported into the "userProperties" namespace)',
            'Which interpolation to use for primvar-type attributes'
        ]

        if role == QtCore.Qt.DisplayRole:
            return COLUMN_HEADERS[section]
        elif role == QtCore.Qt.ToolTipRole:
            return COLUMN_TOOLTIPS[section]

        return None

    def data(self, index, role=QtCore.Qt.DisplayRole):
        value = None

        if not self._exportedAttrs or not index.isValid() or role != QtCore.Qt.DisplayRole:
            return value

        row = index.row()
        column = index.column()

        exportedAttr = self._exportedAttrs[row]
        if column == ExportedAttributesModel.MAYA_ATTR_NAME_COLUMN:
            value = exportedAttr.mayaAttrName
        elif column == ExportedAttributesModel.USD_ATTR_TYPE_COLUMN:
            value = exportedAttr.usdAttrType
        elif column == ExportedAttributesModel.USD_ATTR_NAME_COLUMN:
            value = exportedAttr.usdAttrName
        elif column == ExportedAttributesModel.PRIMVAR_INTERPOLATION_COLUMN:
            value = exportedAttr.primvarInterpolation

        return value

    def setData(self, index, value, role=QtCore.Qt.EditRole):
        if not self._exportedAttrs:
            return False

        row = index.row()
        column = index.column()

        exportedAttr = self._exportedAttrs[row]

        if column == ExportedAttributesModel.USD_ATTR_TYPE_COLUMN:
            exportedAttr.usdAttrType = value
        elif column == ExportedAttributesModel.USD_ATTR_NAME_COLUMN:
            exportedAttr.usdAttrName = value
        elif column == ExportedAttributesModel.PRIMVAR_INTERPOLATION_COLUMN:
            exportedAttr.primvarInterpolation = value
        else:
            return False

        # Update the selected nodes with the new data.
        selectedNodeNames = cmds.ls(selection=True, long=True)
        if not selectedNodeNames:
            return False

        for selectedNodeName in selectedNodeNames:
            ExportedAttribute.UpdateExportedAttributesForNode(selectedNodeName,
                self._exportedAttrs)

        self.dataChanged.emit(index, index)
        return True

    def flags(self, index):
        if not index.isValid():
            return QtCore.Qt.ItemIsDropEnabled

        itemFlags = (QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable)

        column = index.column()
        if column == ExportedAttributesModel.MAYA_ATTR_NAME_COLUMN:
            itemFlags |= QtCore.Qt.ItemIsDragEnabled
        elif (column == ExportedAttributesModel.USD_ATTR_TYPE_COLUMN or
                column == ExportedAttributesModel.USD_ATTR_NAME_COLUMN):
            itemFlags |= QtCore.Qt.ItemIsEditable
        elif column == ExportedAttributesModel.PRIMVAR_INTERPOLATION_COLUMN:
            # The primvar column is only editable if this is a primvar.
            exportedAttr = self._exportedAttrs[index.row()]
            if exportedAttr.usdAttrType == USD_ATTR_TYPE_PRIMVAR:
                itemFlags |= QtCore.Qt.ItemIsEditable

        return itemFlags

    def mimeTypes(self):
        return [ITEM_MIME_TYPE]

    def mimeData(self, indexList):
        attrsString = ','.join([x.data() for x in indexList if x.isValid()])

        mimeData = QtCore.QMimeData()
        mimeData.setData(ITEM_MIME_TYPE, QtCore.QByteArray(attrsString))

        return mimeData

    def dropMimeData(self, mimeData, action, row, column, parent):
        # When attributes are dropped here, it means they are being added
        # as user exported attributes.
        selectedNodeNames = cmds.ls(selection=True, long=True)
        if not selectedNodeNames:
            return True

        mayaAttrNamesStr = str(mimeData.data(ITEM_MIME_TYPE))
        if not mayaAttrNamesStr:
            return True

        mayaAttrNames = mayaAttrNamesStr.split(',')
        exportedAttrs = [ExportedAttribute(x) for x in mayaAttrNames]
        for selectedNodeName in selectedNodeNames:
            ExportedAttribute.UpdateExportedAttributesForNode(selectedNodeName,
                exportedAttrs)

        index = self.createIndex(row, column)
        self.dataChanged.emit(index, index)
        return True


class ExportedAttributesViewItemDelegate(QtGui.QStyledItemDelegate):

    def __init__(self, choices, parent=None):
        super(ExportedAttributesViewItemDelegate, self).__init__(parent=parent)
        self._choices = choices

    def createEditor(self, parent, option, index):
        editor = QtGui.QComboBox(parent)
        editor.addItems(self._choices)

        # Use the model data to pre-select a choice in the combo box.
        currentValue = index.model().data(index)
        if currentValue in self._choices:
            currentIndex = self._choices.index(currentValue)
            editor.setCurrentIndex(currentIndex)

        editor.currentIndexChanged.connect(self.currentIndexChanged)
        return editor

    def setEditorData(self, editor, index):
        editor.blockSignals(True)
        editor.setCurrentIndex(editor.currentIndex())
        editor.blockSignals(False)

    def setModelData(self, editor, model, index):
        model.setData(index, editor.currentText())

    def currentIndexChanged(self):
        self.commitData.emit(self.sender())


class ExportedAttributesView(QtGui.QTableView):

    def __init__(self, parent=None):
        super(ExportedAttributesView, self).__init__(parent=parent)
        self.setDragEnabled(True)
        self.setAcceptDrops(True)

        self.setItemDelegateForColumn(ExportedAttributesModel.USD_ATTR_TYPE_COLUMN,
            ExportedAttributesViewItemDelegate(USD_ATTR_TYPE_OPTIONS, self))

        self.setItemDelegateForColumn(ExportedAttributesModel.PRIMVAR_INTERPOLATION_COLUMN,
            ExportedAttributesViewItemDelegate(PRIMVAR_INTERPOLATION_OPTIONS, self))

    def dragEnterEvent(self, event):
        if event.source() == self:
            event.ignore()
            return

        return super(ExportedAttributesView, self).dragEnterEvent(event)


class AddAttributesModel(QtGui.QStringListModel):

    def flags(self, index):
        if not index.isValid():
            return QtCore.Qt.ItemIsDropEnabled

        return (QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsDragEnabled)

    def mimeTypes(self):
        return [ITEM_MIME_TYPE]

    def mimeData(self, indexList):
        attrsString = ','.join([x.data() for x in indexList if x.isValid()])

        mimeData = QtCore.QMimeData()
        mimeData.setData(ITEM_MIME_TYPE, QtCore.QByteArray(attrsString))

        return mimeData

    def dropMimeData(self, mimeData, action, row, column, parent):
        # When attributes are dropped here, it means they are being removed
        # as user exported attributes.
        selectedNodeNames = cmds.ls(selection=True, long=True)
        if not selectedNodeNames:
            return True

        mayaAttrNamesStr = str(mimeData.data(ITEM_MIME_TYPE))
        if not mayaAttrNamesStr:
            return True

        mayaAttrNames = mayaAttrNamesStr.split(',')
        for selectedNodeName in selectedNodeNames:
            ExportedAttribute.RemoveExportedAttributesForNode(selectedNodeName,
                mayaAttrNames)

        index = self.createIndex(row, column)
        self.dataChanged.emit(index, index)
        return True


class AddAttributesView(QtGui.QListView):

    def __init__(self, parent=None):
        super(AddAttributesView, self).__init__(parent=parent)
        self.setDragEnabled(True)
        self.setAcceptDrops(True)
        self.setSelectionMode(QtGui.QAbstractItemView.ExtendedSelection)

    def dragEnterEvent(self, event):
        if event.source() == self:
            event.ignore()
            return

        return super(AddAttributesView, self).dragEnterEvent(event)


class UserExportedAttributeWidget(mayaMixin.MayaQWidgetDockableMixin, QtGui.QWidget):

    _currentInstance = None

    @classmethod
    def closeCurrentInstance(cls):
        if cls._currentInstance is not None:
            if cls._currentInstance._mayaSelectionChangedJob is not None:
                cmds.scriptJob(kill=cls._currentInstance._mayaSelectionChangedJob)
                cls._currentInstance._mayaSelectionChangedJob = None

            if cls._currentInstance._mayaUndoJob is not None:
                cmds.scriptJob(kill=cls._currentInstance._mayaUndoJob)
                cls._currentInstance._mayaUndoJob = None

            if cls._currentInstance._mayaRedoJob is not None:
                cmds.scriptJob(kill=cls._currentInstance._mayaRedoJob)
                cls._currentInstance._mayaRedoJob = None

            cls._currentInstance.close()
            cls._currentInstance = None

    def __init__(self, parent=None):
        UserExportedAttributeWidget.closeCurrentInstance()

        super(UserExportedAttributeWidget, self).__init__(parent=parent)
        self._setupUI()

        self._mayaSelectionChangedJob = cmds.scriptJob(event=["SelectionChanged", self._syncUI])
        self._mayaUndoJob = cmds.scriptJob(event=["Undo", self._syncUI])
        self._mayaRedoJob = cmds.scriptJob(event=["Redo", self._syncUI])

        UserExportedAttributeWidget._currentInstance = self

        # Force a sync on the first load.
        self._syncUI()

    def _setupUI(self):
        self.setWindowTitle("Export Attributes to USD")
        layout = QtGui.QVBoxLayout()

        # This section contains the attributes tagged for export.
        label = QtGui.QLabel()
        label.setText('Exported Attributes:')
        layout.addWidget(label)

        self.exportedAttrsModel = ExportedAttributesModel()
        self.exportedAttrsView = ExportedAttributesView()
        self.exportedAttrsView.verticalHeader().hide()
        self.exportedAttrsView.setModel(self.exportedAttrsModel)
        selectionModel = self.exportedAttrsView.selectionModel()
        selectionModel.selectionChanged.connect(self._onExportedAttrsSelectionChanged)
        self.exportedAttrsModel.dataChanged.connect(self._onModelDataChanged)
        layout.addWidget(self.exportedAttrsView)

        self.removeExportedAttrButton = QtGui.QPushButton("Remove Exported Attribute")
        self.removeExportedAttrButton.clicked.connect(self._onRemoveExportedAttrPressed)
        self.removeExportedAttrButton.setEnabled(False)
        layout.addWidget(self.removeExportedAttrButton)

        # This section contains the attributes available for export.
        label = QtGui.QLabel()
        label.setText('Available Attributes:')
        layout.addWidget(label)

        self.userDefinedCheckBox = QtGui.QCheckBox('User Defined')
        self.userDefinedCheckBox.setToolTip('Show only user-defined (dynamic) attributes')
        self.userDefinedCheckBox.setChecked(True)
        self.userDefinedCheckBox.stateChanged.connect(self._syncUI)
        layout.addWidget(self.userDefinedCheckBox)

        self.addAttrsModel = AddAttributesModel()
        self.addAttrsView = AddAttributesView()
        self.addAttrsView.setModel(self.addAttrsModel)
        selectionModel = self.addAttrsView.selectionModel()
        selectionModel.selectionChanged.connect(self._onAddAttrsSelectionChanged)
        self.addAttrsModel.dataChanged.connect(self._onModelDataChanged)
        layout.addWidget(self.addAttrsView)

        self.addExportedAttrButton = QtGui.QPushButton("Add Exported Attribute")
        self.addExportedAttrButton.clicked.connect(self._onAddExportedAttrPressed)
        self.addExportedAttrButton.setEnabled(False)
        layout.addWidget(self.addExportedAttrButton)

        self.setLayout(layout)

    def _onExportedAttrsSelectionChanged(self, selected, deselected):
        if selected.isEmpty():
            self.removeExportedAttrButton.setEnabled(False)
        else:
            self.removeExportedAttrButton.setEnabled(True)

    def _onAddAttrsSelectionChanged(self, selected, deselected):
        if selected.isEmpty():
            self.addExportedAttrButton.setEnabled(False)
        else:
            self.addExportedAttrButton.setEnabled(True)

    def _onRemoveExportedAttrPressed(self):
        selectedNodeNames = cmds.ls(selection=True, long=True)
        if not selectedNodeNames:
            return

        mayaAttrNames = [x.data() for x in self.exportedAttrsView.selectedIndexes()]
        if not mayaAttrNames:
            return

        for selectedNodeName in selectedNodeNames:
            ExportedAttribute.RemoveExportedAttributesForNode(selectedNodeName,
                mayaAttrNames)

        self._syncUI()

    def _onAddExportedAttrPressed(self):
        selectedNodeNames = cmds.ls(selection=True, long=True)
        if not selectedNodeNames:
            return

        exportedAttrs = [ExportedAttribute(x.data()) for x in self.addAttrsView.selectedIndexes()]
        if not exportedAttrs:
            return

        for selectedNodeName in selectedNodeNames:
            ExportedAttribute.UpdateExportedAttributesForNode(selectedNodeName,
                exportedAttrs)

        self._syncUI()

    def _onModelDataChanged(self, topLeft, bottomRight):
        self._syncUI()

    def _syncUI(self):
        # Since _syncUI is called in response to events that invalidate/clear
        # the selections in the views, disable the buttons until something is
        # selected again.
        self.removeExportedAttrButton.setEnabled(False)
        self.addExportedAttrButton.setEnabled(False)

        selectedNodeNames = cmds.ls(selection=True, long=True)
        if not selectedNodeNames:
            self.addAttrsModel.setStringList([])
            self.exportedAttrsModel.exportedAttributes = []
            self.exportedAttrsView.resizeColumnsToContents()
            return

        # Collect the export attributes common to all selected nodes. If the
        # same attribute is configured differently on multiple objects (e.g.
        # different usdAttrName), then do not include that attribute.
        allExportedAttributeNames = set()
        commonExportedAttributeNames = set()
        commonExportedAttrs = {}
        for exportedAttr in ExportedAttribute.GetExportedAttributesFromNode(selectedNodeNames[0]):
            mayaAttrName = exportedAttr.mayaAttrName
            allExportedAttributeNames.add(mayaAttrName)
            commonExportedAttributeNames.add(mayaAttrName)
            commonExportedAttrs[mayaAttrName] = exportedAttr

        for selectedNodeName in selectedNodeNames[1:]:
            exportedAttrNames = set()
            for exportedAttr in ExportedAttribute.GetExportedAttributesFromNode(selectedNodeName):
                mayaAttrName = exportedAttr.mayaAttrName
                allExportedAttributeNames.add(mayaAttrName)
                if (mayaAttrName in commonExportedAttrs and
                        commonExportedAttrs[mayaAttrName] == exportedAttr):
                    exportedAttrNames.add(mayaAttrName)
            commonExportedAttributeNames.intersection_update(exportedAttrNames)

        commonExportedAttrs = [commonExportedAttrs[x] for x in commonExportedAttributeNames]
        commonExportedAttrs.sort(key=lambda x: x.mayaAttrName)
        self.exportedAttrsModel.exportedAttributes = commonExportedAttrs

        # Normally, the combo boxes for selecting usdAttrType and
        # primvarInterpolation would only appear when the table cell is put into
        # edit mode. Instead, we want the combo boxes to always be visible, so
        # we tell the view to open them as persistent editors.
        for row in xrange(self.exportedAttrsModel.rowCount()):
            usdAttrTypeIndex = self.exportedAttrsModel.index(row,
                ExportedAttributesModel.USD_ATTR_TYPE_COLUMN)
            self.exportedAttrsView.openPersistentEditor(usdAttrTypeIndex)

            # Only open the interpolation editor if this is a primvar.
            if self.exportedAttrsModel.data(usdAttrTypeIndex) == USD_ATTR_TYPE_PRIMVAR:
                primvarInterpolationIndex = self.exportedAttrsModel.index(row,
                    ExportedAttributesModel.PRIMVAR_INTERPOLATION_COLUMN)
                self.exportedAttrsView.openPersistentEditor(primvarInterpolationIndex)

        self.exportedAttrsView.resizeColumnsToContents()

        # Collect the attributes common to all selected nodes.
        cmdOptions = {'read': True}
        if self.userDefinedCheckBox.isChecked():
            cmdOptions['userDefined'] = True

        commonAttrNames = set(cmds.listAttr(selectedNodeNames[0], **cmdOptions) or [])
        for selectedNodeName in selectedNodeNames[1:]:
            attrNames = set(cmds.listAttr(selectedNodeName, **cmdOptions) or [])
            commonAttrNames.intersection_update(attrNames)

        # Subtract out reserved attribute names and attributes already being
        # exported by ANY node.
        commonAttrNames -= RESERVED_ATTRIBUTES
        commonAttrNames -= allExportedAttributeNames

        self.addAttrsModel.setStringList(sorted(list(commonAttrNames)))
