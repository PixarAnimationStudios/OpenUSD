set(PXR_PACKAGE pxrUsdMayaGL)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        arch
        gf
        glf
        hd
        hdSt
        hdx
        px_vp20
        sdf
        tf
        usd
        usdGeom
        usdImaging
        usdImagingGL
        usdMaya
        vt
        ${MAYA_Foundation_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h
        renderParams.h

    PUBLIC_CLASSES
        batchRenderer
        debugCodes
        hdImagingShapeDrawOverride
        hdImagingShapeUI
        hdRenderer
        proxyDrawOverride
        proxyShapeUI
        sceneDelegate
        shapeAdapter
        softSelectHelper
        usdProxyShapeAdapter
        userData

    PRIVATE_CLASSES
        instancerImager
        instancerShapeAdapter

    CPPFILES
        proxyShapeDelegate

    DISABLE_PRECOMPILED_HEADERS
)

