# Install hdPrman's headers in pxr/imaging alongside other shared
# libraries for Hydra backends, like hdSt.
set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrman)

# RiEntryPoints.h incorrectly makes libloadprman symbols DLL import
# This is only temporary and will be removed shortly
add_definitions(-DPRMANBUILDINGAPI)

if(WIN32)
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libmmd.lib /NODEFAULTLIB:libirc.lib /NODEFAULTLIB:svml_dispmd.lib /NODEFAULTLIB:libdecimal.lib" )
endif()

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        ar
        plug
        tf
        vt
        gf
        hd
        usdRi
        ${RENDERMAN_LIBRARY}

    INCLUDE_DIRS
        ${RENDERMAN_INCLUDE_DIR}

    PUBLIC_CLASSES
        context
        convertPreviewMaterial
        coordSys
        debugCodes
        basisCurves
        instancer
        light
        material
        mesh
        points
        renderDelegate
        renderPass
        rixStrings
        volume

    PUBLIC_HEADERS
        api.h
        renderParam.h

    DISABLE_PRECOMPILED_HEADERS
)

# Include paths to hdPrman headers within hdPrman itself look like
# #include "hdPrman/...", so we need to add include/pxr/imaging
# to the include paths for this library. We use CMAKE_BINARY_DIR
# to include headers out of the build directory, which is what
# normally happens in the USD build.
target_include_directories(${PXR_PACKAGE}
    PRIVATE
        "${CMAKE_BINARY_DIR}/include/pxr/imaging"
)

if(WIN32 AND NOT BUILD_SHARED_LIBS)
    # XXX: Temporarily disable this test on static Windows builds
    #      due to symbol visibility issues.
else()
    pxr_build_test(testHdPrman
        LIBRARIES
            hdPrman
            hd
            usd
            usdImaging
            ${RENDERMAN_LIBRARY}
        CPPFILES
            testenv/testHdPrman.cpp
    )
endif()
