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

def _assertSelectedProp(mainWindow, propName):
    selected = mainWindow._ui.propertyView.selectedItems()
    assert len(selected) == 1
    # 1 indicates the first column where we store prop names
    assert propName == selected[0].text(1), propName + '!=' + selected[0].text(1)

def _selectPrim(mainWindow, primName):
    mainWindow._ui.primViewLineEdit.setText(primName)
    mainWindow._primViewFindNext()

def _search(mainWindow, searchTerm, expectedItems):
    # Repainting isn't necessary at all here, its left in as
    # a visual aid for anyone running this test manually
    propSearch = mainWindow._ui.attrViewLineEdit
    propSearch.setText(searchTerm)
    mainWindow.repaint()

    for item in expectedItems:
        mainWindow._attrViewFindNext()
        mainWindow.repaint()
        _assertSelectedProp(mainWindow, item)

def _testSearchBasic(mainWindow):
    intPropName = 'a'
    relPropName = 'myRel' 
    stringPropName = 'z'
    floatPropName = 'y'

    _search(mainWindow, 'a', [intPropName, 'Local to World Xform'])
    _search(mainWindow, 'myR', [relPropName])
    _search(mainWindow, 'y', [floatPropName, relPropName])
    _search(mainWindow, 'z', [stringPropName])

def testUsdviewInputFunction(mainWindow):
    # Select a prim under which all of our props are authored
    _selectPrim(mainWindow, 'f')
    _testSearchBasic(mainWindow)
