set(PXR_PACKAGE pxrUsdMayaGL)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        arch
        garch
        gf
        glf
        hd
        hdx
        plug
        px_vp20
        sdf
        tf
        usd
        usdImaging
        usdImagingGL
        usdMaya
        ${Boost_LIBRARIES}
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        batchRenderer
        hdRenderer
        proxyDrawOverride
        proxyShapeUI
        softSelectHelper

    DISABLE_PRECOMPILED_HEADERS
)

