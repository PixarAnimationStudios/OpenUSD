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
    for (actual, expected) in zip(map(str.strip, widgetText.split(',')), 
                                  map(str.strip, pathStr.split(','))):
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
    paths = map(str.strip, primNames.split(','))
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
