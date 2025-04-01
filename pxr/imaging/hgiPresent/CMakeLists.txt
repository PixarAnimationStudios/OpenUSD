set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiPresent)

if (NOT PXR_BUILD_GPU_SUPPORT)
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

set(optionalLibraries "")
set(optionalPrivateClasses "")
set(optionalPrivateHeaders "")
set(optionalCppFiles "")

if (PXR_ENABLE_GL_SUPPORT AND NOT PXR_APPLE_EMBEDDED)
    list(APPEND optionalLibraries hgiInterop)
    list(APPEND optionalPrivateClasses glInterop)
endif()

if (PXR_ENABLE_METAL_SUPPORT)
    list(APPEND optionalLibraries hgiMetal)
    list(APPEND optionalLibraries "-framework CoreGraphics")
    list(APPEND optionalLibraries "-framework QuartzCore")
    list(APPEND optionalPrivateHeaders metal.h)
    list(APPEND optionalCppFiles metal.mm)
endif()

if (PXR_ENABLE_VULKAN_SUPPORT)
    list(APPEND optionalLibraries hgiVulkan)
    list(APPEND optionalPrivateClasses vulkan)
endif()

pxr_library(hgiPresent
    LIBRARIES
        gf
        tf
        hgi
        vt
        ${optionalLibraries}

    PUBLIC_CLASSES
        debugCodes
        present

    PUBLIC_HEADERS
        api.h
        interopHandle.h
        windowHandle.h

    PRIVATE_CLASSES
        noOp
        ${optionalPrivateClasses}

    PRIVATE_HEADERS
        presentImpl.h
        ${optionalPrivateHeaders}

    CPPFILES
        ${optionalCppFiles}
)

if (PXR_HEADLESS_TEST_MODE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

set(optionalTestLibraries "")
set(optionalTestClasses)
if (X11_FOUND)
    list(APPEND optionalTestLibraries X11::X11)
    list(APPEND optionalTestClasses testenv/testWindowX11)
endif()

pxr_build_test(testHgiPresent
    LIBRARIES
        usdImagingGL
        hgiPresent
        ${optionalTestLibraries}

    CLASSES
        testenv/testWindow
        ${optionalTestClasses}

    CPPFILES
        testenv/testHgiPresent.cpp
)
pxr_install_test_dir(
    SRC testenv/testHgiPresent
    DEST testHgiPresent
)

if (PXR_ENABLE_VULKAN_SUPPORT)
    if (X11_FOUND)
        pxr_register_test(testHgiPresentVulkanX11
            COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHgiPresent -window X11 -capture 0 60 15 -stage testHgiPresent.usda"
            IMAGE_DIFF_COMPARE
                testHgiPresent_frame0.png
                testHgiPresent_frame15.png
                testHgiPresent_frame30.png
                testHgiPresent_frame45.png
            FAIL 0.05
            FAIL_PERCENT 0.5
            WARN 0.01
            WARN_PERCENT 0.5
            EXPECTED_RETURN_CODE 0
            TESTENV testHgiPresent
            ENV
                HGI_ENABLE_VULKAN=1
                HGI_ENABLE_WEBGPU=0
        )
    endif()
endif()
