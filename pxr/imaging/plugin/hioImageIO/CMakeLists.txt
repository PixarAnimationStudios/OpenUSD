set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hioImageIO)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

if(APPLE)
    find_library(FOUNDATION Foundation ${CMAKE_SYSTEM_FRAMEWORK_PATH} REQUIRED)
    message("Foundation framework is: ${FOUNDATION}")

    find_library(IMAGEIO ImageIO ${CMAKE_SYSTEM_FRAMEWORK_PATH} REQUIRED)
    message("ImageIO framework is: ${IMAGEIO}")
    set(PLATFORM_LIBS ${FOUNDATION} ${IMAGEIO} ${PLATFORM_LIBS})
    find_library(COREGRAPHICS CoreGraphics ${CMAKE_SYSTEM_FRAMEWORK_PATH} REQUIRED)
    message("CoreGraphics framework is: ${COREGRAPHICS}")
    set(PLATFORM_LIBS ${FOUNDATION} ${COREGRAPHICS} ${PLATFORM_LIBS})
endif()

pxr_plugin(hioImageIO
    LIBRARIES
        ar
        arch
        gf
        hio
        tf
        ${PLATFORM_LIBS}

    CPPFILES
        imageIOImage.mm

    RESOURCE_FILES
        plugInfo.json
)
