set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiInterop)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
    return()
endif()

set(optionalLibraries "")
set(optionalCppFiles "")
set(optionalPrivateHeaders "")

# Destination of hgiInterop is always OpenGL, so always include garch
list(APPEND optionalLibraries garch)
list(APPEND optionalLibraries hgiGL)

if (PXR_ENABLE_METAL_SUPPORT)
    add_compile_options(-x objective-c++)
    list(APPEND optionalLibraries hgiMetal)
endif()

if (PXR_ENABLE_METAL_SUPPORT AND PXR_ENABLE_GL_SUPPORT)
    FIND_LIBRARY(COREVIDEO_LIBRARY CoreVideo)
    list(APPEND optionalLibraries ${COREVIDEO_LIBRARY})
    list(APPEND optionalCppFiles metal.mm)
    list(APPEND optionalPrivateHeaders metal.h)
elseif(PXR_ENABLE_VULKAN_SUPPORT)
    list(APPEND optionalLibraries hgiVulkan)
    list(APPEND optionalCppFiles vulkan.mm)
    list(APPEND optionalPrivateHeaders vulkan.h)
elseif(PXR_ENABLE_GL_SUPPORT)
    # No OpenGL-to-OpenGL interop when using Metal.
    list(APPEND optionalCppFiles opengl.cpp)
    list(APPEND optionalPrivateHeaders opengl.h)
endif()

pxr_library(hgiInterop
    LIBRARIES
        gf
        tf
        hgi
        vt
        ${optionalLibraries}

    PUBLIC_HEADERS
        api.h
        hgiInterop.h

    PRIVATE_HEADERS
        ${optionalPrivateHeaders}

    CPPFILES
        hgiInterop.cpp
        ${optionalCppFiles}
)
