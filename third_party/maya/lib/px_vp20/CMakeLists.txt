set(PXR_PACKAGE px_vp20)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        garch
        gf
        glf
        tf
        ${GLEW_LIBRARY}
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaUI_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${OPENGL_gl_LIBRARY}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${MAYA_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIRS}

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        glslProgram
        utils
        utils_legacy

    DISABLE_PRECOMPILED_HEADERS
)

