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

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

def _emitCollapseAllAction(appController):
    appController._ui.actionCollapse_All.triggered.emit() 
    QtWidgets.QApplication.processEvents()

def _postAndProcessKeyEvent(key, widget):
    event = QtGui.QKeyEvent(QtCore.QEvent.KeyPress,
                            key,
                            QtCore.Qt.NoModifier)
    QtWidgets.QApplication.postEvent(widget, event)
    QtWidgets.QApplication.processEvents()

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

    # This test is highly asset-dependent.  Our scene has a single hierarchy
    # chain, so it takes two "move right" actions to select each level of
    # path hierarchy
    path = Sdf.Path("/World/sets/setModel")
    for i in xrange(2 * path.pathElementCount):
        _postAndProcessKeyEvent(QtCore.Qt.Key_Right, appObj)

    # XXX Change to using selection datamodel when available
    assert appController._currentPrims and len(appController._currentPrims) == 1
    selected = appController._currentPrims[0]
    assert selected.GetPath() == path

    # Now roll it all back up
    for i in xrange(1, 2 * path.pathElementCount):
        # Send the event to mainWindow to ensure our app filter reroutes it
        # to the focusWidget.
        _postAndProcessKeyEvent(QtCore.Qt.Key_Left, appObj)

    # XXX Change to using selection datamodel when available
    assert appController._currentPrims and len(appController._currentPrims) == 1
    selected = appController._currentPrims[0]
    assert selected.IsPseudoRoot()
    
    # Then test that right/left keys sent to other widgets (including the 
    # MainWindow) will result in transport movement
    startFrame = appController._rootDataModel.stage.GetStartTimeCode()
    appController._mainWindow.setFocus()
    assert appController._currentFrame == startFrame

    _postAndProcessKeyEvent(QtCore.Qt.Key_Right, appObj)
    assert appController._currentFrame == startFrame + 1

    _postAndProcessKeyEvent(QtCore.Qt.Key_Left, appObj)
    assert appController._currentFrame == startFrame


    # Unfortunately, we don't have a good way of testing the "choose focus
    # widget based on mouse position" behavior.  Will keep working on that...

def testUsdviewInputFunction(appController):
    _testBasic(appController)
