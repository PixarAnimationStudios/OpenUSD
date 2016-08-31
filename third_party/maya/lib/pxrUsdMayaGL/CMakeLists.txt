set(PXR_PACKAGE pxrUsdMayaGL)

add_definitions(
	/DUSDMAYAGL_EXPORTS
)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        arch
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
        usdMaya
        ${Boost_LIBRARIES}
        ${GLUT_LIBRARIES}
        ${MAYA_LIBRARIES}

    INCLUDE_DIRS
		${TBB_INCLUDE_DIRS}
		${GLUT_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        hdRenderer
        batchRenderer
        proxyDrawOverride
        proxyShapeUI
        softSelectHelper
)

