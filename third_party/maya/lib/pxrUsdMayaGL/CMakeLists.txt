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
        ${Boost_LIBRARIES}
        "C:/Program Files (x86)/NVIDIA Corporation/Cg/lib.x64/glut32.lib"
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
)

