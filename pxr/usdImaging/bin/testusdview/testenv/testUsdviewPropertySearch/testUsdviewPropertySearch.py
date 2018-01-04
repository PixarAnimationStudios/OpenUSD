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

def _assertSelectedProp(appController, propName):
    selected = appController._selectionDataModel.getProps()
    selectedComputed = appController._selectionDataModel.getComputedProps()

    if len(selected) == 1:
        assert len(selectedComputed) == 0
        selectedPropName = selected[0].GetName()
    else:
        assert len(selectedComputed) == 1
        selectedPropName = selectedComputed[0].GetName()
    
    assert propName == selectedPropName, propName + '!=' + selectedPropName

def _selectPrim(appController, primName):
    appController._ui.primViewLineEdit.setText(primName)
    appController._primViewFindNext()

def _search(appController, searchTerm, expectedItems):
    # Repainting isn't necessary at all here, its left in as
    # a visual aid for anyone running this test manually
    propSearch = appController._ui.attrViewLineEdit
    propSearch.setText(searchTerm)
    appController._mainWindow.repaint()

    for item in expectedItems:
        appController._attrViewFindNext()
        appController._mainWindow.repaint()
        _assertSelectedProp(appController, item)

def _testSearchBasic(appController):
    intPropName = 'a'
    relPropName = 'myRel' 
    stringPropName = 'z'
    floatPropName = 'y'

    _search(appController, 'a', [intPropName, 'Local to World Xform'])
    _search(appController, 'myR', [relPropName])
    _search(appController, 'y', [floatPropName, relPropName])
    _search(appController, 'z', [stringPropName])

def testUsdviewInputFunction(appController):
    # Select a prim under which all of our props are authored
    _selectPrim(appController, 'f')
    _testSearchBasic(appController)
