set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImagingGL)

if (NOT ${PXR_BUILD_GPU_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
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
        hio
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
        ${PYTHON_LIBRARIES}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${PYTHON_INCLUDE_PATH}
        ${PYTHON_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        drawModeAdapter
        engine
        package
        renderParams

    PUBLIC_HEADERS
        api.h
        rendererSettings.h
        version.h

    PRIVATE_CLASSES
        legacyEngine

    PRIVATE_HEADERS
        unitTestGLDrawing.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapEngine.cpp
        wrapRenderParams.cpp
        wrapRendererCommands.cpp
        wrapRendererSettings.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/drawMode.glslfx
)

#
# C++ tests - Only build on all platforms for now.
# 
set(TEST_LIBS "arch" "garch" "glf" "hd" "sdf" "tf" "usd" "usdUtils" "usdImaging" "usdImagingGL")

pxr_build_test(testUsdImagingGLBasicDrawing
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLBasicDrawing.cpp
)

pxr_build_test(testUsdImagingGLBatchedDrawing
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLBatchedDrawing.cpp
)

pxr_build_test(testUsdImagingGLHighlight
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLHighlight.cpp
)

pxr_build_test(testUsdImagingGLPickAndHighlight
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLPickAndHighlight.cpp
)

pxr_build_test(testUsdImagingGLPopOut
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLPopOut.cpp
)

pxr_build_test(testUsdImagingGLPurpose
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLPurpose.cpp
)

pxr_build_test(testUsdImagingGLResync
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLResync.cpp
)

pxr_build_test(testUsdImagingGLSurfaceShader
    LIBRARIES
        ${TEST_LIBS}
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testUsdImagingGLSurfaceShader.cpp
)
