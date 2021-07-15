set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hioOpenVDB)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

pxr_library(hioOpenVDB
    LIBRARIES
        ar
        gf
        hio
        tf
        usd
        ${OPENEXR_Half_LIBRARY}
        ${OPENVDB_LIBRARY}

    INCLUDE_DIRS
        ${OPENVDB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        vdbAssetInterface

    PRIVATE_CLASSES
        debugCodes
        vdbTextureData

    RESOURCE_FILES
        plugInfo.json
)
