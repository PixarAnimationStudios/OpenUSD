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

from pxr.Usdviewq.qt import QtWidgets

# positions and names of our variants
CAPSULE = (1, 'capsule')
CONE = (2, 'cone')
CUBE = (3, 'cube')
CYLINDER = (4, 'cylinder')

VARIANT_INFO_POS = 0
VARIANT_INFO_NAME = 1

# Identifiers for variants in our stage
FIRST_VARIANT = 'a_shapeVariant'
SECOND_VARIANT = 'b_shapeVariant'

def _makeFileName(variantInfo, index):
    return variantInfo[1] + str(index) + '.png'

def _modifySettings(mainWindow):
    mainWindow.showBBoxes = False
    mainWindow.showHUD = False

def _setupWidgets(mainWindow):
    # Select our prim with the variant authored
    mainWindow._ui.primViewLineEdit.setText('Shapes')
    mainWindow._primViewFindNext()
    mainWindow.repaint()

def _getVariantSelector(mainWindow, whichVariant):
    # Select the metadata tab in the lower right corner
    attributeInspector = mainWindow._ui.attributeInspector
    attributeInspector.setCurrentIndex(1)

    # Grab the rows of our metadata tab and select the set containing
    # our variant selection
    metadataTable = attributeInspector.currentWidget().findChildren(QtWidgets.QTableWidget)[0]

    for i in range(0, metadataTable.rowCount()):
        currentName = metadataTable.item(i,0).text()
        if str(currentName).startswith(whichVariant):
            return metadataTable.cellWidget(i,1) 

    return None

def _takeShot(mainWindow, fileName):
    mainWindow.repaint()
    mainWindow._stageView.updateGL()
    viewportShot = mainWindow.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

def _selectVariant(mainWindow, variantPos, whichVariant):
    selector = _getVariantSelector(mainWindow, whichVariant)
    selector.setCurrentIndex(variantPos)
    mainWindow.repaint()

def _testBasic(mainWindow):
    # select items from the first variant

    # select capsule
    _selectVariant(mainWindow, CAPSULE[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(mainWindow, _makeFileName(CAPSULE, 1))

    # select cone
    _selectVariant(mainWindow, CONE[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(mainWindow, _makeFileName(CONE, 1))

    # select cube
    _selectVariant(mainWindow, CUBE[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(mainWindow, _makeFileName(CUBE, 1))

    # select cylinder 
    _selectVariant(mainWindow, CYLINDER[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(mainWindow, _makeFileName(CYLINDER, 1))

    # select items from the second variant

    # select capsule
    _selectVariant(mainWindow, CAPSULE[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(mainWindow, _makeFileName(CAPSULE, 2))

    # select cone
    _selectVariant(mainWindow, CONE[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(mainWindow, _makeFileName(CONE, 2))

    # select cube
    _selectVariant(mainWindow, CUBE[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(mainWindow, _makeFileName(CUBE, 2))

    # select cylinder 
    _selectVariant(mainWindow, CYLINDER[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(mainWindow, _makeFileName(CYLINDER, 2))

def testUsdviewInputFunction(mainWindow):
    _modifySettings(mainWindow)
    _setupWidgets(mainWindow)
    
    _testBasic(mainWindow)
