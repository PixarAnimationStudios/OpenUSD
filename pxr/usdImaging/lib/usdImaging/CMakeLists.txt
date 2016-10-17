set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImaging)

set(optionalPublicClasses "")
if (QT4_FOUND)
    list(APPEND optionalPublicClasses unitTestHelper)
endif()

# Note: both python include directives are included for compatibility.
pxr_shared_library(usdImaging
    LIBRARIES
        gf
        tf
        plug
        tracelite
        vt
        work
        garch
        glf
        hd
        hdx
        pxOsd
        sdf
        usd
        usdGeom
        usdShade
        usdHydra
        ar
        ${Boost_PYTHON_LIBRARY}
        ${PYTHON_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}
        ${GLEW_LIBRARY}
        ${TBB_tbb_LIBRARY}
        ${TBB_tbbmalloc_LIBRARY}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        adapterRegistry
        debugCodes
        delegate
        inheritedCache
        instancerContext
        shaderAdapter
        tokens
        valueCache
        taskDelegate
        basisCurvesAdapter
        capsuleAdapter
        coneAdapter
        cubeAdapter
        cylinderAdapter
        gprimAdapter
        instanceAdapter
        meshAdapter
        pointsAdapter
        primAdapter
        sphereAdapter
        nurbsPatchAdapter
        ${optionalPublicClasses}

    PUBLIC_HEADERS
        version.h

    RESOURCE_FILES
        plugInfo.json
)

