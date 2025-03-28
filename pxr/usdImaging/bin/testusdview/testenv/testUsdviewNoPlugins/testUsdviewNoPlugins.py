#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

def _testBasic(appController):
    # Ensure that no plugin menus are loaded.
    menuItems = [a for a in appController._mainWindow.menuBar().actions()
                 if not a.isSeparator()]
    expectedItems = [u'&File', u'&Edit', u'Window']
    actualItems = [p.text() for p in menuItems]
    failureStr = str(expectedItems) + ' != ' + str(actualItems)
    assert actualItems == expectedItems, failureStr

def testUsdviewInputFunction(appController):
    _testBasic(appController)
