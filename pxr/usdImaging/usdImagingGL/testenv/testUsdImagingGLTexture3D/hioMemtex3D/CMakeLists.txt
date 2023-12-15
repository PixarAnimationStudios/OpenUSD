set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE hioMemtex3D)

if (NOT ${PXR_BUILD_GPU_SUPPORT} )
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

pxr_plugin(hioMemtex3D
    LIBRARIES
        ar
        arch
        gf
        hio
        tf

    CPPFILES
        memtex3d.cpp

    RESOURCE_FILES
        plugInfo.json
)
