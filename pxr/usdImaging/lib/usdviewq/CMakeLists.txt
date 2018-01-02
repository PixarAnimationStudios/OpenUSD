set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_ENABLE_PYTHON_SUPPORT)    
    message(STATUS "Not building ${PXR_PACKAGE} because PXR_ENABLE_PYTHON_SUPPORT=OFF")   
    return()
elseif (NOT PYSIDE_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: PySide")
    return()
elseif(NOT PYOPENGL_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: PyOpenGL")
    return()
endif()

pxr_library(usdviewq
    LIBRARIES
        tf
        usd
        usdGeom
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        utils

    PYTHON_CPPFILES
        moduleDeps.cpp
        
    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        module.cpp
        wrapUtils.cpp

    PYMODULE_FILES
        __init__.py
        appEventFilter.py
        arrayAttributeView.py
        customAttributes.py
        appController.py
        plugContext.py
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
        settings2.py
        adjustClipping.py
        adjustDefaultMaterial.py
        primViewItem.py
        common.py
        legendUtil.py
        primLegend.py
        propertyLegend.py
        attributeValueEditor.py
        pythonExpressionPrompt.py
        overridableLineEdit.py
        variantComboBox.py
        qt.py
        constantGroup.py
        rootDataModel.py
        usdviewstyle.qss

    PYSIDE_UI_FILES
        mainWindowUI.ui
        adjustClippingUI.ui
        adjustDefaultMaterialUI.ui
        attributeValueEditorUI.ui
        primLegendUI.ui
        propertyLegendUI.ui
        pythonExpressionPromptUI.ui
)

install(DIRECTORY
    icons
    DESTINATION lib/python/pxr/Usdviewq
)

pxr_test_scripts(
    testenv/testUsdviewqConstantGroup.py
    testenv/testUsdviewqSettings2.py
    testenv/testUsdviewqRootDataModel.py
    testenv/testUsdviewqPlugContextProxy.py
    testenv/testUsdviewqRootDataModel.py
)

pxr_install_test_dir(
    SRC testenv/testUsdviewqRootDataModel
    DEST testUsdviewqRootDataModel
)

pxr_register_test(testUsdviewqConstantGroup
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqConstantGroup"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqSettings2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqSettings2"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqRootDataModel
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqRootDataModel"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewqPlugContextProxy
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdviewqPlugContextProxy"
    EXPECTED_RETURN_CODE 0
)
