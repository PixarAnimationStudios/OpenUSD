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
    # by default the prim display name option is on
    # so since neither 'f' nor 'foo' have authored
    # display names, it will search the prim name
    _search(appController, 'f', ['f', 'foo'])

    # with the prim display name option on, search 
    # is only searching the display name value
    # so a search that is targeting the prim name
    # won't produce the prim name as a result
    _search(appController, 'g', [])

    # On an invalid search, the results will be
    # equivalent to what was previously there (which in
    # this case is nothing...)
    _search(appController, 'xxx', [])

    # Do a regex based search - again, this is
    # looking at prim names here not display names
    # since they aren't authored
    _search(appController, 'f.*', ['f', 'foo'])

    # search based on display name
    _search(appController, 'test', ['testDisplayName'])

def _testSearchNoPrimDisplayName(appController):
    """
    Performs a test of the search capability of usdview
    without the default "Show Prim Display Names" on.
    Without this option on, search will only look at
    prim identifers and will not consider the display
    name metadata.
    """
    # this searches specifically for a known display name
    # which should fail - the rest of the searches
    # should only look at prim names until the option is turned back on
    appController._dataModel.viewSettings.showPrimDisplayNames = False
    _search(appController, 'test', [])

    # the block of _search calls only look at prim names
    # not display names
    _search(appController, 'f', ['f', 'foo'])
    _search(appController, 'g', ['g'])

    # On an invalid search, the old term will remain
    _search(appController, 'xxx', ['g'])

    # Do a regex based search
    _search(appController, 'f.*', ['f', 'foo'])

    appController._dataModel.viewSettings.showPrimDisplayNames = True
    _search(appController, 'test', ['testDisplayName'])

def testUsdviewInputFunction(appController):
    _testSearchBasic(appController)

    # test with prim display name off
    _testSearchNoPrimDisplayName(appController)