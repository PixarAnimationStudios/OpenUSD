set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdStream)

set(optionalPublicClasses "")

if (OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
endif()
if (OPENSUBDIV_HAS_GLSL_COMPUTE)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_COMPUTE)
endif()

pxr_library(hdStream
    LIBRARIES
        plug
        tf
        tracelite
        vt
        work
        hd
        hdSt
        hdx
        ${OPENSUBDIV_LIBRARIES}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${OPENSUBDIV_INCLUDE_DIR}

    PUBLIC_CLASSES
        rendererPlugin
        taskController

    RESOURCE_FILES
        plugInfo.json
)
