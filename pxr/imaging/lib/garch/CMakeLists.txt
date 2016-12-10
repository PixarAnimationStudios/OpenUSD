set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE garch)

if(APPLE)
    set(GLPLATFORMCONTEXT_H   glPlatformContextDarwin.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextDarwin.mm)
    set(GLPLATFORMDEBUGWINDOW_H   glPlatformDebugWindowDarwin.h)
    set(GLPLATFORMDEBUGWINDOW_CPP glPlatformDebugWindowDarwin.mm)
    set(GLDEBUGWINDOW         glDebugWindow)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
elseif(X11_FOUND)
    set(GLPLATFORMCONTEXT_H   glPlatformContextGLX.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextGLX.cpp)
    set(GLPLATFORMDEBUGWINDOW_H   glPlatformDebugWindowGLX.h)
    set(GLPLATFORMDEBUGWINDOW_CPP glPlatformDebugWindowGLX.cpp)
    set(GLDEBUGWINDOW         glDebugWindow)
elseif(WIN32)
    set(GLPLATFORMCONTEXT_H   glPlatformContextWindows.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextWindows.cpp)
endif()

pxr_shared_library(garch
    LIBRARIES
        arch
        tf
        ${Boost_SYSTEM_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${APPKIT_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        glPlatformContext
        glPlatformDebugContext
        ${GLDEBUGWINDOW}

    PUBLIC_HEADERS
        gl.h
        glext.h
        glu.h
        glut.h
        ${GLPLATFORMCONTEXT_H}

    PRIVATE_HEADERS
        ${GLPLATFORMDEBUGWINDOW_H}

    CPPFILES
        moduleDeps.cpp
        ${GLPLATFORMCONTEXT_CPP}
        ${GLPLATFORMDEBUGWINDOW_CPP}

    PYMODULE_CPPFILES
        module.cpp
        wrapPlatformDebugContext.cpp

    PYTHON_FILES
        __init__.py
)
