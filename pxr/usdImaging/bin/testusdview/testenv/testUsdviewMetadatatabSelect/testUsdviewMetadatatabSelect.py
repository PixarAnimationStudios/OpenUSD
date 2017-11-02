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

def _testSelectionChangeScrollPosition(mainWindow):
    # A test to ensure changing the selection in the metadata
    # tab view does not affect the current scroll position in 
    # the property view
    propView = mainWindow._ui.propertyView
    inspectorView = mainWindow._ui.attributeInspector

    initialScroll = propView.verticalScrollBar().value()

    inspectorView.setCurrentIndex(2)
    mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(3)
    mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(2)
    mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(1)
    mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

    inspectorView.setCurrentIndex(0)
    mainWindow.repaint()
    assert propView.verticalScrollBar().value() == initialScroll 

def _testBasic(mainWindow):
    inspectorView = mainWindow._ui.attributeInspector
    
    inspectorView.setCurrentIndex(0)
    mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Value'
 
    inspectorView.setCurrentIndex(1)
    mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Meta Data'
 
    inspectorView.setCurrentIndex(2)
    mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Layer Stack'
 
    inspectorView.setCurrentIndex(3)
    mainWindow.repaint()
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Composition'
 
def testUsdviewInputFunction(mainWindow):
    # select our initial elements(prim and property).
    mainWindow._ui.primViewLineEdit.setText('Implicits') 
    mainWindow._primViewFindNext()
    mainWindow.repaint()

    mainWindow._ui.attrViewLineEdit.setText('z')
    mainWindow._attrViewFindNext()
    mainWindow._ui.attrViewLineEdit.setText('y')
    mainWindow._attrViewFindNext()
    mainWindow.repaint()

    _testBasic(mainWindow)
    _testSelectionChangeScrollPosition(mainWindow)      
