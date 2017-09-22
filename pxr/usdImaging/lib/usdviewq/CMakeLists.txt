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
        arrayAttributeView.py
        customAttributes.py
        mainWindow.py
        plugContext.py
        scalarTypes.py
        stageView.py
        usdviewContextMenuItem.py
        nodeContextMenu.py
        nodeContextMenuItems.py
        headerContextMenu.py
        attributeViewContextMenu.py
        layerStackContextMenu.py
        prettyPrint.py
        pythonInterpreter.py
        settings.py
        watchWindow.py
        adjustClipping.py
        adjustDefaultMaterial.py
        nodeViewItem.py
        common.py
        attributeValueEditor.py
        pythonExpressionPrompt.py
        overridableLineEdit.py
        variantComboBox.py
        qt.py
        usdviewstyle.qss

    PYSIDE_UI_FILES
        mainWindowUI.ui
        watchWindowUI.ui
        adjustClippingUI.ui
        adjustDefaultMaterialUI.ui
        attributeValueEditorUI.ui
        pythonExpressionPromptUI.ui
)

install(DIRECTORY
    icons
    DESTINATION lib/python/pxr/Usdviewq
)
