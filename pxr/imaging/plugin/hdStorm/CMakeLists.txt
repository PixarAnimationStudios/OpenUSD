set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdStorm)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

pxr_plugin(hdStorm
    LIBRARIES
        plug
        tf
        trace
        vt
        work
        hd
        hdSt
        ${OPENSUBDIV_LIBRARIES}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        rendererPlugin

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)
