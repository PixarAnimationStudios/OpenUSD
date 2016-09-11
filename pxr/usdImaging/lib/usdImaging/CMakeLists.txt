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
        defaultShaderAdapter
        delegate
        engine
        gl
        hdEngine
        inheritedCache
        instancerContext
        refEngine
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
        api.h
        version.h
        shaderAdapter.h

    PRIVATE_CLASSES
        defaultTaskDelegate

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapGL.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

if (QT4_FOUND)
    pxr_build_test(testUsdImagingBasicDrawing
        LIBRARIES
            hd
            glf
            glfq
            usd
            usdUtils
            usdImaging
            sdf
            Qt4::QtCore
            Qt4::QtOpenGL
        CPPFILES
            unitTestGLDrawing.cpp
            testenv/testUsdImagingBasicDrawing.cpp
    )
endif()
