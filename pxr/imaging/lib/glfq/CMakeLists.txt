set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glfq)

if (NOT Qt5Core_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Qt4")
    return()
endif()

if(APPLE)
    FIND_LIBRARY(COCOA_LIBRARY Cocoa)
    set(COCOA_SRC selectVisual.mm)
endif()

pxr_shared_library(glfq
    LIBRARIES
        garch
        glf
        arch
        tf
        Qt5::Core
        Qt5::Gui
        Qt5::OpenGL
        Qt5::Xml
        Qt5::Network
        ${OPENGL_gl_LIBRARY}
        ${GLEW_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}

    PUBLIC_CLASSES
        glDebugContext

    PUBLIC_HEADERS
        api.h
        
    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp

    PYTHON_FILES
        __init__.py

)
