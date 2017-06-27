set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdUI)

pxr_library(usdUI
    LIBRARIES
        tf
        vt
        sdf
        usd

    PUBLIC_CLASSES
        backdrop
        nodeGraphNodeAPI
        sceneGraphPrimAPI
        tokens

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBackdrop.cpp
        wrapNodeGraphNodeAPI.cpp
        wrapSceneGraphPrimAPI.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda
)

pxr_test_scripts(
    testenv/testUsdUINodeGraphNode.py
    testenv/testUsdUISceneGraphPrim.py
)

pxr_install_test_dir(
    SRC testenv/testUsdUINodeGraphNode
    DEST testUsdUINodeGraphNode
)
pxr_install_test_dir(
    SRC testenv/testUsdUISceneGraphPrim
    DEST testUsdUISceneGraphPrim
)

pxr_register_test(testUsdUINodeGraphNode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUINodeGraphNode"
)
pxr_register_test(testUsdUISceneGraphPrim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUISceneGraphPrim"
)
