set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE garch)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

if(APPLE)
    set(GARCH_GLPLATFORMCONTEXT       glPlatformContextDarwin)
    set(GARCH_GLPLATFORMDEBUGWINDOW   glPlatformDebugWindowDarwin)
    set(GARCH_SOURCE_EXTENSION        mm)
    set(GARCH_PLATFORM_LIBRARIES      "-framework Foundation")
    if (PXR_APPLE_EMBEDDED)
        list(APPEND GARCH_PLATFORM_LIBRARIES "-framework UIKit")
    else()
        list(APPEND GARCH_PLATFORM_LIBRARIES "-framework AppKit")
        list(APPEND GARCH_PLATFORM_LIBRARIES OpenGL::GL)
    endif()
elseif(X11_FOUND)
    set(GARCH_GLPLATFORMCONTEXT       glPlatformContextGLX)
    set(GARCH_GLPLATFORMDEBUGWINDOW   glPlatformDebugWindowGLX)
    set(GARCH_SOURCE_EXTENSION        cpp)
    set(GARCH_PLATFORM_LIBRARIES      ${X11_LIBRARIES} OpenGL::GL)
elseif(WIN32)
    set(GARCH_GLPLATFORMCONTEXT       glPlatformContextWindows)
    set(GARCH_GLPLATFORMDEBUGWINDOW   glPlatformDebugWindowWindows)
    set(GARCH_SOURCE_EXTENSION        cpp)
    set(GARCH_PLATFORM_LIBRARIES      OpenGL::GL)
endif()

pxr_library(garch
    LIBRARIES
        arch
        tf
        ${GARCH_PLATFORM_LIBRARIES}

    PUBLIC_CLASSES
        glApi
        glDebugWindow
        glPlatformDebugContext

    PUBLIC_HEADERS
        api.h
        gl.h
        glPlatformContext.h
        khrplatform.h
        ${GARCH_GLPLATFORMCONTEXT}.h

    PRIVATE_HEADERS
        ${GARCH_GLPLATFORMDEBUGWINDOW}.h

    CPPFILES
        ${GARCH_GLPLATFORMCONTEXT}.${GARCH_SOURCE_EXTENSION}
        ${GARCH_GLPLATFORMDEBUGWINDOW}.${GARCH_SOURCE_EXTENSION}

    PYMODULE_CPPFILES
        module.cpp
        wrapPlatformDebugContext.cpp

    PYMODULE_FILES
        __init__.py
)
