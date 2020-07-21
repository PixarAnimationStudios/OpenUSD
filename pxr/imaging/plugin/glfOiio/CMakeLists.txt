set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE glfOiio)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
            "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_plugin(glfOiio
    LIBRARIES
        ar
        arch
        gf
        glf
        tf
        ${OIIO_LIBRARIES}
        ${OPENEXR_LIBRARY}

    INCLUDE_DIRS
        ${OIIO_INCLUDE_DIRS}
        ${OPENEXR_INCLUDE_DIRS}

    CPPFILES
        oiioImage.cpp

    RESOURCE_FILES
        plugInfo.json
)
