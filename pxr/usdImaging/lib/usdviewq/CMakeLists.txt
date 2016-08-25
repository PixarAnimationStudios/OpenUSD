set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT QT4_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Qt4")
    return()
endif()

pxr_shared_library(usdviewq
    LIBRARIES
        tf
        usd
        usdGeom
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        utils

    CPPFILES
        moduleDeps.cpp
        
    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        module.cpp
        wrapUtils.cpp

    PYTHON_FILES
        __init__.py
        arrayAttributeView.py
        customAttributes.py
        mainWindow.py
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
        nodeViewItem.py
        common.py
        attributeValueEditor.py
        pythonExpressionPrompt.py
        referenceEditor.py
        overridableLineEdit.py
        icons/branch-closed.png
        icons/branch-closed-selected.png
        icons/branch-open.png
        icons/branch-open-selected.png
        usdviewstyle.qss

    PYSIDE_UI_FILES
        mainWindowUI.ui
        watchWindowUI.ui
        adjustClippingUI.ui
        attributeValueEditorUI.ui
        pythonExpressionPromptUI.ui
        referenceEditorUI.ui
)

install(DIRECTORY
    icons
    DESTINATION lib/python/pxr/Usdviewq
)
