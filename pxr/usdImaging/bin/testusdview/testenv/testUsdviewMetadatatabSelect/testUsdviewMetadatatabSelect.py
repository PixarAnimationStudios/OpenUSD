#!/pxrpythonsubst
#
# Copyright 2017 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.

from pxr import Usd, UsdLux
from pxr.UsdUtils.constantsGroup import ConstantsGroup
from pxr.Usdviewq.qt import QtWidgets

class MetadataKeys(ConstantsGroup):
    ROOT_METADATA_KEY = "upAxis"
    APPLIED_API_SCHEMAS_FIELD = "[applied API schemas]"
    AUTHORED_API_SCHEMAS_FIELD = 'apiSchemas'

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

def _testAPISchemaMetadata(appController):
    # Check root metadata is visible
    inspectorView = appController._ui.propertyInspector
    appController.selectPseudoroot()
    inspectorView.setCurrentIndex(1)
    assert inspectorView.tabText(inspectorView.currentIndex()) == 'Meta Data'
    appController._mainWindow.repaint()
    metadataTable = inspectorView.currentWidget().findChildren(
        QtWidgets.QTableWidget)[0]

    foundRootMetadata = False
    for i in range(metadataTable.rowCount()):
        fieldName = str(metadataTable.item(i, 0).text())
        value = str(metadataTable.item(i, 1).text())
        if fieldName == MetadataKeys.ROOT_METADATA_KEY:
            foundRootMetadata = True
            break

    assert foundRootMetadata

    # Check Applied API schemas are set and correct
    inspectorView = appController._ui.propertyInspector
    inspectorView.setCurrentIndex(1)
    appController._ui.primViewLineEdit.setText('Light')
    appController._primViewFindNext()
    appController._updateMetadataView()

    metadataTable = inspectorView.currentWidget().findChildren(
        QtWidgets.QTableWidget)[0]

    reg = Usd.SchemaRegistry()
    primDef = reg.FindConcretePrimDefinition("RectLight")
    originalAppliedSchemas = primDef.GetAppliedAPISchemas()

    for i in range(0, metadataTable.rowCount()):
        fieldName = str(metadataTable.item(i, 0).text())
        value = str(metadataTable.item(i, 1).text())
        if fieldName == MetadataKeys.APPLIED_API_SCHEMAS_FIELD:
            assert all(str(s) in value for s in originalAppliedSchemas)

    prim = appController._dataModel.selection.getFocusPrim()
    UsdLux.MeshLightAPI.Apply(prim)
    appController._updateMetadataView()
    apiDef = reg.FindAppliedAPIPrimDefinition("MeshLightAPI")
    additionalAppliedSchemas = primDef.GetAppliedAPISchemas()

    for i in range(0, metadataTable.rowCount()):
        fieldName = str(metadataTable.item(i, 0).text())
        value = str(metadataTable.item(i, 1).text())
        if fieldName == MetadataKeys.APPLIED_API_SCHEMAS_FIELD:
            assert all(str(s) in value for s in originalAppliedSchemas)
            assert all(str(s) in value for s in additionalAppliedSchemas)
            assert "MeshLightAPI" in value
        elif fieldName == MetadataKeys.AUTHORED_API_SCHEMAS_FIELD:
            assert "MeshLightAPI" in value

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
    _testAPISchemaMetadata(appController)
