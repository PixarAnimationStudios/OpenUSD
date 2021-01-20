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

if (PXR_ENABLE_METAL_SUPPORT)
    add_compile_options(-x objective-c++)
    FIND_LIBRARY(COREVIDEO_LIBRARY CoreVideo)
    list(APPEND optionalLibraries ${COREVIDEO_LIBRARY} hgiMetal)
    list(APPEND optionalCppFiles metal.mm)
    list(APPEND optionalPrivateHeaders metal.h)
else()
    list(APPEND optionalLibraries hgiGL)
    list(APPEND optionalCppFiles opengl.cpp)
    list(APPEND optionalPrivateHeaders opengl.h)
endif()

pxr_library(hgiInterop
    LIBRARIES
        gf
        tf
        hgi
        ${GLEW_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}
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
