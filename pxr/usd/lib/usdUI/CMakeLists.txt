set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdUI)

pxr_shared_library(usdUI
    LIBRARIES
        tf
        vt
        sdf
        usd

    PUBLIC_CLASSES
        nodeGraphNodeAPI
        tokens

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapNodeGraphNodeAPI.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

pxr_test_scripts(
    testenv/testUsdUINodeGraphNode.py
)

pxr_install_test_dir(
    SRC testenv/testUsdUINodeGraphNode
    DEST testUsdUINodeGraphNode
)

pxr_register_test(testUsdUINodeGraphNode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUINodeGraphNode"
)
