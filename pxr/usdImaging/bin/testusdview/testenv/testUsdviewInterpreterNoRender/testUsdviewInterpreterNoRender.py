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

def _emitShowInterpreter(appController):
    appController._ui.showInterpreter.triggered.emit() 
    QtWidgets.QApplication.processEvents()

def _postKeyPress(key, widget):
    event = QtGui.QKeyEvent(QtCore.QEvent.KeyPress,
                            key,
                            QtCore.Qt.NoModifier)
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
    assert 'layer' in appController._console.locals()
    

def testUsdviewInputFunction(appController):
    _testInterpreterWorks(appController)
