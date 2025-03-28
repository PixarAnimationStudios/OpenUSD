set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hioOpenVDB)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

if (WIN32)
    # OpenVDB uses constants from <cmath> that aren't available on
    # Windows unless this is defined.
    add_definitions(-D_USE_MATH_DEFINES)
endif()

# Use the import targets set by Imath's package config
if (Imath_FOUND)
    LIST(APPEND __VDB_IMATH_LIBS "Imath::Imath")
else()
    LIST(APPEND __VDB_IMATH_LIBS ${OPENEXR_Half_LIBRARY})
endif()

pxr_library(hioOpenVDB
    LIBRARIES
        ar
        gf
        hio
        tf
        usd
        ${__VDB_IMATH_LIBS}
        ${OPENVDB_LIBRARY}

    INCLUDE_DIRS
        ${OPENVDB_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}

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
