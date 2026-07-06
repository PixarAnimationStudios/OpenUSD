#
# Copyright 2026 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
from pxr import Usd, Sdf, Tf, Ts, InvertibleRigsExampleAuthoringCode
from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

def SetAtTime(attr, value, time):
    """
    Sets a value on an attribute at a specified time.

    If setting at the default time, a default value is written. If time samples
    have already been authored on the attribute (which would override a spline),
    or if the attribute's value type isn't supported by splines, authors a time
    sample at the given time. Otherwise, the value is written as a knot in a
    Ts.Spline. If no knot exists at that time, creates a new knot with curve
    interpolation for the segment that follows it.

    NOTE: This implementation is inconsistent in that a weaker spline will be
    lofted into the authoring layer and modified there, but time samples are not
    lofted. If there are weaker time samples, they will be overridden by any
    time samples authored by this function.
    """

    valueType = attr.GetTypeName().type
    
    # If we're setting at the default time, or if the attribute's value type
    # isn't supprted by splines, or if the attribute already has time samples,
    # use Set to write to the default or time samples.
    if (time.IsDefault() or attr.GetNumTimeSamples() > 0 or
        not Ts.Spline.IsSupportedValueType(valueType)):
        attr.Set(value, time)
        return
    
    # Otherwise, write the value as a knot on the attribute's spline.
    frame = time.GetValue()
    spline = attr.GetSpline()
    knot = spline.GetKnot(frame)
    if knot:
        knot.SetValue(value)
    else:
        knot = Ts.Knot(
            typeName=valueType.typeName, time=frame, value=value,
            nextInterp=Ts.InterpCurve)
    spline.SetKnot(knot)
    attr.SetSpline(spline)

def AttributeValueMayHaveChanged(attrPath, notice):
    """
    Returns True if the given notice indicates that the value of the attribute
    at attrPath may have changed.
    """
    # TODO: The paths we get from the notice are in lexographical order, so I
    # _think_ we can probably improve performance here by using bisect.

    if attrPath in notice.GetChangedInfoOnlyPaths():
        return True

    for path in notice.GetResyncedPaths():
        if attrPath.HasPrefix(path):
            return True

    return False

class AttributeValueWidget(QtWidgets.QLineEdit):
    """
    A widget for displaying and authoring the value of a UsdAttribute
        
    The widget text is initialized with the current value of the attribute.
    If the attribute's value type supports munging (int or double), then the
    text is editable by entering a value, or by munging (clicking inside
    the text box and dragging the mouse).
    """

    __mungButton = QtCore.Qt.MouseButton.LeftButton
    __mungSensitivity = 0.02

    def __init__(self, usdAttribute, usdviewApi, parent=None):
        super(AttributeValueWidget, self).__init__(parent)
        
        self._usdviewApi = usdviewApi
        self._usdAttribute = usdAttribute

        # To enable "mung" behavior, this needs to track the position of the
        # mouse and the initial value when the mouse button is first clicked.
        self._mungBaseMousePos = None
        self._mungBaseValue = None

        # Configure the widget for the attribute's value type.
        self._valueTypeName = self._usdAttribute.GetTypeName()
        if self._valueTypeName == Sdf.ValueTypeNames.Int:
            self.setValidator(QtGui.QIntValidator(parent=self))
        elif AttributeValueWidget._IsFloatingPointType(self._valueTypeName):
            self.setValidator(QtGui.QDoubleValidator(parent=self))
        elif (self._valueTypeName == Sdf.ValueTypeNames.Token or 
              self._valueTypeName == Sdf.ValueTypeNames.String):
            pass
        else:
            self.setReadOnly(True)

        # Set the initial value of the widget, and maintain a cache of the 
        # last text value set on the widget. This cached value will be used to
        # avoid unwanted authoring due to spurious editingFinished signals
        # invoking _onEditingFinished().
        self._cachedText = self._TypedValueToString(
            usdAttribute.Get(self._usdviewApi.dataModel.currentFrame))
        self.setText(self._cachedText)

        # Configure the widget to update in response to the frame counter 
        # changing.
        self._usdviewApi.dataModel.currentFrameChanged.connect(
            self._onCurrentFrameChanged)

        # Configure the widget to author the attribute's value when the user
        # is done editing the text.
        self.editingFinished.connect(self._onEditingFinished)

        # Configure the widget to update in response to ObjectsChanged notices.
        self._noticeKey = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged,
            self._onObjectsChanged,
            self._usdAttribute.GetStage())

    @staticmethod
    def _IsFloatingPointType(valueTypeName: Sdf.ValueTypeName):
        """
        Returns true if valueTypeName is a floating point value type (regardless 
        of precision).
        """
        return (valueTypeName == Sdf.ValueTypeNames.Double or
                valueTypeName == Sdf.ValueTypeNames.Float or
                valueTypeName == Sdf.ValueTypeNames.Half)

    @staticmethod
    def _IsNumericType(valueTypeName: Sdf.ValueTypeName):
        """
        Returns true if valueTypeName is either an integer or floating point
        type. 
        """
        return (AttributeValueWidget._IsFloatingPointType(valueTypeName) or 
                valueTypeName == Sdf.ValueTypeNames.Int)

    def _TypedValueToString(self, typedValue):
        """
        Makes sure typed values are displayed nicely in the widget. Primarily, 
        rounds floating point values to a few decimal places. 
        """
        if AttributeValueWidget._IsFloatingPointType(self._valueTypeName):
            return str(round(typedValue, 2))
        
        return str(typedValue)

    def _StringToTypedValue(self, textValue: str):
        """
        Attempts to convert a string to a Python type corresponding to 
        valueTypeName. For numeric types, failure results in a zero value. 
        
        Supports Int, Half, Float, Double, Token and String. Raises a TypeError 
        exception for unsupported types.
        """
        if self._valueTypeName == Sdf.ValueTypeNames.Int:
            try:
                newValue = int(textValue)
            except:
                newValue = 0
        elif AttributeValueWidget._IsFloatingPointType(self._valueTypeName):
            try:
                newValue = float(textValue)
            except:
                newValue = 0.0
        elif (self._valueTypeName == Sdf.ValueTypeNames.Token or
              self._valueTypeName == Sdf.ValueTypeNames.String):
            newValue = textValue
        else:
            raise TypeError(
                f'ValueTypeName {self._valueTypeName} not supported.')
        
        return newValue

    def _authorValue(self, newValue):
        """
        Authors newValue to the underlying UsdAttribute at the current time.
        """
        currentTime = self._usdviewApi.dataModel.currentFrame
        
        # Try authoring the value to the attribute at the current time
        try:
            SetAtTime(self._usdAttribute, newValue, currentTime)
        except Exception as err:
            Tf.Warn(
                f'Failed to set <{self._usdAttribute.GetPath()}> to value: '
                f'{newValue} : {err}')

    def _onCurrentFrameChanged(self, frame):
        """
        Handler method for changes to currentFrame.

        Read the current value of the UsdAttribute and update the widget text.
        """
        self._cachedText = self._TypedValueToString(
            self._usdAttribute.Get(frame))
        self.setText(self._cachedText)

    def _onEditingFinished(self):
        """
        When the user is done editing, author the value to the underlying
        UsdAttribute.
        """
        # If the text in the gui matches our cached value, nothing has changed,
        # so there is no need to try authoring anything.
        if self._cachedText == self.text():
            return

        try:
            newValue = self._StringToTypedValue(self.text())
        except TypeError as err:
            return

        self._authorValue(newValue)

    def _onObjectsChanged(self, notice, stage):
        """
        Handler method for Usd's ObjectsChanged notice.

        Read the current value of the UsdAttribute and update the widget text.
        """
        if AttributeValueMayHaveChanged(self._usdAttribute.GetPath(), notice):
            currentTime = self._usdviewApi.dataModel.currentFrame
            value = self._usdAttribute.Get(currentTime)

            # It is possible to receive 'None' values from UsdAttribute.Get(). 
            # For example, if no value is authored and no fallback value is 
            # provided by the schema for this attribute, or if the attribute is 
            # initialized with an empty spline.
            if value == None:
                return

            self._cachedText = self._TypedValueToString(value)
            self.setText(self._cachedText)

    def mousePressEvent(self, event):
        """
        Begins a mung operation.
        """
        if event.buttons() == self.__mungButton and not self.isReadOnly() and \
            self._IsNumericType(self._valueTypeName):
            self._mungBaseMousePos = event.pos()
            self._mungBaseValue = self._StringToTypedValue(self.text())
            self.setCursor(QtGui.QCursor(QtCore.Qt.SizeHorCursor))
            self.selectAll()
        else:
            super(AttributeValueWidget, self).mousePressEvent(event)

    def mouseReleaseEvent(self, event):
        """
        Ends a mung operation.
        """
        if event.button() == self.__mungButton and not self.isReadOnly():
            self._mungBaseMousePos = None
            self._mungBaseValue = None
            self.setCursor(QtGui.QCursor(QtCore.Qt.IBeamCursor))
        else:
            super(AttributeValueWidget, self).mouseReleaseEvent(event)

    def mouseMoveEvent(self, event):
        """
        If munging, updates the UsdAttribute value in response to the mouse
        position.
        """
        if event.buttons() == self.__mungButton and self._mungBaseMousePos and \
            self._IsNumericType(self._valueTypeName):
            delta = event.pos().x() - self._mungBaseMousePos.x()
            delta *= self.__mungSensitivity
            value = self._mungBaseValue + delta
            self._authorValue(value)
            self.selectAll()
        else:
            super(AttributeValueWidget, self).mouseMoveEvent(event)

class AttributeValueSelectWidget(QtWidgets.QComboBox):
    """
    A widget for displaying and authoring the value of a UsdAttribute whose
    value is selected from a list of possible values.
    """

    def __init__(self, usdAttribute, usdviewApi, parent=None):
        super(AttributeValueSelectWidget, self).__init__(parent)
        
        self._usdviewApi = usdviewApi
        self._usdAttribute = usdAttribute

        if self._usdAttribute is None:
            return

        if self._usdAttribute.HasMetadata('allowedTokens'):
            # The items in the combo box are the allowed tokens for the
            # attribute.
            self.addItems(
                [str(x) for x in
                 self._usdAttribute.GetMetadata('allowedTokens')])
        else:
            Tf.Warn(f"No combo box items for <{self._usdAttribute.GetPath()}>")

        # Set the initial value for the widget.
        self.setCurrentText(
            usdAttribute.Get(self._usdviewApi.dataModel.currentFrame))

        # Configure the widget to update in response to the frame counter 
        # changing.
        self._usdviewApi.dataModel.currentFrameChanged.connect(
            self._onCurrentFrameChanged)

        # Configure the widget to author the attribute's value when the combo
        # box selection changes.
        self.currentIndexChanged.connect(self._onSelectionChanged)

        # Configure the widget to update in response to ObjectsChanged notices.
        self._noticeKey = Tf.Notice.Register(
            Usd.Notice.ObjectsChanged,
            self._onObjectsChanged,
            self._usdAttribute.GetStage())

    def _onCurrentFrameChanged(self, frame):
        """
        Handler method for changes to currentFrame.

        Read the current value of the UsdAttribute and update the selected item.
        Block signals while doing so to avoid sending a currentIndexChanged
        signal (whose handler invokes an authoring operation).
        """

        self.blockSignals(True)
        self.setCurrentText(self._usdAttribute.Get(time=frame))
        self.blockSignals(False)

    def _onSelectionChanged(self, index):
        """
        When the selection changes, author the value to the attribute.
        """
        newValue = self._usdAttribute.GetMetadata('allowedTokens')[index]
        currentTime = self._usdviewApi.dataModel.currentFrame

        try:
            SetAtTime(self._usdAttribute, newValue, currentTime)
        except Exception as err:
            Tf.Warn(
                f'Failed to set <{self._usdAttribute.GetPath()}> to value: '
                f'{newValue} : {err}')

    def _onObjectsChanged(self, notice, stage):
        """
        Handler method for the ObjectsChanged notice.

        Read the current value of the UsdAttribute and update the selected item.
        Block signals while doing so to avoid sending a currentIndexChanged
        signal (whose handler invokes an authoring operation).
        """
        if AttributeValueMayHaveChanged(self._usdAttribute.GetPath(), notice):
            currentTime = self._usdviewApi.dataModel.currentFrame
            self.blockSignals(True)
            self.setCurrentText(self._usdAttribute.Get(currentTime))
            self.blockSignals(False)

class AttributeValueSelectWidgetWithCompensation(AttributeValueSelectWidget):
    
    def __init__(self, switchAttribute, usdviewApi, parent=None):
        super(AttributeValueSelectWidgetWithCompensation, self) \
            .__init__(switchAttribute, usdviewApi, parent)

        self._authoring = InvertibleRigsExampleAuthoringCode.Authoring(
            self._usdviewApi.dataModel.stage)
        self._switchAttribute = switchAttribute

    def _onSelectionChanged(self, index):
        """
        When the selection changes, author the value to the attribute with 
        compensation.
        """
        newValue = self._usdAttribute.GetMetadata('allowedTokens')[index]
        currentTime = self._usdviewApi.dataModel.currentFrame
        
        try:
            self._authoring.CompensateSwitch(self._switchAttribute, 
                                             currentTime,
                                             newValue)
        except Exception as err:
            Tf.Warn(f'Caught exception while calling CompensateSwitch(): {err}')
        
class AuthoringWindow(QtWidgets.QWidget):

    __instance = None

    @classmethod
    def GetInstance(cls, usdviewApi):
        if not cls.__instance:
            cls.__instance = AuthoringWindow(usdviewApi)
        return cls.__instance

    def __init__(self, usdviewApi):
        super(AuthoringWindow, self).__init__(usdviewApi.qMainWindow, 
            QtCore.Qt.WindowType.Window)
        
        self._usdviewApi = usdviewApi

        # Set window details.
        self.setGeometry(0, 0, 640, 480)
        self.setWindowTitle('OpenExec: Invertible Rig Demo')

        # Create the root layout, which will add widgets and nested layouts
        # in a horizontal fashion
        self._rootLayout = QtWidgets.QHBoxLayout()
        self.setLayout(self._rootLayout)

        # Create and add the nested widgets to the root layout
        self._setupSceneBrowserTreeWidget()
        self._setupDemoControlsWidget()

    def _setupSceneBrowserTreeWidget(self):

        # Start with a QScrollArea as a container widget
        self._scrollArea = QtWidgets.QScrollArea()
        self._scrollArea.setWidgetResizable(True)
        self._rootLayout.addWidget(self._scrollArea)

        # We're using the model/view architecture. 
        # First create the model (we'll configure it below)
        model = QtGui.QStandardItemModel()
        
        # The view is a QTreeView. Create one of these and set its model to the 
        # above
        self._treeView = QtWidgets.QTreeView()
        self._treeView.setModel(model)      

        # The view is the widget that goes in the scroll area
        self._scrollArea.setWidget(self._treeView)

        # Now configure the model. 

        # First set the header labels
        model.setHorizontalHeaderLabels(['Path', 'Value'])

        # And now build the tree
        for prim in self._usdviewApi.dataModel.stage.TraverseAll():

            # Each entry consists of two QStandardItems (one for each column)
            
            # Path Column: Display name is just the leaf of the path
            itemPath = QtGui.QStandardItem(prim.GetPath().name)
            # Path Column: holds user data, which is the whole path 
            # (so we can search for it later)
            itemPath.setData(prim.GetPath().pathString, QtCore.Qt.UserRole)

            # Value Column: empty for now
            itemValue = QtGui.QStandardItem()

            # look for an index whose item matches our parent path
            start_index = model.index(0, 0)
            hits = model.match(
                start_index,
                QtCore.Qt.UserRole,
                prim.GetPath().GetParentPath().pathString,
                -1,
                QtCore.Qt.MatchExactly | QtCore.Qt.MatchRecursive
            )

            # if we found something, it should be unique. 
            # appendRow to it; otherwise to the model (i.e. root)
            if len(hits) != 0:
                item = model.itemFromIndex(hits[0])
                item.appendRow([itemPath, itemValue])
            else:
                model.appendRow([itemPath, itemValue])

            for attribute in prim.GetAttributes():
                # create items for attribute-typed rows
                labelItem = QtGui.QStandardItem(attribute.GetPath().name)
                valueItem = QtGui.QStandardItem()
                # append it to the current prim
                itemPath.appendRow([labelItem, valueItem])

                # Place an attribute editing widget in the value column
                index = model.indexFromItem(valueItem)
                if (attribute.GetTypeName() == Sdf.ValueTypeNames.Token and
                    attribute.HasMetadata('allowedTokens')):
                    valueWidget = AttributeValueSelectWidget(attribute, 
                                                             self._usdviewApi)
                else:
                    valueWidget = AttributeValueWidget(attribute, 
                                                       self._usdviewApi)
                    
                self._treeView.setIndexWidget(index, valueWidget)

        # Expand the tree and resize the headers
        self._treeView.expandAll()    
        self._treeView.header().setSectionResizeMode(0, 
            QtWidgets.QHeaderView.ResizeToContents)

    def _setupDemoControlsWidget(self):

        # Add a vertical box layout to the root layout
        self._vLayout = QtWidgets.QVBoxLayout()
        self._rootLayout.addLayout(self._vLayout)

        # The vertical box layout's first widget is an unnamed group box
        self._groupBox = QtWidgets.QGroupBox('')
        self._groupBox.setMinimumWidth(180)
        self._vLayout.addWidget(self._groupBox)

        # The group box contains a form layout that takes name:widget pairs.
        # Align it right, allow it to grow with its surroundings, and give it 
        # comfortable padding.
        self._formLayout = QtWidgets.QFormLayout()
        self._formLayout.setLabelAlignment(QtCore.Qt.AlignmentFlag.AlignRight)
        self._formLayout.setFieldGrowthPolicy(
            QtWidgets.QFormLayout.AllNonFixedFieldsGrow)
        self._formLayout.setContentsMargins(12, 12, 12, 12)
        self._groupBox.setLayout(self._formLayout)

        # Set up the rig selector switch attribute, and use an
        # AttributeValueSelectWidgetWithCompensation widget as a gui control
        self._switchAttribute = None

        stage = self._usdviewApi.dataModel.stage
        query = InvertibleRigsExampleAuthoringCode.Query(stage)
        sources = query.FindSwitchAvars("switch")
        foundSwitchAvar = len(sources) == 1
        
        if foundSwitchAvar:
            self._switchAttribute = sources[0]
        else:
            Tf.Warn("Can't find switch attribute")
        
        # Note that self._switchAttribute will be None if the search failed. 
        # We still build the widget but disable it below.
        self._rigSelection = AttributeValueSelectWidgetWithCompensation(
            self._switchAttribute, self._usdviewApi)

        # Add it to the form layout
        self._formLayout.addRow('Select rig:', self._rigSelection)

        # Add a QPushButton for breaking down relevant avars
        self._breakdownButton = QtWidgets.QPushButton('Breakdown')
        self._breakdownButton.clicked.connect(self._onBreakdownClicked)
        self._formLayout.addRow('', self._breakdownButton)

        # Disable widgets that depend on the switch avar if the search failed
        if not foundSwitchAvar:
            self._rigSelection.setEnabled(False)
            self._breakdownButton.setEnabled(False)

        # Add a QPushButton for clearing authoring
        self._clearButton = QtWidgets.QPushButton('Clear authoring')
        self._clearButton.clicked.connect(self._onClearButtonClicked)
        self._formLayout.addRow('', self._clearButton)

    def _onBreakdownClicked(self):
        """
        For all avars that contribute to the pose, author spline knots or time 
        samples at the current time, with the current value of the avar.
        """
        authoring = InvertibleRigsExampleAuthoringCode.Authoring(
            self._usdviewApi.dataModel.stage)
        
        try:
            authoring.BreakdownInputAvars(self._switchAttribute,
                self._usdviewApi.dataModel.currentFrame)
        except Exception as err:
            Tf.Warn(f'Exception while calling BreakdownInputAvars: {err}')

    def _onClearButtonClicked(self):
        """
        Clear all authoring on the current layer.
        """
        stage = self._usdviewApi.dataModel.stage

        currentLayer = stage.GetEditTarget().GetLayer()
        currentLayer.Clear()
        
def invertibleRigDemo(usdviewApi):
    window = AuthoringWindow.GetInstance(usdviewApi)
    window.show()
