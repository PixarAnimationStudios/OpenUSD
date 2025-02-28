set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdx)

# XXX We check for PXR_ENABLE_GL_SUPPORT since Hdx currently still uses
# GL via GarchGLApi. Once Hdx uses only Hgi, remove GL_SUPPORT check.
if (NOT ${PXR_BUILD_GPU_SUPPORT} OR NOT ${PXR_ENABLE_GL_SUPPORT})
    message(STATUS
        "Skipping ${PXR_PACKAGE} because PXR_BUILD_GPU_SUPPORT is OFF")
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
        hgi
        hgiInterop
        cameraUtil
        sdf
        ${optionalLibs}

    INCLUDE_DIRS
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        aovInputTask
        boundingBoxTask
        colorizeSelectionTask
        colorChannelTask
        colorCorrectionTask
        drawTargetTask
        effectsShader
        freeCameraSceneDelegate
        freeCameraPrimDataSource
        fullscreenShader
        hgiConversions
        oitBufferAccessor
        oitRenderTask
        oitResolveTask
        oitVolumeRenderTask
        package
        pickTask
        pickFromRenderBufferTask
        presentTask
        renderTask
        renderSetupTask
        selectionSceneIndexObserver
        selectionTask
        selectionTracker
        shadowTask
        simpleLightTask
        skydomeTask
        task
        taskController
        tokens
        types
        visualizeAovTask

    PUBLIC_HEADERS
        shadowMatrixComputation.h
        version.h
        api.h

    PRIVATE_CLASSES
        debugCodes
        unitTestDelegate

    PRIVATE_HEADERS
        unitTestUtils.h

    RESOURCE_FILES
        plugInfo.json
        shaders/boundingBox.glslfx
        shaders/colorChannel.glslfx
        shaders/colorCorrection.glslfx
        shaders/fullscreen.glslfx
        shaders/oitResolveImageShader.glslfx
        shaders/outline.glslfx
        shaders/renderPass.glslfx
        shaders/renderPassColorShader.glslfx
        shaders/renderPassColorAndSelectionShader.glslfx
        shaders/renderPassColorWithOccludedSelectionShader.glslfx
        shaders/renderPassPickingShader.glslfx
        shaders/renderPassOitShader.glslfx
        shaders/renderPassOitOpaqueShader.glslfx
        shaders/renderPassOitVolumeShader.glslfx
        shaders/renderPassShadowShader.glslfx
        shaders/selection.glslfx
        shaders/skydome.glslfx
        shaders/visualize.glslfx
        textures/StinsonBeach.hdr
        textures/StinsonBeach.tex

    DOXYGEN_FILES
        overview.dox
)

if (X11_FOUND OR APPLE)
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
endif()

if (X11_FOUND)
pxr_build_test(testHdxIdRender
    LIBRARIES
        hdx
        hd
        hdSt
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        testenv/testHdxIdRender.cpp
)
pxr_build_test(testHdxPickAndHighlight
    LIBRARIES
        hdx
        hd
        hdSt
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestUtils.cpp
        testenv/testHdxPickAndHighlight.cpp
)
pxr_build_test(testHdxCameraAndLight
    LIBRARIES
        hdx
        hd
        sdf
        glf
        arch
        tf
    CPPFILES
        testenv/testHdxCameraAndLight.cpp
)
pxr_build_test(testHdxPickingWithReprAndRefineChanges
    LIBRARIES
        hdx
        hd
        hdSt
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestUtils.cpp
        testenv/testHdxPickingWithReprAndRefineChanges.cpp
)
pxr_build_test(testHdxUnpickablesAsOccluders
    LIBRARIES
        hdx
        hd
        hdSt
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestUtils.cpp
        testenv/testHdxUnpickablesAsOccluders.cpp
)
pxr_build_test(testHdxPickTarget
    LIBRARIES
        hdx
        hd
        hdSt
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestUtils.cpp
        testenv/testHdxPickTarget.cpp
)
pxr_build_test(testHdxPickResolveMode
    LIBRARIES
        hdx
        hd
        hdSt
        sdf
        glf
        garch
        arch
        tf
    CPPFILES
        unitTestUtils.cpp
        testenv/testHdxPickResolveMode.cpp
)
endif()

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

if (APPLE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on macOS")
    return()
endif()

if (WIN32)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on Windows")
    return()
endif()

#
# Install tests
#
pxr_install_test_dir(
    SRC testenv/testHdxLightAndShadow
    DEST testHdxLightAndShadow
)
pxr_install_test_dir(
    SRC testenv/testHdxPickAndHighlight
    DEST testHdxPickAndHighlight
)
pxr_install_test_dir(
    SRC testenv/testHdxPickTarget
    DEST testHdxPickTarget
)
pxr_install_test_dir(
    SRC testenv/testHdxPickingWithReprAndRefineChanges
    DEST testHdxPickingWithReprAndRefineChanges
)
pxr_install_test_dir(
    SRC testenv/testHdxRenderTask
    DEST testHdxRenderTask
)
pxr_install_test_dir(
    SRC testenv/testHdxUnpickablesAsOccluders
    DEST testHdxUnpickablesAsOccluders
)

#
# Register tests
#
pxr_register_test(testHdxCameraAndLight
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxCameraAndLight"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdxIdRender
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxIdRender --offscreen"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testHdxIdRender_Refined
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxIdRender --offscreen --repr refined --refineLevel 2"
    EXPECTED_RETURN_CODE 0
)
pxr_register_test(testHdxLightAndShadow
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxLightAndShadow"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1.png
        color2.png
        color3.png
        color4.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdxPickAndHighlight
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxPickAndHighlight --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1_unselected.png
        color2_select.png
        color3_select.png
        color4_locate.png
        color5_locate.png
        color6_select_all_instances.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
)
pxr_register_test(testHdxPickResolveMode
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxPickResolveMode --offscreen"
    EXPECTED_RETURN_CODE 0
    ENV
        TF_DEBUG="HDX_SELECTION_SETUP HD_SELECTION_UPDATE"
)
pxr_register_test(testHdxPickTarget
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxPickTarget --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1_unselected.png
        color2_cube0_pick_face.png
        color3_tet1_pick_faces.png
        color4_tet0_pick_edge.png
        color5_cube1_pick_edges.png
        color6_cube1_pick_points.png
        color7_cube1_pick_points_pick_through.png
        color8_points_with_color.png
        color9_cube0_wire_pick_face.png
        color10_tet1_pick_prims.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    ENV
        TF_DEBUG="HDX_SELECTION_SETUP HD_SELECTION_UPDATE"
)
pxr_register_test(testHdxPickingWithReprAndRefineChanges
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxPickingWithReprAndRefineChanges --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1_unselected.png
        color2_refine_wont_change_cube1.png
        color3_repr_change_cube2.png
        color4_repr_and_refine_change_cube1.png
        color5_refine_change_cube2.png
        color6_unselected.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
)
pxr_register_test(testHdxRenderTask
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxRenderTask"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1.png
        color2.png
        color3.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)
pxr_register_test(testHdxUnpickablesAsOccluders
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxUnpickablesAsOccluders --offscreen"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1_unselected.png
        color2_cube0_pickable.png
        color3_cube0_unpickable.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
)

# Certain tests use plugins that require shared libraries.
if (TARGET shared_libs)

pxr_build_test(testHdxDrawTarget
    LIBRARIES
        hdx
        hd
        sdf
        glf
        arch
        tf
    CPPFILES
        testenv/testHdxDrawTarget.cpp
)
pxr_install_test_dir(
    SRC testenv/testHdxDrawTarget
    DEST testHdxDrawTarget
)
pxr_register_test(testHdxDrawTarget
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdxDrawTarget"
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        color1.png
    FAIL 1
    FAIL_PERCENT 1
    PERCEPTUAL
    ENV
        TF_DEBUG=HD_SAFE_MODE
)

endif() # TARGET shared_libs
