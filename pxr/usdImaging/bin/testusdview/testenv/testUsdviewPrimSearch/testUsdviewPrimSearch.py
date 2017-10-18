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

def _assertSelectedPrim(mainWindow, primName):
    selected = mainWindow._ui.nodeView.selectedItems()
    assert len(selected) == 1
    # 0 indicates the first column where we store prim names
    assert primName == selected[0].text(0)

def _search(mainWindow, searchTerm, expectedItems):
    # Repainting isn't necessary at all here, its left in as
    # a visual aid for anyone running this test manually
    primSearch = mainWindow._ui.nodeViewLineEdit
    primSearch.setText(searchTerm)
    mainWindow.repaint()

    for item in expectedItems:
        mainWindow._nodeViewFindNext()
        mainWindow.repaint()
        _assertSelectedPrim(mainWindow, item)

    # Looping over again will currently cycle through the elements again
    for item in expectedItems:
        mainWindow._nodeViewFindNext()
        mainWindow.repaint()
        _assertSelectedPrim(mainWindow, item)

def _testSearchBasic(mainWindow):
    _search(mainWindow, 'f', ['f', 'foo'])
    _search(mainWindow, 'g', ['g'])

    # On an invalid search, the old term will remain
    _search(mainWindow, 'xxx', ['g'])

    # Do a regex based search
    _search(mainWindow, 'f.*', ['f', 'foo'])

def testUsdviewInputFunction(mainWindow):
    _testSearchBasic(mainWindow)
