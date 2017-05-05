set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdLux)

pxr_shared_library(usdLux
    LIBRARIES
        tf
        vt
        sdf
        usd
        usdGeom

    PUBLIC_CLASSES
        diskLight
        distantLight
        domeLight
        geometryLight
        light
        listAPI
        linkingAPI
        shapingAPI
        shadowAPI
        lightFilter
        lightPortal
        pointLight
        rectLight
        sphereLight
        tokens

    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapDiskLight.cpp
        wrapDistantLight.cpp
        wrapDomeLight.cpp
        wrapLightPortal.cpp
        wrapGeometryLight.cpp
        wrapLight.cpp
        wrapListAPI.cpp
        wrapLinkingAPI.cpp
        wrapShapingAPI.cpp
        wrapShadowAPI.cpp
        wrapLightFilter.cpp
        wrapPointLight.cpp
        wrapRectLight.cpp
        wrapSphereLight.cpp
        wrapTokens.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)

