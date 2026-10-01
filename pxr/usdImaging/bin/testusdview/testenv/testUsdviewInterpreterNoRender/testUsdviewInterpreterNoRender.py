#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# Leaving these in for future debugging
from __future__ import print_function
import sys

from pxr.Usdviewq.qt import QtCore, QtGui, QtWidgets

def _emitShowInterpreter(appController):
    appController._ui.showInterpreter.triggered.emit() 
    QtWidgets.QApplication.processEvents()

def _postKeyPress(key, widget):
    event = QtGui.QKeyEvent(QtCore.QEvent.Type.KeyPress,
                            key,
                            QtCore.Qt.KeyboardModifier.NoModifier)
    QtWidgets.QApplication.postEvent(widget, event)
    QtWidgets.QApplication.processEvents()

def _testInterpreterWorks(appController):
    #
    # Trigger the interpreter console to display, then verify it worked
    # by making sure the appController._console's context got initialized
    # properly.  This is somewhat unfortunate a test, but sending key events
    # to the console to mimic typing doesn't seem to work.
    #

    # usdview happily swallows exceptions. It would be really nice if we could
    # put it into a mode where it didn't - were that the case, the following
    # try/except block is all we would need.  Currently it has no value, but
    # we leave it in for future aspirations.
    try:
        _emitShowInterpreter(appController)
    except:
        successfullyBroughtUpInterpreter = False
        assert successfullyBroughtUpInterpreter
    assert appController._console

    # So instead, we check the last context variable that the interpreter
    # initializes. This is pretty brittle, but other options seem just as 
    # brittle,
    assert "usdviewApi" in appController._console.locals()

def _testInterpreterHandlesSyntaxError(appController):
    #
    # Simulate entering some invalid Python code into the interpreter and
    # verify that the exception handling worked correctly by checking that
    # the console text includes a report of the SyntaxError.
    #

    import pxr.Usdviewq.qt
    if pxr.Usdviewq.qt.PySideModule == 'PySide2':
        from PySide2 import QtTest
    elif pxr.Usdviewq.qt.PySideModule == 'PySide6':
        from PySide6 import QtTest
    else:
        raise ImportError('PySide QtTest module not available for import')

    controller = appController._console._controller

    QtTest.QTest.keyClicks(controller.textEdit, 'A Syntax Error')
    QtTest.QTest.keyClick(controller.textEdit, QtCore.Qt.Key.Key_Return)

    console_text = controller.textEdit.toPlainText()
    assert 'A Syntax Error' in console_text
    assert 'SyntaxError: invalid syntax' in console_text

def testUsdviewInputFunction(appController):
    _testInterpreterWorks(appController)
    _testInterpreterHandlesSyntaxError(appController)
