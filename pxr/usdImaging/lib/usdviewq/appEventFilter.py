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
# Qt Components
from qt import QtCore, QtGui, QtWidgets

class AppEventFilter(QtCore.QObject):
    '''This class's primary responsibility is delivering key events to
    "the right place".  Given usdview's simplistic approach to shortcuts
    (i.e. just uses the native Qt mechanism that does not allow for 
    context-sensitive keypress dispatching), we take a simplistic approach
    to routing: use Qt's preferred mechanism of processing keyPresses 
    only in widgets that have focus; therefore, the primary behavior of this
    filter is to track mouse-position in order to set widget focus, so that
    widgets with keyboard navigation behaviors operate when the mouse is over
    them.
    
    We add one special behaviors on top of that, which is to turn unaccepted
    left/right events into up/down events for TreeView widgets, because we
    do not have a specialized class on which to provide this nice navigation
    behavior.'''

    # in future it would be a hotkey dispatcher instead of appController
    # that we'd dispatch to, but we don't have one yet
    def __init__(self, appController):
        QtCore.QObject.__init__(self)
        self._appController = appController
        
    def IsNavKey(self, key, modifiers):
        return ((key in (QtCore.Qt.Key_Left, QtCore.Qt.Key_Right,
                         QtCore.Qt.Key_Up, QtCore.Qt.Key_Down,
                         QtCore.Qt.Key_PageUp, QtCore.Qt.Key_PageDown,
                         QtCore.Qt.Key_Home, QtCore.Qt.Key_End))
                and modifiers == QtCore.Qt.NoModifier)
        
    def _IsWindow(self, obj):
        if isinstance(obj, QtWidgets.QWidget):
            return obj.isWindow()
        else:
            return isinstance(obj, QtGui.QWindow)
            
    def TopLevelWindow(self, obj):
        parent = obj.parent()
        return obj if (self._IsWindow(obj) or not parent) else self.TopLevelWindow(parent)

    def WantsNavKeys(self, w):
        if not w or self._IsWindow(w):
            return False
        # The broader test would be QtWidgets.QAbstractItemView,
        # but pragmatically, the TableViews in usdview don't really
        # benefit much from keyboard navigation, and we'd rather
        # allow the arrow keys drive the playhead when such widgets would
        # otherwise get focus
        elif isinstance(w, QtWidgets.QTreeView):
            return True
        else:
            return self.WantsNavKeys(w.parent())

    def NavigableOrTopLevelObject(self, w):
        if (not w or 
            self._IsWindow(w) or 
            isinstance(w, QtWidgets.QTreeView) or
            isinstance(w, QtWidgets.QDialog)):
            return w
        else:
            parent = w.parent()
            return w if not parent else self.NavigableOrTopLevelObject(parent)
        
    def JealousFocus(self, w):
        return (isinstance(w, QtWidgets.QLineEdit) or 
                isinstance(w, QtWidgets.QComboBox) or
                isinstance(w, QtWidgets.QTextEdit) or
                isinstance(w, QtWidgets.QAbstractSlider) or
                isinstance(w, QtWidgets.QAbstractSpinBox))
            
    def SetFocusFromMousePos(self, backupWidget):
        # It's possible the mouse isn't over any of our windows at the time,
        # in which case use the top-level window of backupWidget.
        overObject = QtWidgets.QApplication.widgetAt(QtGui.QCursor.pos())
        topLevelObject = self.NavigableOrTopLevelObject(overObject)
        focusObject = topLevelObject if topLevelObject else self.TopLevelWindow(backupWidget)

        if focusObject and isinstance(focusObject, QtWidgets.QWidget):
            focusObject.setFocus()

    def eventFilter(self, widget, event):
        # There is currently no filtering we want to do for modal or popups
        if (QtWidgets.QApplication.activeModalWidget() or
            QtWidgets.QApplication.activePopupWidget()):
            return False
        
        currFocusWidget = QtWidgets.QApplication.focusWidget()
        
        if event.type() == QtCore.QEvent.KeyPress:
            key = event.key()

            isNavKey = self.IsNavKey(key, event.modifiers())
            if key == QtCore.Qt.Key_Escape:
                # ESC resets focus based on mouse position, regardless of
                # who currently holds focus
                self.SetFocusFromMousePos(widget)
                return True
            elif currFocusWidget and self.JealousFocus(currFocusWidget):
                # Don't touch if there's a greedy focus widget
                return False
            elif (isNavKey and self.WantsNavKeys(currFocusWidget)):
                # Special handling for navigation keys:
                # 1. When a "navigable" widget is focussed (a TreeView), 
                #    route arrow keys to the widget and consume them
                # 2. To make for snappier navigation, when the TreeView
                #    won't accept a left/right because an item is already
                #    opened or closed, turn it into an up/down event.  It
                #    WBN if this behavior could be part of the widgets 
                #    themselves, but currently, usdview does not specialize
                #    a class for its TreeView widgets.
                event.setAccepted(False)
                currFocusWidget.event(event)
                accepted = event.isAccepted()
                if (not accepted  and 
                    key in (QtCore.Qt.Key_Left, QtCore.Qt.Key_Right)):
                    advance = (key == QtCore.Qt.Key_Right)
                    altNavKey = QtCore.Qt.Key_Down if advance else QtCore.Qt.Key_Up
                    subEvent = QtGui.QKeyEvent(QtCore.QEvent.KeyPress,
                                               altNavKey,
                                               event.modifiers())
                    QtWidgets.QApplication.postEvent(currFocusWidget, subEvent)
                event.setAccepted(True)
                return True
            elif isNavKey:
                if self._appController.processNavKeyEvent(event):
                    return True

        elif (event.type() == QtCore.QEvent.MouseMove and 
              not self.JealousFocus(currFocusWidget)):
            self.SetFocusFromMousePos(widget)
            # Note we do not consume the event!
            
        # During startup, Qt seems to queue up events on objects that may
        # have disappeared by the time the eventFilter is called upon.  This
        # is true regardless of how late we install the eventFilter, and
        # whether we process pending events before installing.  So we 
        # silently ignore Runtime errors that occur as a result.
        try:
            return QtCore.QObject.eventFilter(self, widget, event)
        except RuntimeError:
            return True
