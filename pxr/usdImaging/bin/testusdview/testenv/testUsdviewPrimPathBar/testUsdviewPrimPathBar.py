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

from pxr import Sdf

def _setPaths(mainWindow, pathStrs):
    pathWidget = mainWindow._ui.currentPathWidget
    pathWidget.setText(pathStrs)
    mainWindow._currentPathChanged()

    # We dont strictly need to repaint here, but its makes
    # what is happening clearer to anyone obeserving this test
    mainWindow.repaint()

def _getPath(mainWindow):
    return str(mainWindow._ui.currentPathWidget.text())

def _assertPathsEq(mainWindow, pathStr):
    widgetText = _getPath(mainWindow)
    assert widgetText == pathStr
    for (actual, expected) in zip(map(str.strip, widgetText.split(',')), 
                                  map(str.strip, pathStr.split(','))):
        assert Sdf.Path(actual)
        assert actual == expected

def _assertPathIsPrim(mainWindow):
    for p in _getPath(mainWindow).split(','):
        assert Sdf.Path(p).IsAbsoluteRootOrPrimPath()

def _assertPathIsProp(mainWindow):
    for p in _getPath(mainWindow).split(','):
        assert Sdf.Path(_getPath(mainWindow)).IsPropertyPath()

def _assertSelectedPrims(mainWindow, primNames):
    primView = mainWindow._ui.primView
    paths = map(str.strip, primNames.split(','))
    selected = primView.selectedItems()

    assert len(selected) == len(paths)
    for index, p in enumerate(paths):
        # 0 indicates the first column where we store prim names
        assert p == selected[index].text(0)

def _assertSelectedProp(mainWindow, propName):
    propView = mainWindow._ui.propertyView
    selected = propView.selectedItems()

    # Since these tests are exercising the prim bar,
    # we'll only ever get 1 prop path at most
    assert len(selected) == 1

    # 1 indicates the second column where we store prop names
    assert propName == selected[0].text(1)

def _testGarbageInput(mainWindow):
    # Ensure that sending invalid text gets reset to the 
    # previous path, which is the pseudoroot
    _setPaths(mainWindow, 'xxx')
    _assertPathsEq(mainWindow, '/')

    _setPaths(mainWindow, '/f, aisdhioasj')
    _assertPathsEq(mainWindow, '/')

def _testRootPath(mainWindow):
    _setPaths(mainWindow, '/')
    _assertPathsEq(mainWindow, '/')
    _assertPathIsPrim(mainWindow)
    _assertSelectedPrims(mainWindow, 'root')

def _testExistingPath(mainWindow):
    _setPaths(mainWindow, '/f')
    _assertPathsEq(mainWindow, '/f')
    _assertSelectedPrims(mainWindow, 'f')

    _setPaths(mainWindow, '/f.x')
    _assertPathsEq(mainWindow, '/f')
    _assertSelectedPrims(mainWindow, 'f')
    _assertSelectedProp(mainWindow, 'x')

def _testMixedPaths(mainWindow):
    # test all valid paths
    _setPaths(mainWindow, '/f.x, /g')
    _assertPathsEq(mainWindow, '/f, /g')
    _assertSelectedPrims(mainWindow, 'f, g')

    _setPaths(mainWindow, '/f, /g')
    _assertPathsEq(mainWindow, '/f, /g')
    _assertSelectedPrims(mainWindow, 'f, g')

    # test all invalid paths
    _setPaths(mainWindow, '/x, /y')
    _assertPathsEq(mainWindow, '/f, /g')

    # test some valid/some invalid
    _setPaths(mainWindow, '/f, /x')
    _assertPathsEq(mainWindow, '/f, /g')

# Test that the prim path bar works properly in usdview
def testUsdviewInputFunction(mainWindow):
    # test entering single paths
    _testGarbageInput(mainWindow)
    _testRootPath(mainWindow)
    _testExistingPath(mainWindow)
    _testMixedPaths(mainWindow)
