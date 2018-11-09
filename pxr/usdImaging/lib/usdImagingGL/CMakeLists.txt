set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingGL)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

# Note: both python include directives are included for compatibility.
pxr_library(usdImagingGL
    LIBRARIES
        gf
        tf
        plug
        trace
        vt
        work
        garch
        glf
        hd
        hdx
        pxOsd
        sdf
        sdr
        usd
        usdGeom
        usdHydra
        usdShade
        usdImaging
        ar
        ${Boost_PYTHON_LIBRARY}
        ${PYTHON_LIBRARY}
        ${OPENGL_gl_LIBRARY}
        ${OPENGL_glu_LIBRARY}
        ${GLEW_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${OPENGL_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        drawModeAdapter
        engine
        gl
        hydraMaterialAdapter
        package
        renderParams

    PUBLIC_HEADERS
        api.h
        rendererSettings.h
        version.h

    PRIVATE_CLASSES
        legacyEngine
        textureUtils

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapEngine.cpp
        wrapRenderParams.cpp
        wrapRendererSettings.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/drawMode.glslfx
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
