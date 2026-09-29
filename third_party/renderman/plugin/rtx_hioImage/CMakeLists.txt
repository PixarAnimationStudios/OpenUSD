set(PXR_PREFIX "")
set(PXR_PACKAGE RtxHioImage)

pxr_plugin(${PXR_PACKAGE}

    LIBRARIES
        tf
        gf
        trace
        hio
        ${PXRCORE_LIBRARY}
        ${PRMAN_LIBRARY}

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    CPPFILES
        main.cpp 

    PRIVATE_HEADERS

    RESOURCE_FILES

    DISABLE_PRECOMPILED_HEADERS
)
