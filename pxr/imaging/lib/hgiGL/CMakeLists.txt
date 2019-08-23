set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hgiGL)

pxr_library(hgiGL
    LIBRARIES
        arch
        hgi
        tf
        trace
        ${GLEW_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${OPENGL_INCLUDE_DIR}

    PRIVATE_CLASSES
        conversions
        diagnostic

    PUBLIC_CLASSES
        blitEncoder
        graphicsEncoder
        hgi
        immediateCommandBuffer
        texture

    PUBLIC_HEADERS
        api.h

)
