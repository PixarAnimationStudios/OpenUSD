set(PXR_PACKAGE pxrUsdMayaGL)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        arch
        garch
        gf
        tf
        plug
        glf
        hd
        hdx
        px_vp20
        sdf
        usd
        usdImaging
        usdImagingGL
        usdMaya
        ${Boost_LIBRARIES}
        ${GLUT_LIBRARIES}
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_CLASSES
        hdRenderer
        batchRenderer
        proxyDrawOverride
        proxyShapeUI
        softSelectHelper
)

