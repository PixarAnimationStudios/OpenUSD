set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiInterop)

if (NOT PXR_ENABLE_GL_SUPPORT)
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

set(optionalLibraries "")
set(optionalCppFiles "")
set(optionalPrivateHeaders "")

# Destination of hgiInterop is always OpenGL, so always include it
list(APPEND optionalLibraries garch)

list(APPEND optionalLibraries hgiGL)
list(APPEND optionalCppFiles opengl.cpp)
list(APPEND optionalPrivateHeaders opengl.h)

if (PXR_ENABLE_VULKAN_SUPPORT)
    list(APPEND optionalLibraries hgiVulkan)
    list(APPEND optionalCppFiles vulkan.cpp)
    list(APPEND optionalPrivateHeaders vulkan.h)
endif()

if (PXR_ENABLE_METAL_SUPPORT)
    add_compile_options(-x objective-c++)
    list(APPEND optionalLibraries hgiMetal)
    if (NOT PXR_APPLE_EMBEDDED)
        list(APPEND optionalLibraries "-framework Foundation")
        list(APPEND optionalLibraries "-framework CoreVideo")
        list(APPEND optionalCppFiles metal.mm)
        list(APPEND optionalPrivateHeaders metal.h)
    endif()
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
