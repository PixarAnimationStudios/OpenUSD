#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

from pxr import Sdf

def _setPaths(appController, pathStrs):
    pathWidget = appController._ui.currentPathWidget
    pathWidget.setText(pathStrs)
    appController._currentPathChanged()

    # We dont strictly need to repaint here, but its makes
    # what is happening clearer to anyone obeserving this test
    appController._mainWindow.repaint()

def _getPath(appController):
    return str(appController._ui.currentPathWidget.text())

def _assertPathsEq(appController, pathStr):
    widgetText = _getPath(appController)
    assert widgetText == pathStr
    for (actual, expected) in zip(list(map(str.strip, widgetText.split(','))), 
                                  list(map(str.strip, pathStr.split(',')))):
        assert Sdf.Path(actual)
        assert actual == expected

def _assertPathIsPrim(appController):
    for p in _getPath(appController).split(','):
        assert Sdf.Path(p).IsAbsoluteRootOrPrimPath()

def _assertPathIsProp(appController):
    for p in _getPath(appController).split(','):
        assert Sdf.Path(_getPath(appController)).IsPropertyPath()

def _assertSelectedPrims(appController, primNames):
    primView = appController._ui.primView
    paths = list(map(str.strip, primNames.split(',')))
    selected = primView.selectedItems()

    assert len(selected) == len(paths)
    for index, p in enumerate(paths):
        # 0 indicates the first column where we store prim names
        assert p == selected[index].text(0)

def _assertSelectedProp(appController, propName):
    propView = appController._ui.propertyView
    selected = propView.selectedItems()

    # Since these tests are exercising the prim bar,
    # we'll only ever get 1 prop path at most
    assert len(selected) == 1

    # 1 indicates the second column where we store prop names
    assert propName == selected[0].text(1)

def _testGarbageInput(appController):
    # Ensure that sending invalid text gets reset to the 
    # previous path, which is the pseudoroot
    _setPaths(appController, 'xxx')
    _assertPathsEq(appController, '/')

    _setPaths(appController, '/f, aisdhioasj')
    _assertPathsEq(appController, '/')

def _testRootPath(appController):
    _setPaths(appController, '/')
    _assertPathsEq(appController, '/')
    _assertPathIsPrim(appController)
    _assertSelectedPrims(appController, 'root')

def _testExistingPath(appController):
    _setPaths(appController, '/f')
    _assertPathsEq(appController, '/f')
    _assertSelectedPrims(appController, 'f')

    _setPaths(appController, '/f.x')
    _assertPathsEq(appController, '/f')
    _assertSelectedPrims(appController, 'f')
    _assertSelectedProp(appController, 'x')

def _testMixedPaths(appController):
    # test all valid paths
    _setPaths(appController, '/f.x, /g')
    _assertPathsEq(appController, '/f, /g')
    _assertSelectedPrims(appController, 'f, g')

    _setPaths(appController, '/f, /g')
    _assertPathsEq(appController, '/f, /g')
    _assertSelectedPrims(appController, 'f, g')

    # test all invalid paths
    _setPaths(appController, '/x, /y')
    _assertPathsEq(appController, '/f, /g')

    # test some valid/some invalid
    _setPaths(appController, '/f, /x')
    _assertPathsEq(appController, '/f, /g')

# Test that the prim path bar works properly in usdview
def testUsdviewInputFunction(appController):
    # test entering single paths
    _testGarbageInput(appController)
    _testRootPath(appController)
    _testExistingPath(appController)
    _testMixedPaths(appController)
