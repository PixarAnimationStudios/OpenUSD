set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingGL)

# Note: both python include directives are included for compatibility.
pxr_shared_library(usdImagingGL
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
        usdImaging
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
        engine
        gl
        hdEngine
        refEngine

    PUBLIC_HEADERS
        api.h

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapGL.cpp

    PYTHON_FILES
        __init__.py
)

if (X11_FOUND OR APPLE)
    pxr_build_test(testUsdImagingGLBasicDrawing
        LIBRARIES
            hd
            garch
            glf
            usd
            usdUtils
            usdImaging
            usdImagingGL
            sdf
        CPPFILES
            unitTestGLDrawing.cpp
            testenv/testUsdImagingGLBasicDrawing.cpp
    )
endif()
