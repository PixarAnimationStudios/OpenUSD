set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glfq)

if (NOT QT4_FOUND)
    message(WARNING "Not building ${PXR_PACKAGE} because of missing dependency: Qt4")
    return()
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
        ${Boost_SYSTEM_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${GLEW_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}

    PUBLIC_CLASSES
        glDebugContext

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp

    PYTHON_FILES
        __init__.py

)
