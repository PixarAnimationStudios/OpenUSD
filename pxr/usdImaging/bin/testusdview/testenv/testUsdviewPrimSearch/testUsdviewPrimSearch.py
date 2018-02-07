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

def _assertSelectedPrim(appController, primName):
    selected = appController._ui.primView.selectedItems()
    assert len(selected) == 1
    # 0 indicates the first column where we store prim names
    assert primName == selected[0].text(0)

def _search(appController, searchTerm, expectedItems):
    # Repainting isn't necessary at all here, its left in as
    # a visual aid for anyone running this test manually
    primSearch = appController._ui.primViewLineEdit
    primSearch.setText(searchTerm)
    appController._mainWindow.repaint()

    for item in expectedItems:
        appController._primViewFindNext()
        appController._mainWindow.repaint()
        _assertSelectedPrim(appController, item)

    # Looping over again will currently cycle through the elements again
    for item in expectedItems:
        appController._primViewFindNext()
        appController._mainWindow.repaint()
        _assertSelectedPrim(appController, item)

def _testSearchBasic(appController):
    _search(appController, 'f', ['f', 'foo'])
    _search(appController, 'g', ['g'])

    # On an invalid search, the old term will remain
    _search(appController, 'xxx', ['g'])

    # Do a regex based search
    _search(appController, 'f.*', ['f', 'foo'])

def testUsdviewInputFunction(appController):
    _testSearchBasic(appController)
