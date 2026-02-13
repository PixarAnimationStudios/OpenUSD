set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdUI)

pxr_library(usdUI
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        vt
        sdf
        usd

    PUBLIC_CLASSES
        attributeHints
        objectHints
        primHints
        propertyHints

    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        wrapAttributeHints.cpp
        wrapObjectHints.cpp
        wrapPrimHints.cpp
        wrapPropertyHints.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdUIHints.py
    testenv/testUsdUINodeGraphNode.py
    testenv/testUsdUISceneGraphPrim.py
)

pxr_register_test(testUsdUIHints
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUIHints"
    TESTENV testenv/testUsdUIHints
)
pxr_register_test(testUsdUINodeGraphNode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUINodeGraphNode"
    TESTENV testenv/testUsdUINodeGraphNode
)
pxr_register_test(testUsdUISceneGraphPrim
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUISceneGraphPrim"
    TESTENV testenv/testUsdUISceneGraphPrim
)

pxr_build_test(testUsdUIAccessibilityAPI
    LIBRARIES
        tf
        usd
        usdUI
    CPPFILES
        testenv/testUsdUIAccessibilityAPI.cpp
)

pxr_register_test(testUsdUIAccessibilityAPI
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdUIAccessibilityAPI"
    EXPECTED_RETURN_CODE 0
)
