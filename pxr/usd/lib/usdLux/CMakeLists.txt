set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLux)

pxr_library(usdLux
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_CLASSES
        blackbody
        cylinderLight
        diskLight
        distantLight
        domeLight
        geometryLight
        light
        listAPI
        shapingAPI
        shadowAPI
        lightFilter
        lightPortal
        rectLight
        sphereLight
        tokens

    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapCylinderLight.cpp
        wrapDiskLight.cpp
        wrapDistantLight.cpp
        wrapDomeLight.cpp
        wrapLightPortal.cpp
        wrapGeometryLight.cpp
        wrapLight.cpp
        wrapListAPI.cpp
        wrapShapingAPI.cpp
        wrapShadowAPI.cpp
        wrapLightFilter.cpp
        wrapRectLight.cpp
        wrapSphereLight.cpp
        wrapTokens.cpp

    PYMODULE_FILES 
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdLux/schema.usda
)

pxr_test_scripts(
    testenv/testUsdLuxLight.py
    testenv/testUsdLuxLinkingAPI.py
    testenv/testUsdLuxListAPI.py
)

pxr_register_test(testUsdLuxLight
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxLight"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    DEST testUsdLuxLinkingAPI
    SRC testenv/testUsdLuxLinkingAPI
)

pxr_register_test(testUsdLuxLinkingAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxLinkingAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_install_test_dir(
    DEST testUsdLuxListAPI
    SRC testenv/testUsdLuxListAPI
)

pxr_register_test(testUsdLuxListAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxListAPI"
    EXPECTED_RETURN_CODE 0
)


