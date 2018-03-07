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
        linkingAPI
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
        wrapLinkingAPI.cpp
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

