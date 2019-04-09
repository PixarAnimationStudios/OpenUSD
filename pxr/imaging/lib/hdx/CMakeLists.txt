set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdx)

if (NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_ENABLE_GL_SUPPORT is OFF")
    return()
endif()

set (optionalCppFiles "")
if (PXR_BUILD_OPENCOLORIO_PLUGIN)
    list(APPEND optionalLibs ${OCIO_LIBRARIES})
    list(APPEND optionalIncludeDirs ${OCIO_INCLUDE_DIRS})
endif()

pxr_library(hdx
    LIBRARIES
        plug
        tf
        vt
        gf
        work
        garch
        glf
        pxOsd
        hd
        hdSt
        cameraUtil
        sdf
        ${GLEW_LIBRARY}
        ${optionalLibs}

    INCLUDE_DIRS
        ${GLEW_INCLUDE_DIR}
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        colorizeTask
        colorCorrectionTask
        compositor
        drawTargetRenderPass
        drawTargetTask
        drawTargetResolveTask
        intersector
        package
        progressiveTask
        rendererPlugin
        rendererPluginRegistry
        renderTask
        renderSetupTask
        selectionTask
        selectionTracker
        shadowTask
        simpleLightingShader
        simpleLightTask
        taskController
        tokens
        types

    PUBLIC_HEADERS
        shadowMatrixComputation.h
        version.h
        api.h

    PRIVATE_CLASSES
        debugCodes
        unitTestDelegate

    PRIVATE_HEADERS
        unitTestGLDrawing.h
        unitTestUtils.h

    RESOURCE_FILES
        plugInfo.json
        shaders/colorCorrection.glslfx
        shaders/fullscreen.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassShader.glslfx
        shaders/renderPassIdShader.glslfx
        shaders/renderPassShadowShader.glslfx
        shaders/selection.glslfx
        shaders/simpleLightingShader.glslfx
)

if (X11_FOUND)
pxr_build_test(testHdxRenderTask
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        testenv/testHdxRenderTask.cpp
)
pxr_build_test(testHdxLightAndShadow
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        testenv/testHdxLightAndShadow.cpp
)
pxr_build_test(testHdxIdRender
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestGLDrawing.cpp
        testenv/testHdxIdRender.cpp
)
pxr_build_test(testHdxPickAndHighlight
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestGLDrawing.cpp
        unitTestUtils.cpp
        testenv/testHdxPickAndHighlight.cpp
)
pxr_build_test(testHdxCameraAndLight
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        testenv/testHdxCameraAndLight.cpp
)
pxr_build_test(testHdxPickingWithReprAndRefineChanges
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestGLDrawing.cpp
        unitTestUtils.cpp
        testenv/testHdxPickingWithReprAndRefineChanges.cpp
)
pxr_build_test(testHdxUnpickablesAsOccluders
    LIBRARIES
        hdx
        hd
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestGLDrawing.cpp
        unitTestUtils.cpp
        testenv/testHdxUnpickablesAsOccluders.cpp
)
endif()
