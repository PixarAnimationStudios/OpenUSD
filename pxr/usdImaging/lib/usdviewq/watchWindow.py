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
from pyside import QtWidgets, QtGui, QtCore
from watchWindowUI import Ui_WatchWindow
import re
import os

# XXX USD WATCH WINDOW DEACTIVATED

separatorColor = QtGui.QColor(255, 255, 255)
separatorString = "\n==================================\n"

class WatchWindow(QtWidgets.QDialog):
    def __init__(self, parent):
        QtWidgets.QDialog.__init__(self,parent)
        self._ui = Ui_WatchWindow()
        self._ui.setupUi(self)

        self._boxWithFocus = self._ui.unvaryingEdit
        self._prevDualScrollVal = 0

        self._parent = parent
        self._searchString = ""

        self.addAction(self._ui.actionFind)
        self.addAction(self._ui.actionFindNext)
        self.addAction(self._ui.actionFindPrevious)

        QtCore.QObject.connect(self._ui.doneButton, 
                               QtCore.SIGNAL('clicked()'),
                               self.accept)
        QtCore.QObject.connect(self,
                               QtCore.SIGNAL('finished(int)'),
                               self._cleanUpAndClose)
        QtCore.QObject.connect(self._ui.dualScroller,
                               QtCore.SIGNAL('valueChanged(int)'),
                               self._changeBothSliders)
        QtCore.QObject.connect(self._ui.diffButton,
                               QtCore.SIGNAL('clicked()'),
                               self._diff)
        QtCore.QObject.connect(self._ui.actionFind,
                               QtCore.SIGNAL('triggered()'),
                               self._find)
        QtCore.QObject.connect(self._ui.actionFindNext,
                               QtCore.SIGNAL('triggered()'),
                               self._findNext)
        QtCore.QObject.connect(self._ui.actionFindPrevious,
                               QtCore.SIGNAL('triggered()'),
                               self._findPrevious)
        QtCore.QObject.connect(self._ui.varyingEdit,
                               QtCore.SIGNAL('cursorPositionChanged()'),
                               self._varyingCursorChanged)
        QtCore.QObject.connect(self._ui.unvaryingEdit,
                               QtCore.SIGNAL('cursorPositionChanged()'),
                               self._unvaryingCursorChanged)
	# save splitter state
	QtCore.QObject.connect(self._ui.splitter,
                               QtCore.SIGNAL('splitterMoved(int, int)'),
                               self._splitterMoved)

	#create a timer for saving splitter state only when it stops moving
	self._splitterTimer = QtCore.QTimer(self)
	self._splitterTimer.setInterval(500)
        QtCore.QObject.connect(self._splitterTimer, QtCore.SIGNAL('timeout()'),
                               self._saveSplitterState)

	self._resetSettings()

    def _splitterMoved(self, pos, index):
	# reset the timer every time the splitter moves
	# when the splitter stop moving for half a second, save state
	self._splitterTimer.stop()
	self._splitterTimer.start()

    def _saveSplitterState(self):
	self._parent._settings.setAndSave(
		watchWindowSplitter = self._ui.splitter.saveState())
	self._splitterTimer.stop()

    def _varyingCursorChanged(self):
        self._boxWithFocus = self._ui.varyingEdit

    def _unvaryingCursorChanged(self):
        self._boxWithFocus = self._ui.unvaryingEdit

    def _find(self):
        searchString = QtWidgets.QInputDialog.getText(self, "Find", 
            "Enter search string\nUse Ctrl+G to \"Find Next\"\n" + \
            "Use Ctrl+Shift+G to \"Find Previous\"")
        if searchString[1]:
            self._searchString = searchString[0]
            if (not self._boxWithFocus.find(self._searchString)):
                self._boxWithFocus.moveCursor(QtGui.QTextCursor.Start)
                self._boxWithFocus.find(self._searchString) 

    def _findNext(self):
        if (self._searchString == ""):
            return
        if (not self._boxWithFocus.find(self._searchString)):
            self._boxWithFocus.moveCursor(QtGui.QTextCursor.Start)
            self._boxWithFocus.find(self._searchString)

    def _findPrevious(self):
        if (self._searchString == ""):
            return
        if (not self._boxWithFocus.find(self._searchString,
            QtWidgets.QTextDocument.FindBackward)):
            self._boxWithFocus.moveCursor(QtGui.QTextCursor.End)
            self._boxWithFocus.find(self._searchString, 
                                      QtWidgets.QTextDocument.FindBackward)

    def _diff(self):
        import tempfile
        unvarFile = tempfile.NamedTemporaryFile()
        unvarFile.write(str(self._ui.unvaryingEdit.toPlainText()))
        unvarFile.flush()
        varFile = tempfile.NamedTemporaryFile()
        varFile.write(str(self._ui.varyingEdit.toPlainText()))
        varFile.flush()

        os.system("xxdiff %s %s" % (unvarFile.name, varFile.name))
                          
        unvarFile.close()
        varFile.close()

    def _changeBothSliders(self, val):
        delta = val - self._prevDualScrollVal
        self._ui.varyingEdit.verticalScrollBar().setValue(
            self._ui.varyingEdit.verticalScrollBar().value() + delta)
        self._ui.unvaryingEdit.verticalScrollBar().setValue(
            self._ui.unvaryingEdit.verticalScrollBar().value() + delta)
        self._prevDualScrollVal = val

    def appendContents(self, s, color = QtGui.QColor(0,0,0)):
	self._ui.varyingEdit.setTextColor(color)
        self._ui.varyingEdit.append(s)

        self._ui.dualScroller.setMaximum(max(
            self._ui.unvaryingEdit.verticalScrollBar().maximum(),
            self._ui.varyingEdit.verticalScrollBar().maximum()))
        self._ui.dualScroller.setPageStep(min(
            self._ui.unvaryingEdit.verticalScrollBar().pageStep(),
            self._ui.varyingEdit.verticalScrollBar().pageStep()))

        self._boxWithFocus = self._ui.varyingEdit

    def appendUnvaryingContents(self, s, color = QtGui.QColor(0,0,0)):
	self._ui.unvaryingEdit.setTextColor(color)
        self._ui.unvaryingEdit.append(s)

        self._ui.dualScroller.setMaximum(max(
            self._ui.unvaryingEdit.verticalScrollBar().maximum(), 
            self._ui.varyingEdit.verticalScrollBar().maximum()))
        self._ui.dualScroller.setPageStep(min(
            self._ui.unvaryingEdit.verticalScrollBar().pageStep(),
            self._ui.varyingEdit.verticalScrollBar().pageStep()))

        self._boxWithFocus = self._ui.unvaryingEdit

    def appendSeparator(self):
	self.appendContents(separatorString, separatorColor)
	self.appendUnvaryingContents(separatorString, separatorColor)

    def clearContents(self):
        self._ui.varyingEdit.setText("")
        self._ui.unvaryingEdit.setText("")

    def _cleanUpAndClose(self, result):
        self._parent._ui.actionWatch_Window.setChecked(False)

    def _resetSettings(self):
	# restore splitter position
	splitterpos = self._parent._settings.get('watchWindowSplitter')
	if not splitterpos is None:
	    self._ui.splitter.restoreState(splitterpos)

