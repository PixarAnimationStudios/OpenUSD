set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiInterop)

set(optionalLibraries "")
set(optionalCppFiles "")
set(optionalPrivateHeaders "")

if (APPLE)
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
        plug
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
