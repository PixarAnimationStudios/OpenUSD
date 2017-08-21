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
from PySide import QtGui, QtCore
from referenceEditorUI import Ui_ReferenceEditor

import mainWindow

# XXX USD REFERENCE EDITOR DISABLED
# Dialog window to edit reference asset path, reference path and offset/scale

class ReferenceEditor(QtGui.QDialog):
    def __init__(self, parent, stage, node):
        QtGui.QDialog.__init__(self, parent)
        self._ui = Ui_ReferenceEditor()
        self._ui.setupUi(self)

        self._stage = stage
        self._node = node
        self._references = stage.GetReferences(node.GetPath())
        if node.GetPath() == '':    # skip the writable reference at root
            self._references = self._references[1:]

        # when users change the drop-down menu, update the fields
        QtCore.QObject.connect(self._ui.multirefList,
                               QtCore.SIGNAL('currentIndexChanged(int)'),
                               self._multirefChanged)

        self.populateMultirefList()
        
    def populateMultirefList(self):
        # populate the drop-down menu of multi refs
        for ref in self._references:
            self._ui.multirefList.addItem(ref.GetAssetPath(UNVAR).Get())

        self._ui.multirefList.setCurrentIndex(0)

    def _multirefChanged(self, index):
        #populate the form after the user selects a new reference from the multiref list
        self._ref = self._references[index]

        # get the ref src stage file, check if it is writable
        self._refSrc = self._ref.GetRefSrcScene().GetUsdFile()
        self._writable, msg = mainWindow.isWritableUsdPath(self._refSrc)

        # disable the form (prevents editing) if not writable, also show explanation msg
        self._ui.groupBox.setEnabled(self._writable)
        if not self._writable:
            self._ui.errorLabel.setText('The stage this reference is authored in '
                                        '(%s) cannot be edited: ' %self._refSrc + msg)
        else:
            self._ui.errorLabel.setText('')

        # get asset path
        self._assetPath = self._ref.GetAssetPath(UNVAR)
        self._ui.usdAssetPathInput.setText(self._assetPath.Get())

        # get reference path
        self._referencePath = self._ref.GetReferencePath()
        self._ui.referencePathInput.setText(self._referencePath)

        # get offset and scale
        self._offsetAndScale = self._ref.GetTimeOffsetAndScale(UNVAR)
        self._ui.offsetInput.setValue(self._offsetAndScale[0])
        self._ui.scaleInput.setValue(self._offsetAndScale[1])

    def accept(self):
        # when the user clicks "Save"
        if self._writable:  # dont save if not writable!
            # generate a backup directory, offer a backup option to the user
            backupfile = mainWindow.getBackupFile(self._refSrc)
            question = QtGui.QMessageBox("Confirm Reference Edit",
                "Your changes will be permanently committed to the reference "
                "source stage %s\n\n"\
                "Do you want to save a backup of that stage at %s "\
                "before continuing?" %(self._refSrc, backupfile),
                QtGui.QMessageBox.Question,
                QtGui.QMessageBox.Yes,
                QtGui.QMessageBox.No,
                QtGui.QMessageBox.Cancel)
            
            yesButton = question.button(QtGui.QMessageBox.Yes)
            noButton =  question.button(QtGui.QMessageBox.No)
            cancelButton = question.button(QtGui.QMessageBox.Cancel)

            yesButton.setText("Save With Backup")
            noButton.setText("Save Without Backup")
            question.setDefaultButton(yesButton)
            question.setEscapeButton(cancelButton)

            question.exec_()
            reply = question.clickedButton()

            if reply == cancelButton:
                # cancel the Save, return to reference editor
                return

            if reply == yesButton:
                # make a backup of the ref src stage
                from shutil import copyfile
                copyfile(self._refSrc, backupfile)
               
            # open ref src stage
            refSrcScene = Scene.New(self._refSrc, self._refSrc, Scene.WRITE)
            # translate the node path in the ref src stage's space
            nodePathInSrcScene = self._ref.GetRefSrcScene()\
                                .TranslateFullToNativePath(self._node.GetPath())
            refSrcScene.ReadShallow() # yay! readshallow saves time!
            refNodes = refSrcScene.GetReferences(nodePathInSrcScene)

            # search for the reference to edit in the ref src stage
            refNode = None
            for n in refNodes:
                if n.GetAssetPath(UNVAR).Get() == self._assetPath.Get() and \
                   n.GetReferencePath() == self._referencePath and \
                   n.GetTimeOffsetAndScale(UNVAR) == self._offsetAndScale:
                    refNode = n

            # alert user if the reference could not be found
            if not refNode:
                QtGui.QMessageBox.critical(self, 'Failed To Edit Reference',
                        'The reference with assetPath "%s", reference path "%s", '
                        'and offset/scale "(%f,%f)" could not be found in stage "%s".'
                        %(self._assetPath.Get(), self._referencePath,\
                          self._offsetAndScale[0], self._offsetAndScale[1], \
                          self._refSrc))
                return

            # set asset path
            if self._assetPath.Get() != self._ui.usdAssetPathInput.text():
                self._assetPath.Set(str(self._ui.usdAssetPathInput.text()))
                refNode.SetAssetPath(UNVAR, self._assetPath)

            # set reference path
            if self._referencePath != self._ui.referencePathInput.text():
                refNode.SetReferencePath(str(self._ui.referencePathInput.text()))

            # set offset and scale
            if self._offsetAndScale[0] != self._ui.offsetInput.value() or \
               self._offsetAndScale[1] != self._ui.scaleInput.value():
                refNode.SetTimeOffsetAndScale(UNVAR, \
                        (self._ui.offsetInput.value(), self._ui.scaleInput.value()))

            refSrcScene.Flush()

        QtGui.QDialog.accept(self)

