set(PXR_PACKAGE px_vp20)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        ${MAYA_OpenMaya_LIBRARY}
        ${MAYA_OpenMayaRender_LIBRARY}
        ${OPENGL_gl_LIBRARY}

    INCLUDE_DIRS
        ${OPENGL_INCLUDE_DIRS}
        ${MAYA_INCLUDE_DIRS}

    PUBLIC_CLASSES
        utils
)

