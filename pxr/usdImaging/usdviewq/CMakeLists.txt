set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_BUILD_USDVIEW)
    return()
endif()

pxr_library(usdviewq
    LIBRARIES
        tf
        usd
        usdGeom
        hd
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        hydraObserver
        utils

    PYTHON_CPPFILES
        moduleDeps.cpp
        
    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        module.cpp
        wrapHydraObserver.cpp
        wrapUtils.cpp

    PYMODULE_FILES
        __init__.py
        appEventFilter.py
        arrayAttributeView.py
        customAttributes.py
        frameSlider.py
        appController.py
        usdviewApi.py
        plugin.py
        scalarTypes.py
        stageView.py
        usdviewContextMenuItem.py
        primContextMenu.py
        primContextMenuItems.py
        headerContextMenu.py
        attributeViewContextMenu.py
        layerStackContextMenu.py
        prettyPrint.py
        pythonInterpreter.py
        settings.py
        adjustDefaultMaterial.py
        adjustFreeCamera.py
        preferences.py
        primViewItem.py
        common.py
        hydraSceneBrowser.py
        legendUtil.py
        primLegend.py
        primTreeWidget.py
        propertyLegend.py
        attributeValueEditor.py
        overridableLineEdit.py
        variantComboBox.py
        qt.py
        rootDataModel.py
        selectionDataModel.py
        viewSettingsDataModel.py
        freeCamera.py
        debugFlagsWidget.py
        configController.py
        usdviewstyle.qss

    PYSIDE_UI_FILES
        mainWindowUI.ui
        adjustDefaultMaterialUI.ui
        adjustFreeCameraUI.ui
        attributeValueEditorUI.ui
        preferencesUI.ui
        primLegendUI.ui
        propertyLegendUI.ui

    DOXYGEN_FILES
        README.md
        blackBoxTesting.md
)

install(DIRECTORY
    icons
    DESTINATION lib/python/pxr/Usdviewq
)

install(DIRECTORY
    fonts
    DESTINATION lib/python/pxr/Usdviewq
)

pxr_test_scripts(
    testenv/testUsdviewqSettings.py
    testenv/testUsdviewqRootDataModel.py
    testenv/testUsdviewqSelectionDataModel.py
    testenv/testUsdviewqLauncher.py
    testenv/testUsdviewqViewSettingsDataModel.py
)

pxr_install_test_dir(
    SRC testenv/testUsdviewqRootDataModel
    DEST testUsdviewqRootDataModel
)

pxr_register_test(testUsdviewqSettings
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqSettings"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqRootDataModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqRootDataModel"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqSelectionDataModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqSelectionDataModel"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqLauncher
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqLauncher"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqViewSettingsDataModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqViewSettingsDataModel"
    EXPECTED_RETURN_CODE 0
)

