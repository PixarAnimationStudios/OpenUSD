set(PXR_PACKAGE px_vp20)

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

    PUBLIC_CLASSES
        glslProgram
        utils
        utils_legacy
)

