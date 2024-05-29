#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def _assertSelectedProp(appController, propName):
    selected = appController._dataModel.selection.getProps()
    selectedComputed = appController._dataModel.selection.getComputedProps()

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
    _search(appController, 'a', 
            ['Local to World Xform', 'Resolved Preview Material',
             'Resolved Full Material', 'a'])
    _search(appController, 'myR', ['myRel'])
    _search(appController, 'y', 
            ['myRel', 'proxyPrim', 'visibility', 'y'])
    _search(appController, 'z', ['z'])

def testUsdviewInputFunction(appController):
    # Select a prim under which all of our props are authored
    _selectPrim(appController, 'f')
    _testSearchBasic(appController)
