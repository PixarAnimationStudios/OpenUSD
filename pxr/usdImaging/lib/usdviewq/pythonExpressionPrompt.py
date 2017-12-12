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
from qt import QtCore, QtWidgets
from pythonExpressionPromptUI import Ui_PythonExpressionPrompt
from prettyPrint import prettyPrint

import sys, copy, pythonInterpreter

# XXX USD EDITING DISABLED
# this widget retrieves a python object from the user by presenting a
# python prompt and returning the last value or "_" upon confirmation.
# The widget can be invoked using 'getValueFromPython' which will return
# the value of "_"
class PythonExpressionPrompt(QtWidgets.QDialog):

    def __init__(self, parent, exception = None, val = None):
        QtWidgets.QDialog.__init__(self, parent)
        self.setObjectName("PythonExpressionPrompt")
        self._ui = Ui_PythonExpressionPrompt()
        self._ui.setupUi(self)
        self._mainWindow = parent._mainWindow     # get mainWindow instance
        self._val = val         # this is the object that is currently authored
        self._oldValID = -1     # this is the ID of the current object at "_"

        self._setupConsole()    # get or create a python console widget

        # if a exception was passed as an argument, raise it so it prints.
        if exception is not None:
            print >> sys.stderr, exception

        self._ui.buttonBox.button(QtWidgets.QDialogButtonBox.Apply).clicked.connect(self.accept)

    def _setupConsole(self):
        # get or create an instance of Myconsole
        if Myconsole.instance is None:
            Myconsole.instance = Myconsole(self)

        # setting the parent places the Myconsole widget inside this one
        Myconsole.instance.setParent(self)

        # place the console
        self._console = Myconsole.instance
        self._ui.consoleLayout.addWidget(self._console)
        self._console.setFocus()    # puts the cursor in the interpreter
        self._console.reloadConsole(self._mainWindow, self._val) # reloads locals

        # adjust splitter size
        self._ui.splitter.setSizes((350,150))
        self.updatePreview()    # update the preview box

        # we want to update the preview box whenver the value of "_" changes
        self._console.textChanged.connect(self.updatePreview)

    def updatePreview(self):
        # called when the text in the interpreter changes, but the preview is
        # only updated if the value of "_" has changed
        val = self._console.getLastValue() # value of "_"

        # check if the id of the value at "_" has changed
        if id(val) != self._oldValID:
            self._ui.previewBox.setText(prettyPrint(val))
            self._oldValID = id(val)

    def accept(self):
        # called when clicking the Save button
        # grab the value of "_" and save it in self._val
        self._val = self._console.getLastValue()
        QtWidgets.QDialog.accept(self)

    def reject(self):
        # called when clicking the Close Without Saving button
        self._val = None
        QtWidgets.QDialog.reject(self)

    def exec_(self):
        # called to "execute" the dialog process
        # we override this function to make sure to return I/O to stdin and
        # stdout, and make exec_ return the last value of "_"
        QtWidgets.QDialog.exec_(self)
        return self._val

    def __del__(self):
        # we have to remove the parent from the "Myconsole" instance
        # because the parent (self) is about to be deleted.
        # not doing this will cause crashes.
        Myconsole.instance.setParent(None)

    @staticmethod
    def getValueFromPython(parent, exception = None, val = None):
        # this is the function to call to invoke this python prompt
        # it returns the last value of "_"
        prompt = PythonExpressionPrompt(parent, exception, val)
        val = prompt.exec_()
        return val


interpreterView = pythonInterpreter.View
class Myconsole(interpreterView):
    instance = None

    def __init__(self, parent):
        initialPrompt = ("\nLocal State Variables\n"
                "    plugCtx: a plugin context object\n"
                "    stage: the current Usd.Stage object\n"
                "    frame: the current frame for playback\n"
                "    selectedPrims: a list of all selected prims\n"
                "    selectedInstances: a dictionary of selected prims to selected indices within\n"
                "    prim: the first selected prim in the selectedPrims list\n"
                "    property: the currently selected usd property (if any)\n"
                "    spec: the currently selected sdf spec in the composition tree (if any)\n"
                "    layer: the currently selected sdf layer in the composition tree (if any)\n\n")

        interpreterView.__init__(self, parent)
        self.setObjectName("Myconsole")

        from pxr import Usd, UsdGeom, Gf, Tf
        from qt import QtCore, QtGui, QtWidgets

        # Make a _Controller
        self._controller = pythonInterpreter.Controller(
            self, initialPrompt, vars())

    def interp(self):
        return self._controller

    def locals(self):
        return self._controller.interpreter.locals

    def getLastValue(self):
        # return the current value of "_"
        return self.locals()['__builtins__']['_']

    def reloadConsole(self, appController, val = None):
        # refreshes locals and redirects I/O
        if '__builtins__' in self.locals():
            self.locals()['__builtins__']['_'] = val

        self.locals()['plugCtx'] = appController._plugCtx
        self.locals()['stage'] = appController._stageDataModel.stage
        self.locals()['frame'] = appController._currentFrame
        self.locals()['selectedPrims'] = list(appController._currentPrims)
        self.locals()['selectedInstances'] = appController._stageView._selectedInstances.copy()
        self.locals()['prim'] = appController._currentPrims[0] if \
                                        appController._currentPrims else None
        self.locals()['property'] = appController._currentProp
        self.locals()['spec'] = appController._currentSpec
        self.locals()['layer'] = appController._currentLayer
