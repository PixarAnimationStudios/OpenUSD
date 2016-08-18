set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE garch)

if(APPLE)
    set(GLPLATFORMCONTEXT_H   glPlatformContextDarwin.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextDarwin.mm)
    FIND_LIBRARY(APPKIT_LIBRARY AppKit)
elseif(X11_FOUND)
    set(GLPLATFORMCONTEXT_H   glPlatformContextGLX.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextGLX.cpp)
elseif(WIN32)
    set(GLPLATFORMCONTEXT_H   glPlatformContextWindows.h)
    set(GLPLATFORMCONTEXT_CPP glPlatformContextWindows.cpp)
endif()

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(garch
    LIBRARIES
        arch
        ${Boost_SYSTEM_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${APPKIT_LIBRARY}
		
	INCLUDE_DIRS
		${Boost_INCLUDE_DIRS}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        glPlatformContext

    PUBLIC_HEADERS
        api.h
        gl.h
        glext.h
        glu.h
        glut.h
        ${GLPLATFORMCONTEXT_H}

    CPPFILES
        ${GLPLATFORMCONTEXT_CPP}
        ${MODULE_ENTRYPOINT_CPP}
)
