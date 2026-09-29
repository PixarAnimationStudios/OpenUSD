#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def _assertSelectedPrim(appController, primName):
    selected = appController._ui.primView.selectedItems()
    assert len(selected) == 1
    # 0 indicates the first column where we store prim names
    assert primName == selected[0].text(0),\
        f"Expected {primName} but got {selected[0].text(0)}"

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
    # won't produce the prim name as a result, so
    # we'll find neither prims "g" nor "oldEncoding"
    # (unless we were dropping backwards compatibility)
    _search(appController, 'g', [])

    # On an invalid search, the result will be the last
    # "found" prim (which in this case is nothing...)
    _search(appController, 'xxx', [])

    # Do a regex based search that matches both
    # an unadorned prim AND one with a displayName
    _search(appController, '.*o.*', ['testBackCompat', 'foo'])

    # search based on display name
    _search(appController, 'test', ['testDisplayName', 'testBackCompat'])

def _testSearchNoPrimDisplayName(appController):
    """
    Performs a test of the search capability of usdview
    without the default "Show Prim Display Names" on.
    Without this option on, search will only look at
    prim identifiers and will not consider the display
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
    _search(appController, 'g', ['g', 'oldEncoding'])

    # On an invalid search, the result will be the last
    # "found" prim
    _search(appController, 'xxx', ['oldEncoding'])

    # Do a regex based search. `foo` will come up first because `oldEncoding`
    # is selected at time of search.
    _search(appController, '.*o.*', ['foo', 'oldEncoding'])

    appController._dataModel.viewSettings.showPrimDisplayNames = True
    _search(appController, 'test', ['testDisplayName', 'testBackCompat'])

def testUsdviewInputFunction(appController):
    _testSearchBasic(appController)

    # test with prim display name off
    _testSearchNoPrimDisplayName(appController)
