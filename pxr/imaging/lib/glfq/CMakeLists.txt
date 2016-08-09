set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glfq)

if (NOT QT4_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Qt4")
    return()
endif()

if(APPLE)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    set(COCOA_SRC selectVisual.mm)
endif()

if(WIN32)
    set(MODULE_ENTRYPOINT_CPP DllMain.cpp)
endif()

pxr_shared_library(glfq
    LIBRARIES
        garch
        glf
        arch
        tf
        Qt4::QtCore
        Qt4::QtGui
        Qt4::QtOpenGL
        Qt4::QtXml
        Qt4::QtNetwork
        ${OPENGL_gl_LIBRARY}
        ${GLEW_LIBRARY}
        ${COCOA_LIBRARY}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}

    PUBLIC_CLASSES
        glDebugContext
        glPlatformDebugContext

    PUBLIC_HEADERS
        api.h
        
    CPPFILES
        ${MODULE_ENTRYPOINT_CPP}
        moduleDeps.cpp
        ${COCOA_SRC}

    PYMODULE_CPPFILES
        module.cpp
        wrapPlatformDebugContext.cpp

    PYTHON_FILES
        __init__.py
)
