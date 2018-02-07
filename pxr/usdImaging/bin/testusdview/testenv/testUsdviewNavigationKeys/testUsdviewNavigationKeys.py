#!/pxrpythonsubst
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

# Leaving these in for future debugging
from __future__ import print_function
import sys

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

def _emitCollapseAllAction(appController):
    appController._ui.actionCollapse_All.triggered.emit() 
    QtWidgets.QApplication.processEvents()

def _popupViewMenu(appController):
    appController._ui.menuView.exec_()
    QtWidgets.QApplication.processEvents()

def _postAndProcessKeyEvent(key, widget):
    event = QtGui.QKeyEvent(QtCore.QEvent.KeyPress,
                            key,
                            QtCore.Qt.NoModifier)
    QtWidgets.QApplication.postEvent(widget, event)
    QtWidgets.QApplication.processEvents()

class EscapeSender(QtCore.QObject):
    def __init__(self, receiver):
        QtCore.QObject.__init__(self)
        self._receiver = receiver

    def doIt(self):
        _postAndProcessKeyEvent(QtCore.Qt.Key_Escape, self._receiver)

def _testBasic(appController):
    from pxr import Sdf

    #
    # First test that right/left keys sent to a TreeView will navigate the 
    # hierarchy, including moving between siblings and cousins.
    #

    # Start out fully collapsed, and make sure PrimView has focus
    _emitCollapseAllAction(appController)
    appController._ui.primView.setFocus()
    QtWidgets.QApplication.processEvents()

    # Send events to the application's QObject to ensure our app filter
    # reroutes it to the focusWidget.
    appObj = QtWidgets.QApplication.instance()

    # Get the stage and selection data model to query selection.
    stage = appController._dataModel.stage
    selectionDataModel = appController._dataModel.selection

    # This test is highly asset-dependent.  Our scene has a single hierarchy
    # chain, so it takes two "move right" actions to select each level of
    # path hierarchy
    path = Sdf.Path("/World/sets/setModel")
    for i in xrange(2 * path.pathElementCount):
        _postAndProcessKeyEvent(QtCore.Qt.Key_Right, appObj)

    assert len(selectionDataModel.getPrims()) == 1
    assert selectionDataModel.getFocusPrim().GetPrimPath() == path

    # Now roll it all back up
    for i in xrange(1, 2 * path.pathElementCount):
        # Send the event to mainWindow to ensure our app filter reroutes it
        # to the focusWidget.
        _postAndProcessKeyEvent(QtCore.Qt.Key_Left, appObj)

    assert len(selectionDataModel.getPrims()) == 1
    assert selectionDataModel.getFocusPrim().IsPseudoRoot()
    
    # Then test that right/left keys sent to other widgets (including the 
    # MainWindow) will result in transport movement
    startFrame = stage.GetStartTimeCode()
    appController._mainWindow.setFocus()
    assert appController._dataModel.currentFrame == startFrame

    _postAndProcessKeyEvent(QtCore.Qt.Key_Right, appObj)
    assert appController._dataModel.currentFrame == startFrame + 1

    _postAndProcessKeyEvent(QtCore.Qt.Key_Left, appObj)
    assert appController._dataModel.currentFrame == startFrame

    # Regression tests for bugs #154716, 154665: Make sure we don't try
    # to filter events while popups or modal dialogs are active.  The best
    # way to test this is to ensure that ESC operates as a terminator for
    # menus and modals, since our filter changes it to be a focus-changer.
    # If the filter is still active (FAILURE), then the test will not
    # terminate, and eventually be killed.
    escSender = EscapeSender(appController._ui.menuView)
    QtCore.QTimer.singleShot(500, escSender, QtCore.SLOT("doIt()"))
    _popupViewMenu(appController)
    
    # Modal dialogs won't receive events sent to the application object,
    # so we must send it to the widget itself. Which means we can't use any
    # of usdview's modals, since we only use static Qt methods that don't
    # return you a widget.
    fileDlg = QtWidgets.QFileDialog(appController._mainWindow)
    escSender = EscapeSender(fileDlg)
    QtCore.QTimer.singleShot(500, escSender, QtCore.SLOT("doIt()"))
    # Causes a modal dialog to pop up
    fileDlg.exec_()

    # Unfortunately, we don't have a good way of testing the "choose focus
    # widget based on mouse position" behavior.  Will keep working on that...

def testUsdviewInputFunction(appController):
    _testBasic(appController)
