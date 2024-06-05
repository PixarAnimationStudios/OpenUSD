#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

def _testSelectionChangeScrollPosition(appController):
    # A test to ensure changing the selection in the metadata
    # tab view does not affect the current scroll position in 
    # the property view
    propView = appController._ui.propertyView
    inspectorView = appController._ui.propertyInspector

    initialScroll = propView.verticalScrollBar().value()

    inspectorView.setCurrentIndex(2)
    appController._mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(3)
    appController._mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(2)
    appController._mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(1)
    appController._mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(0)
    appController._mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

def _testBasic(appController):
    inspectorView = appController._ui.propertyInspector
    
    inspectorView.setCurrentIndex(0)
    appController._mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Value'
 
    inspectorView.setCurrentIndex(1)
    appController._mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Meta Data'
 
    inspectorView.setCurrentIndex(2)
    appController._mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Layer Stack'
 
    inspectorView.setCurrentIndex(3)
    appController._mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Composition'
 
def testUsdviewInputFunction(appController):
    # select our initial elements(prim and property).
    appController._ui.primViewLineEdit.setText('Implicits')
    appController._primViewFindNext()
    appController._mainWindow.repaint()

    appController._ui.attrViewLineEdit.setText('z')
    appController._attrViewFindNext()
    appController._ui.attrViewLineEdit.setText('y')
    appController._attrViewFindNext()
    appController._mainWindow.repaint()

    _testBasic(appController)
    _testSelectionChangeScrollPosition(appController)
