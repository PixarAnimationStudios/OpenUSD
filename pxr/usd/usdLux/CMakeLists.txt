set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLux)

pxr_library(usdLux
    LIBRARIES
        tf
        vt
        ndr
        sdf
        usd
        usdGeom
        usdShade

    PUBLIC_CLASSES
        blackbody
        boundableLightBase
        cylinderLight
        diskLight
        distantLight
        domeLight
        domeLight_1
        geometryLight
        lightAPI
        lightFilter
        lightListAPI
        listAPI
        meshLightAPI
        nonboundableLightBase
        pluginLight
        pluginLightFilter
        portalLight
        rectLight
        shadowAPI
        shapingAPI
        sphereLight
        tokens
        volumeLightAPI

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        discoveryPlugin
        lightDefParser

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBoundableLightBase.cpp
        wrapCylinderLight.cpp
        wrapDiskLight.cpp
        wrapDistantLight.cpp
        wrapDomeLight.cpp
        wrapDomeLight_1.cpp
        wrapGeometryLight.cpp
        wrapLightAPI.cpp
        wrapLightFilter.cpp
        wrapLightListAPI.cpp
        wrapListAPI.cpp
        wrapMeshLightAPI.cpp
        wrapNonboundableLightBase.cpp
        wrapPluginLight.cpp
        wrapPluginLightFilter.cpp
        wrapPortalLight.cpp
        wrapRectLight.cpp
        wrapShadowAPI.cpp
        wrapShapingAPI.cpp
        wrapSphereLight.cpp
        wrapTokens.cpp
        wrapVolumeLightAPI.cpp

    PYMODULE_FILES 
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdLux/schema.usda

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testUsdLuxLight.py
    testenv/testUsdLuxLinkingAPI.py
    testenv/testUsdLuxLightListAPI.py
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
    DEST testUsdLuxLightListAPI
    SRC testenv/testUsdLuxLightListAPI
)

pxr_register_test(testUsdLuxLightListAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdLuxLightListAPI"
    EXPECTED_RETURN_CODE 0
)


