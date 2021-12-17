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
        ${OPENVDB_LIBRARY}

    INCLUDE_DIRS
        ${OPENVDB_INCLUDE_DIR}

    PUBLIC_CLASSES
        utils
        vdbAssetInterface

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        debugCodes
        vdbTextureData

    RESOURCE_FILES
        plugInfo.json
)
