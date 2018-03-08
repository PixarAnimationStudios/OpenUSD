set(PXR_PACKAGE pxrUsdMayaGL)

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
        usdImaging
        usdImagingGL
        usdMaya
        vt
        ${Boost_LIBRARIES}
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h
        renderParams.h

    PUBLIC_CLASSES
        batchRenderer
        debugCodes
        hdRenderer
        proxyDrawOverride
        proxyShapeUI
        sceneDelegate
        shapeAdapter
        softSelectHelper

    DISABLE_PRECOMPILED_HEADERS
)

