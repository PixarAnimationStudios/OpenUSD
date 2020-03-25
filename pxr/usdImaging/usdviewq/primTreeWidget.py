#
# Copyright 2018 Pixar
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
from .constantGroup import ConstantGroup
from pxr import Sdf, Usd, UsdGeom
from .primViewItem import PrimViewItem
from .common import PrintWarning, Timer, UIPrimTreeColors, KeyboardShortcuts

def _GetPropertySpecInSessionLayer(usdAttribute):
    propertyStack = usdAttribute.GetPropertyStack(Usd.TimeCode.Default())
    if len(propertyStack) > 0:
        stageSessionLayer = usdAttribute.GetStage().GetSessionLayer()
        return stageSessionLayer.GetPropertyAtPath(usdAttribute.GetPath())
    return None

# Function for getting the background color of the item for the delegates.
# Returns none if we only want the default paint method. 
def _GetBackgroundColor(item, option):
    mouseOver = option.state & QtWidgets.QStyle.State_MouseOver
    selected = option.state & QtWidgets.QStyle.State_Selected
    pressed = option.state & QtWidgets.QStyle.State_Sunken

    background = None

    if item.ancestorOfSelected:
        background = UIPrimTreeColors.ANCESTOR_OF_SELECTED
        if mouseOver:
            background = UIPrimTreeColors.ANCESTOR_OF_SELECTED_HOVER
        if selected:
            background = UIPrimTreeColors.SELECTED
            if mouseOver:
                background = UIPrimTreeColors.SELECTED_HOVER

    else:
        if not selected and not pressed and mouseOver:
            background = UIPrimTreeColors.UNSELECTED_HOVER

        if selected:
            background = UIPrimTreeColors.SELECTED
            if mouseOver:
                background = UIPrimTreeColors.SELECTED_HOVER

    return background

class PrimViewColumnIndex(ConstantGroup):
    NAME, TYPE, VIS, DRAWMODE = range(4)

class DrawModes(ConstantGroup):
    DEFAULT = "default"
    CARDS = "cards"
    BOUNDS = "bounds"
    ORIGIN = "origin"

class DrawModeComboBox(QtWidgets.QComboBox):
    """ Specialize from QComboBox, so that we can send a signal when the pop-up
        is hidden.
    """
    signalPopupHidden = QtCore.Signal()

    def __init__(self, parent=None):
        QtWidgets.QComboBox.__init__(self, parent)

    def hidePopup(self):
        QtWidgets.QComboBox.hidePopup(self)
        self.signalPopupHidden.emit()

class DrawModeWidget(QtWidgets.QWidget):
    """ This widget contains a combobox for selecting the draw mode and a 
        clear ('x') button for clearing an authored drawMode override in the 
        session layer. 
    """
    def __init__(self, primViewItem, refreshFunc, printTiming=False, 
                 parent=None):
        QtWidgets.QWidget.__init__(self, parent)

        self._primViewItem = primViewItem
        self._layout = QtWidgets.QHBoxLayout()
        self._layout.setSpacing(0)
        self._layout.setContentsMargins(0,0,0,0)
        self.setLayout(self._layout)
        
        self._comboBox = DrawModeComboBox(self)
        self._modelAPI = UsdGeom.ModelAPI(self._primViewItem.prim)
        # Reducing the width further causes the pop-up dialog to be trimmed 
        # and option text to be pruned.
        self._comboBox.setFixedWidth(100)
        self._comboBox.addItem(DrawModes.DEFAULT)
        self._comboBox.addItem(DrawModes.CARDS)
        self._comboBox.addItem(DrawModes.BOUNDS)
        self._comboBox.addItem(DrawModes.ORIGIN)
        self._layout.addWidget(self._comboBox)

        self._clearButton = QtWidgets.QToolButton(self)
        self._clearButton.setText('X')
        self._clearButton.setFixedSize(16, 16)
        retainSizePolicy = self._clearButton.sizePolicy()
        retainSizePolicy.setRetainSizeWhenHidden(True)
        self._clearButton.setSizePolicy(retainSizePolicy)
        self._layout.addWidget(self._clearButton)            
        
        self._currentDrawMode = None
        self.RefreshDrawMode()
        self._firstPaint = True

        self._refreshFunc = refreshFunc
        self._printTiming = printTiming

        self._comboBox.signalPopupHidden.connect(self._PopupHidden)
        self._comboBox.activated.connect(self._UpdateDrawMode)
        self._clearButton.clicked.connect(self._ClearDrawMode)

    def paintEvent(self, event):
        # Virtual override of the paintEvent method on QWidget.
        # Popup the combo box the first time the widget is drawn, since 
        # mouse-down in the column makes the widget appear.
        # 
        # An alternative approach would be to set a timer in a constructor to 
        # pop open the combo box at the end of the event loop, but it causes a 
        # noticeable flicker in the UI.
        if self._firstPaint:
            self._comboBox.showPopup()
            self._firstPaint = False

        # Invoke paintEvent on super class to do the actual painting of the 
        # widget.
        super(DrawModeWidget, self).paintEvent(event)

    def _ShouldHideClearButton(self):
        # Check if there's an authored value in the session layer.
        drawModeAttr = self._modelAPI.GetModelDrawModeAttr()
        
        if drawModeAttr:
            sessionSpec = _GetPropertySpecInSessionLayer(drawModeAttr)
            if sessionSpec and sessionSpec.HasDefaultValue():
                return False
        return True

    def RefreshDrawMode(self, currentDrawMode=None):
        self._currentDrawMode = currentDrawMode if currentDrawMode else \
                                self._modelAPI.ComputeModelDrawMode()
        self._comboBox.setCurrentText(self._currentDrawMode)

        clearButtonIsHidden = self._clearButton.isHidden()
        if self._ShouldHideClearButton():
            if not clearButtonIsHidden:
                self._clearButton.hide()
                self.update()
        else:
            if clearButtonIsHidden:
                self._clearButton.show()
                self.update()

    def _UpdateDrawMode(self):
        newDrawModeSelection = str(self._comboBox.currentText())
        currentDrawMode = self._modelAPI.ComputeModelDrawMode()
        if currentDrawMode != newDrawModeSelection:
            with Timer() as t:
                self._modelAPI.CreateModelDrawModeAttr().Set(
                        newDrawModeSelection)

                self.RefreshDrawMode(currentDrawMode=newDrawModeSelection)

                # We need to redraw the scene to pick up updates to draw mode.
                self._refreshFunc(self._primViewItem)
            if self._printTiming:
                t.PrintTime("change model:drawMode on <%s> to %s" % 
                        (self._modelAPI.GetPath(), newDrawModeSelection))
        self._CloseEditorIfNoEdit()

    def _ClearDrawMode(self):
        with Timer() as t:
            drawModeAttr = self._modelAPI.GetModelDrawModeAttr()
            if drawModeAttr:
                sessionSpec = _GetPropertySpecInSessionLayer(drawModeAttr)
                if sessionSpec:
                    self._primViewItem.drawModeWidget = None
                    self._primViewItem.treeWidget().closePersistentEditor(
                            self._primViewItem, PrimViewColumnIndex.DRAWMODE)

                    sessionSpec.ClearDefaultValue()
                    sessionSpec.layer.ScheduleRemoveIfInert(sessionSpec)
                    self._refreshFunc(self._primViewItem)
                else:
                    PrintWarning(self._modelAPI.GetPath(), "Failed to get "
                        "session layer spec for the model:drawMode attribute")
                    return
            else:
                PrintWarning(self._modelAPI.GetPath(), "Failed to get "
                    "model:drawMode attribute")
                return
        if self._printTiming:
            t.PrintTime("clear model:drawMode on <%s> to %s" % 
                        (self._modelAPI.GetPath(), 
                         self._comboBox.currentText()))

    def _CloseEditorIfNoEdit(self):
        # If the clear button isn't present, then there's no edit.
        if self._clearButton.isHidden():
            self._primViewItem.drawModeWidget = None
            self._primViewItem.treeWidget().closePersistentEditor(
                    self._primViewItem, PrimViewColumnIndex.DRAWMODE)

    def _PopupHidden(self):
        # Schedule closing the editor if no edit was made.
        QtCore.QTimer.singleShot(0, self._CloseEditorIfNoEdit)

class DrawModeItemDelegate(QtWidgets.QStyledItemDelegate):
    def __init__(self, printTiming, parent=None):
        QtWidgets.QStyledItemDelegate.__init__(self, parent=parent)
        self._treeWidget = parent
        self._printTiming = printTiming

    # We need to override paint in this delegate as well so that the
    # Draw Mode column will match with the behavior of the other
    # items in the treeview. 
    def paint(self, painter, option, index):
        primViewItem = self._treeWidget.itemFromIndex(index)
        background = _GetBackgroundColor(primViewItem, option)
        if background:
            painter.fillRect(option.rect, background)

        super(DrawModeItemDelegate, self).paint(painter, option, index)

    def createEditor(self, parent, option, index):
        primViewItem = self._treeWidget.itemFromIndex(index)

        if not primViewItem.supportsDrawMode:
            return None

        drawModeWidget = DrawModeWidget(primViewItem, 
            refreshFunc=self._treeWidget.UpdatePrimViewDrawMode,
            printTiming=self._printTiming,
            parent=parent)
        # Store a copy of the widget in the primViewItem, for use when 
        # propagating changes to draw mode down a prim hierarchy.
        primViewItem.drawModeWidget = drawModeWidget
        return drawModeWidget

class SelectedAncestorItemDelegate(QtWidgets.QStyledItemDelegate):
    def __init__(self, parent=None):
        QtWidgets.QStyledItemDelegate.__init__(self, parent=parent)
        self._treeWidget = parent

    # In order to highlight the ancestors of selected prims, we require
    # a new delegate to be created to override the paint function.
    # Because the stylesheet will override styling when items are hovered
    # over, the hovering styling logic is moved to this delegate, and 
    # primTreeWidget is excluded from the selectors for hovering
    # in the stylesheet. 
    def paint(self, painter, option, index):
        primViewItem = self._treeWidget.itemFromIndex(index)

        originalPosition = option.rect.left()
        offsetPosition = self._treeWidget.header().offset()

        # In order to fill in the entire cell for Prim Name, we must update the 
        # dimensions of the rectangle painted. If the column is for Prim Name,
        # we set the left side of the rectangle to be equal to the offset of the
        # tree widget's header. 
        background = _GetBackgroundColor(primViewItem, option)

        if primViewItem.ancestorOfSelected:
            if index.column() == PrimViewColumnIndex.NAME:
                option.rect.setLeft(offsetPosition)

        if background:
            painter.fillRect(option.rect, background)

        # resetting the dimensions of the rectangle so that we paint the correct
        # content in the cells on top of the colors we previously painted.
        option.rect.setLeft(originalPosition)
        super(SelectedAncestorItemDelegate, self).paint(painter, option, index)


class PrimItemSelectionModel(QtCore.QItemSelectionModel):
    """Standard QItemSelectionModel does not allow one to have full-item
    selection while exlcuding some columns in the view from activating
    selection.  Since that's exactly the behavior we want, we derive our
    own class that we can force to ignore selection requests except when we
    really want it to."""
    
    def __init__(self, model):
        super(PrimItemSelectionModel, self).__init__(model)
        self._processSelections = True

    @property
    def processSelections(self):
        """If True, calls to clear(), reset(), and select() will function
        as normal.  If False, such calls will be ignored."""
        return self._processSelections

    @processSelections.setter
    def processSelections(self, doProcess):
        self._processSelections = doProcess

    def clear(self):
        if self.processSelections:
            super(PrimItemSelectionModel, self).clear()

    def reset(self):
        if self.processSelections:
            super(PrimItemSelectionModel, self).reset()

    def select(self, indexOrSelection, command):
        if self.processSelections:
            super(PrimItemSelectionModel, self).select(indexOrSelection, command)


class SelectionEnabler(object):
    def __init__(self, selectionModel):
        self._selectionModel = selectionModel
        self._selectionWasEnabled = False

    def __enter__(self):
        self._selectionWasEnabled = self._selectionModel.processSelections
        self._selectionModel.processSelections = True
        return self

    def __exit__(self, *args):
        self._selectionModel.processSelections = self._selectionWasEnabled

# This class extends QTreeWidget and is used to draw the prim tree view in 
# usdview. 
# More of the prim browser specific behavior needs to be migrated from 
# appController into this class.
class PrimTreeWidget(QtWidgets.QTreeWidget):
    def __init__(self, parent):
        super(PrimTreeWidget, self).__init__(parent=parent)
        self._appController = None
        self._selectionModel = PrimItemSelectionModel(self.model())
        self.setSelectionModel(self._selectionModel)
        # The list of ancestors of currently selected items
        self._ancestorsOfSelected = []

    def InitControllers(self, appController):
        self._appController = appController
        selectedAncestorItemDelegate = SelectedAncestorItemDelegate(parent=self)
        self.setItemDelegate(selectedAncestorItemDelegate)
        drawModeItemDelegate = DrawModeItemDelegate(appController._printTiming, 
                                                    parent=self)
        self.setItemDelegateForColumn(PrimViewColumnIndex.DRAWMODE, 
                                      drawModeItemDelegate)

    def ShowDrawModeWidgetForItem(self, primViewItem):
        self.openPersistentEditor(primViewItem, PrimViewColumnIndex.DRAWMODE)

    def UpdatePrimViewDrawMode(self, rootItem=None):
        """Updates browser's "Draw Mode" columns."""
        with Timer() as t:
            self.setUpdatesEnabled(False)
            # Update draw-model for the entire prim tree if the given
            # rootItem is None.
            if rootItem is None:
                rootItem = self.invisibleRootItem().child(0)
            if rootItem.childCount() == 0:
                self._appController._populateChildren(rootItem)
            rootsToProcess = [rootItem.child(i) for i in 
                    range(rootItem.childCount())]
            for item in rootsToProcess:
                PrimViewItem.propagateDrawMode(item, self)
            self.setUpdatesEnabled(True)
        if self._appController._printTiming:
            t.PrintTime("update draw mode column")

    def ColumnPressCausesSelection(self, col):
        """If this method returns True for column `col`, then we want a
        click in that column to cause the item to be selected."""
        return col != PrimViewColumnIndex.VIS and col != PrimViewColumnIndex.DRAWMODE

    def ExpandItemRecursively(self, item):
        item = item.parent()
        while item.parent():
            if not item.isExpanded():
                self.expandItem(item)
            item = item.parent()

    def FrameSelection(self):
        if (self._appController):
            selectedItems = [
                self._appController._getItemAtPath(prim.GetPath())
                for prim in self._appController._dataModel.selection.getPrims()]

            for item in selectedItems:
                self.ExpandItemRecursively(item)

            self.scrollToItem(selectedItems[0])

    # We set selectability based on the column we mousePress'd in, and then
    # restore to true when leaving the widget, so that when we're not
    # interacting with the browser, anyone can modify selection through the
    # regular API's, which is important for the appController and other
    # clients.  If we retore it *before* leaving the widget, some internal
    # QTreeWidget mechanism (an event filter?) _occasionally_ selects the item
    # after a mouse release!
    def mousePressEvent(self, ev):
        item = self.itemAt(QtCore.QPoint(ev.x(), ev.y()))
        if item:
            col = self.columnAt(ev.x())
            self._selectionModel.processSelections = self.ColumnPressCausesSelection(col)
        super(PrimTreeWidget, self).mousePressEvent(ev)

    def leaveEvent(self, ev):
        super(PrimTreeWidget, self).leaveEvent(ev)
        self._selectionModel.processSelections = True

    # We override these selection and interaction-related API, and provide a
    # batch wrapper for QTreeWidgetItem.setSelected in case, in the future,
    # we have user plugins firing while we are still interacting with this
    # widget, and they manipulate selection.
    def clearSelection(self):
        self._resetAncestorsOfSelected()
        with SelectionEnabler(self._selectionModel):
            super(PrimTreeWidget, self).clearSelection()

    def reset(self):
        self._resetAncestorsOfSelected()
        with SelectionEnabler(self._selectionModel):
            super(PrimTreeWidget, self).reset()

    def selectAll(self):
        self._resetAncestorsOfSelected()
        with SelectionEnabler(self._selectionModel):
            super(PrimTreeWidget, self).selectAll()

    def keyPressEvent(self, ev):
        with SelectionEnabler(self._selectionModel):
            # Because setCurrentItem autoexpands the primview so that
            # the current item is visible, we must set the current item
            # only when we know it'll be needed for arrow navigation.
            # We call this here before the selection data model clears
            # the selection. 

            if ev.key() == QtCore.Qt.Key_Down \
            or ev.key() == QtCore.Qt.Key_Up \
            or ev.key() == QtCore.Qt.Key_Right \
            or ev.key() == QtCore.Qt.Key_Left:
                currentPrim = self._appController._dataModel.selection.getFocusPrim()
                currentItem = self._appController._getItemAtPath(currentPrim.GetPath())
                self.setCurrentItem(currentItem, 0, QtCore.QItemSelectionModel.NoUpdate)

            super(PrimTreeWidget, self).keyPressEvent(ev)

            # Handling the F hotkey comes after the super class's event handling,
            # since the default behavior in the super class's keyPressEvent function
            # is to set the current item to the first item found that alphabetically
            # matches the key entered. Since we want to override this behavior for
            # the F hotkey, we must handle it after the super class's keyPressEvent. 
            if ev.key() == KeyboardShortcuts.FramingKey:
                self.FrameSelection()

    def keyReleaseEvent(self, ev):
        with SelectionEnabler(self._selectionModel):
            super(PrimTreeWidget, self).keyReleaseEvent(ev)

    def updateSelection(self, added, removed):
        """Mutate the widget's selected items, selecting items in `added`
        and deselecting items in `removed`.  Prefer this method for client
        use over calling setSelected directly on PrimViewItems."""
        with SelectionEnabler(self._selectionModel):
            for item in added:
                item.setSelected(True)
            for item in removed:
                item.setSelected(False)
        self._refreshAncestorsOfSelected()
        # This is a big hammer... if we instead built up a list of the
        # ModelIndices of all the changed ancestors, we could instead make
        # our selectionModel emit selectionChanged for just those items,
        # instead.  Does not currently seem to be impacting interactivity.
        QtWidgets.QWidget.update(self)

    def _resetAncestorsOfSelected(self):
        for item in self._ancestorsOfSelected:
            item.ancestorOfSelected = False
        self._ancestorsOfSelected = []

    # Refresh the list of ancestors of selected prims
    def _refreshAncestorsOfSelected(self):
        selectedItems = [
            self._appController._getItemAtPath(prim.GetPath())
            for prim in self._appController._dataModel.selection.getPrims()]

        self._resetAncestorsOfSelected()

        # Add all ancestor of the prim associated with the item
        # to _ancestorsOfSelected, and mark them as selected ancestors
        for item in selectedItems:
            while item.parent():
                item.parent().ancestorOfSelected = True
                self._ancestorsOfSelected.append(item.parent())
                item = item.parent()
