set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdUI)

pxr_library(usdUI
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        sdf
        usd

    PUBLIC_HEADERS
        api.h
    
    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
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
