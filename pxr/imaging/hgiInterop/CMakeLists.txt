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

if (PXR_ENABLE_GL_SUPPORT)
    list(APPEND optionalCppFiles opengl.cpp)
    list(APPEND optionalPrivateHeaders opengl.h)
endif()

if (PXR_ENABLE_VULKAN_SUPPORT)
    list(APPEND optionalLibraries hgiVulkan)
    list(APPEND optionalCppFiles vulkan.cpp)
    list(APPEND optionalPrivateHeaders vulkan.h)
endif()

if (PXR_ENABLE_METAL_SUPPORT)
    add_compile_options(-x objective-c++)
    list(APPEND optionalLibraries "-framework CoreVideo" hgiMetal)
    list(APPEND optionalCppFiles metal.mm)
    list(APPEND optionalPrivateHeaders metal.h)
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
