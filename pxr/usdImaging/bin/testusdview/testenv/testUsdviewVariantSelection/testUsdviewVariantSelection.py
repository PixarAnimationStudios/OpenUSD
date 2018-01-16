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

def _modifySettings(appController):
    appController._dataModel.viewSettings.showBBoxes = False
    appController._dataModel.viewSettings.showHUD = False

def _setupWidgets(appController):
    # Select our prim with the variant authored
    appController._ui.primViewLineEdit.setText('Shapes')
    appController._primViewFindNext()
    appController._mainWindow.repaint()

def _getVariantSelector(appController, whichVariant):
    # Select the metadata tab in the lower right corner
    attributeInspector = appController._ui.attributeInspector
    attributeInspector.setCurrentIndex(1)

    # Grab the rows of our metadata tab and select the set containing
    # our variant selection
    metadataTable = attributeInspector.currentWidget().findChildren(QtWidgets.QTableWidget)[0]

    for i in range(0, metadataTable.rowCount()):
        currentName = metadataTable.item(i,0).text()
        if str(currentName).startswith(whichVariant):
            return metadataTable.cellWidget(i,1) 

    return None

def _takeShot(appController, fileName):
    appController._mainWindow.repaint()
    appController._stageView.updateGL()
    viewportShot = appController.GrabViewportShot()
    viewportShot.save(fileName, "PNG")

def _selectVariant(appController, variantPos, whichVariant):
    selector = _getVariantSelector(appController, whichVariant)
    selector.setCurrentIndex(variantPos)
    appController._mainWindow.repaint()

def _testBasic(appController):
    # select items from the first variant

    # select capsule
    _selectVariant(appController, CAPSULE[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(appController, _makeFileName(CAPSULE, 1))

    # select cone
    _selectVariant(appController, CONE[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(appController, _makeFileName(CONE, 1))

    # select cube
    _selectVariant(appController, CUBE[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(appController, _makeFileName(CUBE, 1))

    # select cylinder 
    _selectVariant(appController, CYLINDER[VARIANT_INFO_POS], FIRST_VARIANT)
    _takeShot(appController, _makeFileName(CYLINDER, 1))

    # select items from the second variant

    # select capsule
    _selectVariant(appController, CAPSULE[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(appController, _makeFileName(CAPSULE, 2))

    # select cone
    _selectVariant(appController, CONE[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(appController, _makeFileName(CONE, 2))

    # select cube
    _selectVariant(appController, CUBE[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(appController, _makeFileName(CUBE, 2))

    # select cylinder 
    _selectVariant(appController, CYLINDER[VARIANT_INFO_POS], SECOND_VARIANT)
    _takeShot(appController, _makeFileName(CYLINDER, 2))

def testUsdviewInputFunction(appController):
    _modifySettings(appController)
    _setupWidgets(appController)
    
    _testBasic(appController)
