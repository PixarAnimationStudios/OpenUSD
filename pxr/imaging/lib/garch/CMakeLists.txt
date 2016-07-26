set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE garch)

if (X11_FOUND)
    set(GLPLATFORMCONTEXT_H   glPlatformContextGLX.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextGLX.cpp)
elseif(APPLE)
    set(GLPLATFORMCONTEXT_H   glPlatformContextDarwin.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextDarwin.mm)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
elseif(WIN32)
    set(GLPLATFORMCONTEXT_H   glPlatformContextWindows.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextWindows.cpp)
endif()

pxr_shared_library(garch
    LIBRARIES
        arch
        ${OPENGL_gl_LIBRARY}
        ${APPKIT_LIBRARY}

    PUBLIC_CLASSES
        glPlatformContext

    PUBLIC_HEADERS
        gl.h
        glext.h
        glu.h
        glut.h
        ${GLPLATFORMCONTEXT_H}

    CPPFILES
        ${GLPLATFORMCONTEXT_CPP}
)
