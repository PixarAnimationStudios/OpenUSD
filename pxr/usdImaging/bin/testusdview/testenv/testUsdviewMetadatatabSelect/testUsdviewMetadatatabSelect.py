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
