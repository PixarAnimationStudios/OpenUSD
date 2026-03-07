#
# Copyright 2025 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from .qt import QtCore, QtGui, QtWidgets

class SearchBarWidget(QtWidgets.QWidget):
    textChanged = QtCore.Signal(str)

    def __init__(self, placeholder="Search...", parent=None):
        super().__init__(parent)

        layout = QtWidgets.QHBoxLayout(self)
        layout.setContentsMargins(0, 0, 0, 0)

        self.edit = QtWidgets.QLineEdit()
        self.edit.setPlaceholderText(placeholder)
        self.edit.setClearButtonEnabled(True)

        layout.addWidget(self.edit)
        self.edit.textChanged.connect(self.textChanged.emit)

    def text(self):
        return self.edit.text()

    def setText(self, text):
        self.edit.setText(text)

# Populates all validator info: name, isSuite, doc from all validator metadata
def _PopulateValidatorAndSuitesByPlugin():
    from pxr import UsdValidation
    validationRegistry = UsdValidation.ValidationRegistry()
    validatorsByPlugin = {}
    for metadata in validationRegistry.GetAllValidatorMetadata():
        plugName = metadata.plugin.name
        validatorsByPlugin.setdefault(plugName, [])
        # Validator name implicitly has plugin name as prefix, for ui purposes
        # we will strip that out
        validatorsByPlugin[plugName].append(
            (metadata.name.split(":")[1],
             metadata.isSuite,
             metadata.doc))
    return validatorsByPlugin

class WordWrapDelegate(QtWidgets.QStyledItemDelegate):
    def initStyleOption(self, option, index):
        super().initStyleOption(option, index)
        option.wordWrap = True

class ValidatorPluginsWidget(QtWidgets.QWidget):
    # pluginName, validatorName, description, enabled
    validatorToggled = QtCore.Signal(str, str, str, bool) 
    def __init__(self, parent=None):
        super().__init__(parent)

        validatorsByPlugin = _PopulateValidatorAndSuitesByPlugin()

        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(
            QtWidgets.QLabel(
                "Available validators/suites (grouped by plugins):"))

        self.search = SearchBarWidget("Search by plugins, validators/suites names or docs...")
        self.search.textChanged.connect(self._filterPluginsAndValidators)
        layout.addWidget(self.search)

        self.tree = QtWidgets.QTreeWidget()
        self.tree.setHeaderLabels(["Validator/Suite", "Description"])
        self.tree.setColumnCount(2)
        self.tree.setHeaderHidden(False)
        self.tree.setRootIsDecorated(True)
        self.tree.setAlternatingRowColors(True)
        self.tree.setItemDelegate(WordWrapDelegate(self.tree))
        self.tree.header().setSectionResizeMode(
            QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self.tree.header().setStretchLastSection(False)
        self.tree.itemChanged.connect(self._onItemChanged)

        allSelectorItem = self.makeCheckableItem(
            self.tree, "Select/Deselect All", role="all")
        allSelectorItem.setText(1, "Select or deselect all validators/suites.")
        allSelectorItem.setExpanded(True)
        for pluginName in validatorsByPlugin:
            pluginItem = self.makeCheckableItem(
                allSelectorItem, pluginName, role="plugin")
            pluginItem.setText(
                1, f"Validators registered by {pluginName} plugin.")
            for validatorName, isSuite, doc in validatorsByPlugin[pluginName]:
                childItem = self.makeCheckableItem(pluginItem, validatorName)
                if isSuite:
                    childItem.setData(0, QtCore.Qt.ItemDataRole.UserRole, "suite")
                    doc = "(Suite) " + (doc if doc else "")
                childItem.setText(1, doc if doc else "")
                childItem.setToolTip(1, doc if doc else "")

        self.tree.sortItems(0, QtCore.Qt.SortOrder.AscendingOrder)

        layout.addWidget(self.tree)

    def makeCheckableItem(self, parent, text, role=None):
        item = QtWidgets.QTreeWidgetItem(parent, [text])
        item.setFlags(item.flags() | QtCore.Qt.ItemFlag.ItemIsUserCheckable)
        item.setCheckState(0, QtCore.Qt.CheckState.Unchecked)
        if role == "plugin":
            item.setData(0, QtCore.Qt.ItemDataRole.UserRole, "plugin")
        elif role == "all":
            item.setData(0, QtCore.Qt.ItemDataRole.UserRole, "all")
        return item

    def _filterPluginsAndValidators(self, text):
        root = self.tree.invisibleRootItem()
        allSelectorItem = root.child(0)
        for i in range(allSelectorItem.childCount()):
            pluginItem = allSelectorItem.child(i)
            anyChildVisible = False
            if not text:
                pluginItem.setHidden(False)
                pluginItem.setExpanded(False)
                continue
            for j in range(pluginItem.childCount()):
                validatorItem = pluginItem.child(j)
                # match both name and docs
                validatorMatches = text.lower() in validatorItem.text(0).lower() or \
                                   text.lower() in validatorItem.text(1).lower()
                validatorItem.setHidden(not validatorMatches)
                validatorItem.setExpanded(validatorMatches)
                anyChildVisible = anyChildVisible or validatorMatches
            pluginItemMatches = anyChildVisible or \
                text.lower() in pluginItem.text(0).lower()
            pluginItem.setHidden(not pluginItemMatches)
            pluginItem.setExpanded(pluginItemMatches)

    def _onItemChanged(self, item, column):
        # Special toggling for everything
        if item.data(0, QtCore.Qt.ItemDataRole.UserRole) == "all":
            # Everything was changed
            state = item.checkState(0)
            for i in range(item.childCount()):
                pluginItem = item.child(i)
                pluginItem.setCheckState(0, state)
                for j in range(pluginItem.childCount()):
                    pluginItem.child(j).setCheckState(0, state)
            return
        
        # Special toggling for a specific plugin
        if item.data(0, QtCore.Qt.ItemDataRole.UserRole) == "plugin":
            state = item.checkState(0)
            for i in range(item.childCount()):
                item.child(i).setCheckState(0, state)
            return

        # toggling for a specific validator/suite
        parent = item.parent()
        if not parent:
            return
        pluginName = parent.text(0)
        validatorName = item.text(0)
        enabled = item.checkState(0) == QtCore.Qt.CheckState.Checked
        self.validatorToggled.emit(
            pluginName, validatorName, item.text(1), enabled)

class SelectedValidatorsWidget(QtWidgets.QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(QtWidgets.QLabel("Selected validators/suites:"))

        self.search = SearchBarWidget("Search by validators/suites names or docs...")
        self.search.textChanged.connect(self._filterSelectedValidators)
        layout.addWidget(self.search)

        self.tree = QtWidgets.QTreeWidget()
        self.tree.setHeaderHidden(True)
        self.tree.setRootIsDecorated(True)
        self.tree.setAlternatingRowColors(True)
        self.tree.setItemDelegate(WordWrapDelegate(self.tree))
        layout.addWidget(self.tree)

    def addValidator(self, pluginName, validatorName, description):
        pluginItem = self._findOrCreatePluginItem(pluginName)
        if not self._findChild(pluginItem, validatorName):
            item = QtWidgets.QTreeWidgetItem(pluginItem, [validatorName])
            item.setToolTip(0, description if description else "")
        self.tree.expandAll()
        self.tree.sortItems(0, QtCore.Qt.SortOrder.AscendingOrder)
        self._filterSelectedValidators(self.search.text())

    def removeValidator(self, pluginName, validatorName):
        pluginItem = self._findPluginItem(pluginName)
        if not pluginItem:
            return
        childItem = self._findChild(pluginItem, validatorName)
        if childItem:
            pluginItem.removeChild(childItem)
        if pluginItem.childCount() == 0:
            index = self.tree.indexOfTopLevelItem(pluginItem)
            self.tree.takeTopLevelItem(index)
        self.tree.sortItems(0, QtCore.Qt.SortOrder.AscendingOrder)
        self._filterSelectedValidators(self.search.text())

    def getSelectedValidators(self):
        # Returns a list of validator names in the form "PluginName:ValidatorName"
        selectedValidators = []
        for i in range(self.tree.topLevelItemCount()):
            pluginItem = self.tree.topLevelItem(i)
            pluginName = pluginItem.text(0)
            for j in range(pluginItem.childCount()):
                validatorName = pluginItem.child(j).text(0)
                selectedValidators.append(f"{pluginName}:{validatorName}")
        return selectedValidators

    def _filterSelectedValidators(self, text):
        root = self.tree.invisibleRootItem()
        for i in range(root.childCount()):
            pluginItem = root.child(i)
            anyChildVisible = False
            if not text:
                pluginItem.setHidden(False)
                continue
            for j in range(pluginItem.childCount()):
                validatorItem = pluginItem.child(j)
                # match both name and docs
                validatorMatches = text.lower() in validatorItem.text(0).lower() or \
                                   text.lower() in validatorItem.toolTip(0).lower()
                validatorItem.setHidden(not validatorMatches)
                # expand the item if it matches
                validatorItem.setExpanded(validatorMatches)
                anyChildVisible = anyChildVisible or validatorMatches
            pluginItemMatches = anyChildVisible or \
                text.lower() in pluginItem.text(0).lower()
            pluginItem.setHidden(not pluginItemMatches)

    def _findPluginItem(self, pluginName):
        items = self.tree.findItems(pluginName, QtCore.Qt.MatchFlag.MatchExactly, 0)
        return items[0] if items else None

    def _findOrCreatePluginItem(self, pluginName):
        return self._findPluginItem(pluginName) or \
            QtWidgets.QTreeWidgetItem(self.tree, [pluginName])

    def _findChild(self, parentItem, childName):
        for i in range(parentItem.childCount()):
            if parentItem.child(i).text(0) == childName:
                return parentItem.child(i)
        return None

class TimeRangeOptions:
    def __init__(self):
        self.mode = 0
        self.startTime = 0.0
        self.endTime = 1.0
        self.includeDefaultTime = False

class OptionsWidget(QtWidgets.QWidget):
    # primsSetFromSelection, includeDescendantPrims, timeRangeOptions
    runValidation = QtCore.Signal(bool, bool, TimeRangeOptions)
    def __init__(self, parent=None):
        super().__init__(parent)

        layout = QtWidgets.QVBoxLayout(self)
        layout.addWidget(QtWidgets.QLabel("Validation Context Options:"))

        self.primsSetFromSelection = QtWidgets.QCheckBox(
            "Validate selected prim(s)")
        self.primsSetFromSelection.setToolTip(
            "If checked, validates based on currently selected prim(s).\n"
            "If unchecked, validates the entire stage.")
        layout.addWidget(self.primsSetFromSelection)
        self.primsSetFromSelection.stateChanged.connect(
            lambda state: self.includeDescendantPrims.setEnabled(
                state == QtCore.Qt.CheckState.Checked))

        self.includeDescendantPrims = QtWidgets.QCheckBox(
            "Include descendant prims")
        self.includeDescendantPrims.setToolTip(
            "If checked, includes all descendant prims of the selected prim(s).\n"
            "If unchecked, only validates the explicitly selected prim(s).")
        self.includeDescendantPrims.setEnabled(False)
        layout.addWidget(self.includeDescendantPrims)

        timeRangeGroup = QtWidgets.QGroupBox("Time Range Options")
        timeRangeLayout = QtWidgets.QVBoxLayout(timeRangeGroup)
        self.timeRangeCombo = QtWidgets.QComboBox()
        self.timeRangeCombo.addItems([
            "Current Frame",
            "Playback Range",
            "Specific Time Range"])
        timeRangeLayout.addWidget(self.timeRangeCombo)
        vLayout = QtWidgets.QVBoxLayout()
        vLayout.addWidget(QtWidgets.QLabel("Start Time:"))
        self.startTimeEdit = QtWidgets.QDoubleSpinBox()
        self.startTimeEdit.setRange(-1e6, 1e6)
        self.startTimeEdit.setValue(0.0)
        self.startTimeEdit.setEnabled(False)
        vLayout.addWidget(self.startTimeEdit)
        vLayout.addWidget(QtWidgets.QLabel("End Time:"))
        self.endTimeEdit = QtWidgets.QDoubleSpinBox()
        self.endTimeEdit.setRange(-1e6, 1e6)
        self.endTimeEdit.setValue(1.0)
        self.endTimeEdit.setEnabled(False)
        vLayout.addWidget(self.endTimeEdit)
        timeRangeLayout.addLayout(vLayout)
        self.includeDefaultTime = QtWidgets.QCheckBox("Include Default Time")
        timeRangeLayout.addWidget(self.includeDefaultTime)
        layout.addWidget(timeRangeGroup)

        def _onTimeRangeChanged(index):
            isSpecific = \
                self.timeRangeCombo.currentText() == "Specific Time Range"
            self.startTimeEdit.setEnabled(isSpecific)
            self.endTimeEdit.setEnabled(isSpecific)

        self.timeRangeCombo.currentIndexChanged.connect(_onTimeRangeChanged)

         # Add stretch to push the button to the bottom
        layout.addStretch()

        self.runButton = QtWidgets.QPushButton("Run Validation")
        self.runButton.setEnabled(False)
        layout.addWidget(self.runButton)

        self.runButton.clicked.connect(self._onRunClicked)

    def _getTimeRangeOptions(self):
        timeRangeOptions = TimeRangeOptions()
        if self.timeRangeCombo.currentText() == "Current Frame":
            timeRangeOptions.mode = 0
        elif self.timeRangeCombo.currentText() == "Playback Range":
            timeRangeOptions.mode = 1
        else:
            timeRangeOptions.mode = 2
        if timeRangeOptions.mode == 2:
            timeRangeOptions.startTime = self.startTimeEdit.value()
            timeRangeOptions.endTime = self.endTimeEdit.value()
        timeRangeOptions.includeDefaultTime = self.includeDefaultTime.isChecked()
        return timeRangeOptions


    def _onRunClicked(self):
        self.runValidation.emit(
            self.primsSetFromSelection.isChecked(), 
            self.includeDescendantPrims.isChecked(),
            self._getTimeRangeOptions())

class ErrorTypeDelegate(QtWidgets.QStyledItemDelegate):
    def paint(self, painter, option, index):
        if index.column() == 4:
            error_type = index.data()
            if error_type == "Error":
                painter.fillRect(option.rect, QtGui.QColor(220, 53, 69))
            elif error_type == "Warn":
                painter.fillRect(option.rect, QtGui.QColor(218, 165, 32))
            elif error_type == "Info":
                painter.fillRect(option.rect, QtGui.QColor(23, 162, 184))
            else:
                painter.fillRect(option.rect, QtGui.QColor(108, 117, 125))
            optionsFont = option.font
            optionsFont.setBold(True)
            option.font = optionsFont
        # Call base paint to draw text/etc.
        super().paint(painter, option, index)

class ResultWidget(QtWidgets.QWidget):
    def __init__(self, appController, parent=None):
        self._appController = appController
        super().__init__(parent)

        layout = QtWidgets.QVBoxLayout(self)       
        resultsGroupBox = QtWidgets.QGroupBox("Validation Results")
        groupBoxLayout = QtWidgets.QVBoxLayout(resultsGroupBox)
        self.search = SearchBarWidget("Search validation results by any column...")
        self.search.textChanged.connect(self._filterResultRows)
        self.search.setVisible(False)
        groupBoxLayout.addWidget(self.search)

        self.noResultLabel = QtWidgets.QLabel("No validation results yet.")
        groupBoxLayout.addWidget(self.noResultLabel)

        self.resultTable = QtWidgets.QTableWidget()
        self.resultTable.setSortingEnabled(True)
        self.resultTable.setColumnCount(5)
        self.resultTable.setHorizontalHeaderLabels(
            ["Validator Name", "Error Name", "Error Sites", "Message", "Type"])
        self.resultTable.horizontalHeader().setStretchLastSection(False)
        self.resultTable.horizontalHeader().setSectionResizeMode(
            QtWidgets.QHeaderView.ResizeMode.Stretch)
        self.resultTable.setVisible(False)
        self.resultTable.selectionModel().selectionChanged.connect(
            self._onRowsSelected)
        self.resultTable.cellDoubleClicked.connect(self._onCellDoubleClicked)
        groupBoxLayout.addWidget(self.resultTable)
        layout.addWidget(resultsGroupBox)
        self.results = []
        self.currentSelectedErrors = []

    def populateResultsAndUpdateWidget(self, validationErrors):
        self.results.clear()
        self.resultTable.clearContents()
        if not validationErrors:
            self.search.setVisible(False)
            self.noResultLabel.setText("No validation errors found.")
            self.noResultLabel.setVisible(True)
            self.resultTable.setVisible(False)
            return

        self.search.setVisible(True)
        self.noResultLabel.setVisible(False)
        self.resultTable.setRowCount(len(validationErrors))
        self.resultTable.setVisible(True)

        for row, error in enumerate(validationErrors):
            self.results.append(error)

            nameItem = QtWidgets.QTableWidgetItem(
                error.GetValidator().GetMetadata().name)
            nameItem.setToolTip(error.GetValidator().GetMetadata().name)
            nameItem.setFlags(nameItem.flags() & ~QtCore.Qt.ItemFlag.ItemIsEditable)

            errorNameItem = QtWidgets.QTableWidgetItem(error.GetName())
            errorNameItem.setToolTip(error.GetName())
            errorNameItem.setFlags(errorNameItem.flags() & ~QtCore.Qt.ItemFlag.ItemIsEditable)

            errorSites = error.GetSites()
            errorSitePaths = []
            for site in errorSites:
                if site.IsPrim():
                    errorSitePaths.append(
                        site.GetPrim().GetPath().pathString)
                elif site.IsProperty():
                    errorSitePaths.append(
                        site.GetProperty().GetPath().pathString)
                else:
                    layer = site.GetLayer()
                    if layer:
                        errorSitePaths.append(layer.identifier)
                    else:
                        errorSitePaths.append(
                            error.GetStage().GetRootLayer().identifier)
            errorSiteItem = QtWidgets.QTableWidgetItem("\n".join(errorSitePaths))
            errorSiteItem.setFlags(errorSiteItem.flags() & ~QtCore.Qt.ItemFlag.ItemIsEditable)

            errorMsgItem = QtWidgets.QTableWidgetItem(error.GetMessage())
            errorMsgItem.setToolTip(error.GetMessage())
            errorMsgItem.setFlags(errorMsgItem.flags() & ~QtCore.Qt.ItemFlag.ItemIsEditable)

            typeItem = QtWidgets.QTableWidgetItem(error.GetType().name)
            typeItem.setFlags(typeItem.flags() & ~QtCore.Qt.ItemFlag.ItemIsEditable)

            self.resultTable.setItem(row, 0, nameItem)
            self.resultTable.setItem(row, 1, errorNameItem)
            self.resultTable.setItem(row, 2, errorSiteItem)
            if len(errorSitePaths) > 1:
                self.resultTable.resizeRowToContents(row)
            self.resultTable.setItem(row, 3, errorMsgItem)
            self.resultTable.setItem(row, 4, typeItem)
            self.resultTable.setItemDelegateForColumn(4, ErrorTypeDelegate(self.resultTable))

        header = self.resultTable.horizontalHeader()
        header.setSectionResizeMode(0, QtWidgets.QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(1, QtWidgets.QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(2, QtWidgets.QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(3, QtWidgets.QHeaderView.ResizeMode.Interactive)
        header.setSectionResizeMode(4, QtWidgets.QHeaderView.ResizeMode.ResizeToContents)
        self._filterResultRows(self.search.text())

    def _filterResultRows(self, text):
        for row in range(self.resultTable.rowCount()):
            rowVisible = False
            for col in range(self.resultTable.columnCount()):
                item = self.resultTable.item(row, col)
                if item and text.lower() in item.text().lower():
                    rowVisible = True
                    break
            self.resultTable.setRowHidden(row, not rowVisible)

    def _onRowsSelected(self, selected, deselected):
        self.currentSelectedErrors.clear()
        for index in self.resultTable.selectionModel().selectedRows():
            row = index.row()
            if row < 0 or row >= len(self.results):
                continue
            error = self.results[row]
            self.currentSelectedErrors.append(error)

    def _onCellDoubleClicked(self, row, column):
        if row < 0 or row >= len(self.results):
            return
        error = self.results[row]
        errorSites = error.GetSites()
        if not errorSites:
            return

        self._appController._dataModel.selection.clear()

        primPaths = []
        propPaths = []
        for site in errorSites:
            if site.IsPrim():
                primPaths.append(site.GetPrim().GetPath())
            elif site.IsProperty():
                propPaths.append(site.GetProperty().GetPath())
        
        with self._appController._dataModel.selection.batchPrimChanges:
            self._appController._dataModel.selection.clearPrims()
            for primPath in primPaths:
                self._appController._dataModel.selection.addPrimPath(
                    primPath)

        if not propPaths:
            self._appController._ui.primView.FrameSelection()
            return

        # We can only select properties if these belong to the same prim (and
        # if sites have prim then there is only 1 prim)
        if primPaths and len(primPaths) > 1:
            # too many different prims
            return
        primPath = primPaths[0] if primPaths else propPaths[0].GetPrimPath()
        for prop in propPaths:
            if prop.GetPrimPath() != primPath:
                # properties from different prims
                return

        if propPaths[0].GetPrimPath() not in primPaths:
            with self._appController._dataModel.selection.batchPrimChanges:
                self._appController._dataModel.selection.clearPrims()
                self._appController._dataModel.selection.addPrimPath(
                    propPaths[0].GetPrimPath())

        with self._appController._dataModel.selection.batchPropChanges:
            self._appController._dataModel.selection.clearProps()
            for propPath in propPaths:
                if propPath.IsPropertyPath():
                    self._appController._dataModel.selection.addPropPath(
                        propPath)

        # Default selection data model behavior is not to frame the selected
        # prim but highlight the already expanded ancestor, but here we want
        # to frame the selected prim.
        self._appController._ui.primView.FrameSelection()


class ValidationWidget(QtWidgets.QWidget):
    def __init__(self, appController):
        super().__init__()
        self._appController = appController
        self.setWindowTitle("USD Validation")
        self.resize(1200, 800)

        # Main layout
        layout = QtWidgets.QVBoxLayout(self)

        # Vertical splitter to divide selectors/options and results
        verticalSplitter = QtWidgets.QSplitter(QtCore.Qt.Orientation.Vertical)
        layout.addWidget(verticalSplitter)

        # Horizontal splitter for plugins and selected/options
        topSplitter = QtWidgets.QSplitter(QtCore.Qt.Orientation.Horizontal)

        self.plugins = ValidatorPluginsWidget()
        topSplitter.addWidget(self.plugins)

        # Right side container for selected validators and options, called
        # Validation Content
        rightContainer = QtWidgets.QGroupBox("Validation Context")

        rightSplitter = QtWidgets.QSplitter(QtCore.Qt.Orientation.Horizontal, rightContainer)
        
        self.selected = SelectedValidatorsWidget(rightContainer)
        rightSplitter.addWidget(self.selected)

        self.options = OptionsWidget(rightContainer)
        rightSplitter.addWidget(self.options)

        rightLayout = QtWidgets.QHBoxLayout(rightContainer)
        rightLayout.addWidget(rightSplitter)

        rightLayout.addStretch()

        topSplitter.addWidget(rightContainer)

        topSplitter.setSizes([self.width() * 0.5, self.width() * 0.5])
        topSplitter.setStretchFactor(0, 1)
        topSplitter.setStretchFactor(1, 1)
        rightLayout.setStretch(0, 1)

        verticalSplitter.addWidget(topSplitter)

        self.validationErrors = []
        self.resultWidget = ResultWidget(self._appController)
        verticalSplitter.addWidget(self.resultWidget)

        # Connect validtorToggled signal to update selected validators
        self.plugins.validatorToggled.connect(self.onValidatorToggled)

        # Connect runValidation signal to handle running validation
        self.options.runValidation.connect(self.onRunValidation)

    def updateRunButtonState(self):
        if self.selected.getSelectedValidators() and \
                not self._appController._dataModel.playing:
            self.options.runButton.setEnabled(True)
        else:
            self.options.runButton.setEnabled(False)

    def onValidatorToggled(
            self, pluginName, validatorName, description, enabled):
        if enabled:
            self.selected.addValidator(pluginName, validatorName, description)
        else:
            self.selected.removeValidator(pluginName, validatorName)
        self.updateRunButtonState()

    def onRunValidation(
            self, primsSetFromSelection, includeDescendantPrims, 
            timeRangeOptions):
        from pxr import Gf, Sdf, Usd, UsdValidation
        validatorsMetadata = \
            [UsdValidation.ValidationRegistry().GetValidatorMetadata(name) 
             for name in self.selected.getSelectedValidators()]
        validationContext = UsdValidation.ValidationContext(
            metadata=validatorsMetadata)
        stage = self._appController._dataModel.stage
        if not (validationContext or stage):
            return

        primsToValidate = []
        if primsSetFromSelection:
            primPaths = self._appController._dataModel.selection.getPrimPaths()
            # If absolute root is in the explicitly picked paths, we should do
            # the whole stage anyhow, ignore the rest.
            if not Sdf.Path.absoluteRootPath in primPaths:
                for path in primPaths:
                    primsToValidate.append(stage.GetPrimAtPath(path))

        validationTimeRange = None
        isDefaultTimeIncluded = timeRangeOptions.includeDefaultTime
        if timeRangeOptions.mode == 0:
            validationTimeRange = UsdValidation.ValidationTimeRange(
                Gf.Interval(self._appController._dataModel.currentFrame.GetValue()),
                isDefaultTimeIncluded)
        elif timeRangeOptions.mode == 1:
            validationTimeRange = UsdValidation.ValidationTimeRange(
                Gf.Interval(self._appController._dataModel.frameRangeBegin, 
                            self._appController._dataModel.frameRangeEnd),
                isDefaultTimeIncluded)
        else:
            validationTimeRange = UsdValidation.ValidationTimeRange(
                Gf.Interval(timeRangeOptions.startTime, 
                            timeRangeOptions.endTime),
                isDefaultTimeIncluded)

        if primsToValidate:
            if includeDescendantPrims:
                for prim in primsToValidate:
                    self.validationErrors.extend(
                        validationContext.Validate(Usd.PrimRange(prim)))
            else:
                self.validationErrors = validationContext.Validate(
                    primsToValidate)
        else:
            self.validationErrors = validationContext.Validate(stage)

        self.resultWidget.populateResultsAndUpdateWidget(self.validationErrors)
