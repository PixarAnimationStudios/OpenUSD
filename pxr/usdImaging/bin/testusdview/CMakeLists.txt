set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdviewq)

if (NOT PXR_BUILD_USDVIEW)
    return()
endif()

pxr_python_bin(testusdview
    DEPENDENCIES
        usdviewq
)

if (${PXR_HEADLESS_TEST_MODE})
    message(STATUS "Skipping ${PXR_PACKAGE} tests because PXR_HEADLESS_TEST_MODE is ON")
    return()
endif()

# testusdview imaging tests require python and plugin mechanism which we do not
# support for static build configurations due to potential conflict in client
# applications linking against static build USD
if (NOT BUILD_SHARED_LIBS)
    message(STATUS "Skipping ${PXR_PACKAGE} tests on static build configuration")
    return()
endif()

pxr_install_test_dir(
    SRC testenv/testUsdviewAlive
    DEST testUsdviewAlive
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDetachedLayers
    DEST testUsdviewDetachedLayers1
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDetachedLayers
    DEST testUsdviewDetachedLayers2
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDetachedLayers
    DEST testUsdviewDetachedLayers3
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDetachedLayers
    DEST testUsdviewDetachedLayers4
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments1
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments2
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments3
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments4
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments5
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments6
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFileArguments
    DEST testUsdviewFileArguments7
)

pxr_install_test_dir(
    SRC testenv/testUsdviewWrapper
    DEST testUsdviewWrapper1
)

pxr_install_test_dir(
    SRC testenv/testUsdviewWrapper
    DEST testUsdviewWrapper2
)

pxr_install_test_dir(
    SRC testenv/testUsdviewWrapper
    DEST testUsdviewWrapper3
)

pxr_install_test_dir(
    SRC testenv/testUsdviewWrapper
    DEST testUsdviewWrapper4
)

pxr_install_test_dir(
    SRC testenv/testUsdviewWrapper
    DEST testUsdviewWrapper5
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPrimPathBar
    DEST testUsdviewPrimPathBar
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPrimSearch
    DEST testUsdviewPrimSearch
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPropertySearch
    DEST testUsdviewPropertySearch
)

pxr_install_test_dir(
    SRC testenv/testUsdviewMetadatatabSelect
    DEST testUsdviewMetadatatabSelect
)

pxr_install_test_dir(
    SRC testenv/testUsdviewTimeSamples
    DEST testUsdviewTimeSamples
)

pxr_install_test_dir(
    SRC testenv/testUsdviewNavigationKeys
    DEST testUsdviewNavigationKeys
)

pxr_install_test_dir(
    SRC testenv/testUsdviewReloadReopen
    DEST testUsdviewReloadReopen
)

pxr_install_test_dir(
    SRC testenv/testUsdviewNoPlugins
    DEST testUsdviewNoPlugins
)

pxr_install_test_dir(
    SRC testenv/testUsdviewInterpreterNoRender
    DEST testUsdviewInterpreterNoRender
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLoadPlugins
    DEST testUsdviewLoadPlugins_alive
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLoadPlugins
    DEST testUsdviewLoadPlugins_multipleContainers
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLoadPlugins
    DEST testUsdviewLoadPlugins_missingContainer
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLoadPlugins
    DEST testUsdviewLoadPlugins_duplicateCommand
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLoadPlugins
    DEST testUsdviewLoadPlugins_chained
)

pxr_install_test_dir(
    SRC testenv/testUsdviewUpAxis
    DEST testUsdviewUpAxis_zUp
)

pxr_install_test_dir(
    SRC testenv/testUsdviewUpAxis
    DEST testUsdviewUpAxis_yUp
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDeactivate
    DEST testUsdviewDeactivate
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDefaultFontFamily
    DEST testUsdviewDefaultFontFamily
)

pxr_install_test_dir(
    SRC testenv/testUsdviewSkinning
    DEST testUsdviewCPUSkinning
)

pxr_install_test_dir(
    SRC testenv/testUsdviewSkinning
    DEST testUsdviewGPUSkinning
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPrimTreeExpansion
    DEST testUsdviewPrimTreeExpansion
)

pxr_install_test_dir(
    SRC testenv/testUsdviewInfGeom
    DEST testUsdviewInfGeom
)

pxr_install_test_dir(
    SRC testenv/testUsdviewShaderEdits
    DEST testUsdviewShaderEdits
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPrimvarEdits
    DEST testUsdviewPrimvarEdits
)

pxr_install_test_dir(
    SRC testenv/testUsdviewInstancingEdits
    DEST testUsdviewInstancingEdits
)

pxr_install_test_dir(
    SRC testenv/testUsdviewAnimatedBounds
    DEST testUsdviewAnimatedBounds
)

pxr_install_test_dir(
    SRC testenv/testUsdviewMeshBounds
    DEST testUsdviewMeshBounds
)

pxr_install_test_dir(
    SRC testenv/testUsdviewColorManagement
    DEST testUsdviewColorManagement
)

pxr_install_test_dir(
    SRC testenv/testUsdviewMaterialEdits
    DEST testUsdviewMaterialEdits
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFreeCamera
    DEST testUsdviewFreeCamera
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFreeCameraAspect
    DEST testUsdviewFreeCameraAspect
)

pxr_install_test_dir(
    SRC testenv/testUsdviewSelectionHighlighting
    DEST testUsdviewSelectionHighlighting
)

pxr_install_test_dir(
    SRC testenv/testUsdviewRenderMode
    DEST testUsdviewRenderMode
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLights
    DEST testUsdviewLights
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLights
    DEST testUsdviewLights_yup
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLightVisibility
    DEST testUsdviewLightVisibility
)

pxr_install_test_dir(
    SRC testenv/testUsdviewLightVisibility
    DEST testUsdviewLightVisibility_varying
)

pxr_install_test_dir(
    SRC testenv/testUsdviewBackgroundColor
    DEST testUsdviewBackgroundColor
)

pxr_install_test_dir(
    SRC testenv/testUsdviewComplexity
    DEST testUsdviewComplexity
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFieldOfView
    DEST testUsdviewFieldOfView
)

pxr_install_test_dir(
    SRC testenv/testUsdviewClippingPlanes
    DEST testUsdviewClippingPlanes
)

pxr_install_test_dir(
    SRC testenv/testUsdviewDefaultMaterial
    DEST testUsdviewDefaultMaterial
)

pxr_install_test_dir(
    SRC testenv/testUsdviewCameraMaskMode
    DEST testUsdviewCameraMaskMode
)

pxr_install_test_dir(
    SRC testenv/testUsdviewFrameSelection
    DEST testUsdviewFrameSelection
)

pxr_install_test_dir(
    SRC testenv/testUsdviewOIT
    DEST testUsdviewOIT
)

pxr_install_test_dir(
    SRC testenv/testUsdviewAdditive
    DEST testUsdviewAdditive
)

pxr_install_test_dir(
    SRC testenv/testUsdviewVariantSelection
    DEST testUsdviewVariantSelection
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPointWidths
    DEST testUsdviewPointWidths
)

pxr_install_test_dir(
    SRC testenv/testUsdviewPointInstancerVariant
    DEST testUsdviewPointInstancerVariant
)


pxr_register_test(testUsdviewAlive
    PRE_COMMAND "${PYTHON_EXECUTABLE} testUsdviewAliveSetup.py"
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testAlive.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_1.py --norender --detachLayers root.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_2.py --norender --detachLayersInclude 'foo,with spaces' root.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_3.py --norender --detachLayers --detachLayersExclude 'foo,with spaces' root.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_4.py --norender root.usd"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewFileArguments1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usda"
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usd"
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usdc"
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py test.py"
    STDERR_REDIRECT py_test_out
    DIFF_COMPARE py_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py invalidSyntax.usda"
    CLEAN_OUTPUT "(?:[A-Za-z]:)?/(?!refSphere2).*/"
    STDERR_REDIRECT invalidSyntax_test_out
    DIFF_COMPARE invalidSyntax_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments6
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py test.txt"
    STDERR_REDIRECT txt_test_out
    DIFF_COMPARE txt_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments7
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py missing"
    STDERR_REDIRECT missing_test_out
    DIFF_COMPARE missing_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback.py"
    EXPECTED_RETURN_CODE 2
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_1.py test.usda"
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_2.py test.usda"
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_3.py test.usda"
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview test.usda"
    EXPECTED_RETURN_CODE 2
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewPrimPathBar
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimPathBar.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPrimSearch
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimSearch.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPropertySearch
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPropertySearch.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewMetadatatabSelect
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewMetadatatabSelect.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewTimeSamples
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewTimeSamples.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewNavigationKeys
    RUN_SERIAL
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewNavigationKeys.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewNoPlugins
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewNoPlugins.py --noplugins test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewInterpreterNoRender
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInterpreterNoRender.py --norender test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_alive
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_alive.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_multipleContainers
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_multipleContainers.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_missingContainer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_missingContainer.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_duplicateCommand
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_duplicateCommand.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_chained
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_chained.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewUpAxis_yUp
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewUpAxis_yUp.py testYUp.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDefaultFontFamily
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewDefaultFontFamily.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewInstancingEdits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInstancingEdits.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPrimTreeExpansion
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimTreeExpansion.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewAnimatedBounds
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewAnimatedBounds.py test.usda"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewMeshBounds
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewMeshBounds.py meshBounds.usda"
    EXPECTED_RETURN_CODE 0
)

if (APPLE)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on macOS")
    return()
endif()

if (WIN32)
    message(STATUS "Skipping ${PXR_PACKAGE} tests because they are currently unsupported on Windows")
    return()
endif()

pxr_register_test(testUsdviewFreeCamera
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewFreeCamera.py Block.usda"
    IMAGE_DIFF_COMPARE
        start.png
        block_F.png
        block_L.png
        block_B.png
        block_R.png
        phi45.png
        adjustDist_05.png
        adjustDist_05_20.png
        rot_truck.png
        truck_-1_-1.png
        truck_1_1.png
        end.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewFreeCameraAspect
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewFreeCameraAspect.py test.usda"
    IMAGE_DIFF_COMPARE
        aspect185.png
        aspect239.png
        aspect05.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewSelectionHighlighting
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSelectionHighlighting.py test.usda"
    IMAGE_DIFF_COMPARE
        sel_highlight.png
        sel_highlight_none.png
        sel_highlight_double.png
        sel_highlight_instance.png
        sel_highlight_color.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewRenderMode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewRenderMode.py test.usda"
    IMAGE_DIFF_COMPARE
        render_mode_wireframe.png
        render_mode_wireframe_on_surface.png
        render_mode_smooth.png
        render_mode_flat.png
        render_mode_points.png
        geom_only.png
        geom_smooth.png
        geom_flat.png
        render_mode_hidden_surface_wireframe.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLights
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLights.py test.usda"
    IMAGE_DIFF_COMPARE
        camera.png
        noLights.png
        dome.png
        bothLights.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLights_yup
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLightsYup.py testYup.usda"
    IMAGE_DIFF_COMPARE
        domeYup.png
        bothLightsYup.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLightVisibility
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLightVisibility.py test.usda"
    IMAGE_DIFF_COMPARE
        singleInvisible1.png
        singleInvisible2.png
        allVisible.png
        allInvisible.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLightVisibility_varying
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewVaryLightVisibility.py testVarying.usda"
    IMAGE_DIFF_COMPARE
        visible.png
        invisible.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewBackgroundColor
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewBackgroundColor.py test.usda"
    IMAGE_DIFF_COMPARE
        black.png
        grey_dark.png
        grey_light.png
        white.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewComplexity
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewComplexity.py test.usda"
    IMAGE_DIFF_COMPARE
        low.png
        medium.png
        high.png
        very_high.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewFieldOfView
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewFieldOfView.py test.usda"
    IMAGE_DIFF_COMPARE
        fov45.png
        fov60.png
        fov90.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewClippingPlanes
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewClippingPlanes.py test.usda"
    IMAGE_DIFF_COMPARE
        no_override.png
        override_near.png
        override_far.png
        override_both.png
        before_recompute.png
        after_recompute.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDefaultMaterial
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewDefaultMaterial.py test.usda"
    IMAGE_DIFF_COMPARE
        none.png
        ambient.png
        specular.png
        both.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewCameraMaskMode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --camera /camera --renderer GL --testScript testUsdviewCameraMaskMode.py test.usda"
    IMAGE_DIFF_COMPARE
        none_gl.png
        outline_gl.png
        partial_gl.png
        full_gl.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

if (EMBREE_FOUND AND ${PXR_BUILD_EMBREE_PLUGIN})
    pxr_register_test(testUsdviewCameraMaskMode_Embree
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --camera /camera --renderer Embree --testScript testUsdviewCameraMaskMode.py test.usda"
        IMAGE_DIFF_COMPARE
            none_embree.png
            outline_embree.png
            partial_embree.png
            full_embree.png
        FAIL 0.05
        FAIL_PERCENT 0.03
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
        TESTENV testUsdviewCameraMaskMode
    )
endif()

pxr_register_test(testUsdviewFrameSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewFrameSelection.py test.usda"
    IMAGE_DIFF_COMPARE
        framed.png
        toggleToStart.png
        rotatedStart.png
        toggleToFramed.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewReloadReopen
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewReloadReopen.py test.usda"
    IMAGE_DIFF_COMPARE
        coloredAndFramed.png
        reloaded.png
        reopened.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewMaterialEdits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewMaterialEdits.py test.usda"
    IMAGE_DIFF_COMPARE
        1.png
        2.png
        3.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewUpAxis_zUp
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewUpAxis_zUp.py testZUp.usda"
    IMAGE_DIFF_COMPARE
        zUp.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDeactivate
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewDeactivate.py test.usda"
    IMAGE_DIFF_COMPARE
        singleDeactivate.png
        parentDeactivate.png
        parentChildDeactivate1.png
        parentChildDeactivate2.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewCPUSkinning
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSkinning.py arm.usda"
    IMAGE_DIFF_COMPARE
        change_complexity.png
        vis_frame_4.png
        invis_frame_4.png
        vis_frame_8.png
        pre_skinned_prim_resync_frame_2.png
        post_skinned_prim_resync_frame_2.png
        pre_skel_resync_frame_6.png
        post_skel_resync_frame_6.png
    FAIL 0.05
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=1
)

pxr_register_test(testUsdviewGPUSkinning
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSkinning.py arm.usda"
    IMAGE_DIFF_COMPARE
        change_complexity.png
        vis_frame_4.png
        invis_frame_4.png
        vis_frame_8.png
        pre_skinned_prim_resync_frame_2.png
        post_skinned_prim_resync_frame_2.png
        pre_skel_resync_frame_6.png
        post_skel_resync_frame_6.png
    FAIL 0.05
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDSKELIMAGING_FORCE_CPU_COMPUTE=0
)

pxr_register_test(testUsdviewInfGeom
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInfGeom.py infGeom.usda"
    IMAGE_DIFF_COMPARE
        infGeom.png
        fixedGeom.png
    FAIL 0.05
    FAIL_PERCENT 0.06
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

# The following test is disabled since it seems to require Renderman from
# the original testSpecs.xml
#pxr_register_test(testUsdviewShaderEdits
#    PYTHON
#    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewShaderEdits.py test.usda"
#    IMAGE_DIFF_COMPARE
#        0.png
#    FAIL 1
#    FAIL_PERCENT 0.0002
#    PERCEPTUAL
#    EXPECTED_RETURN_CODE 0
#)

pxr_register_test(testUsdviewPrimvarEdits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimvarEdits.py primvars.usda"
    IMAGE_DIFF_COMPARE
        start.png
        add_fallback_primvar_smooth.png
        add_fallback_primvar_flat.png
        remove_fallback_primvar_flat.png
        remove_fallback_primvar_wireOnSurf.png
        add_unused_primvars.png
        remove_unused_primvars.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewOIT
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewOIT.py test.usda"
    IMAGE_DIFF_COMPARE
        oit.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewAdditive
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewAdditive.py test.usda"
    IMAGE_DIFF_COMPARE
        additive.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewVariantSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewVariantSelection.py test.usda"
    IMAGE_DIFF_COMPARE
        capsule1.png
        cone1.png
        cube1.png
        cylinder1.png
        capsule2.png
        cone2.png
        cube2.png
        cylinder2.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPointWidths
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPointWidths.py points.usda"
    IMAGE_DIFF_COMPARE
        start.png
        set_point_widths_025.png
        set_point_widths_050.png
        set_point_widths_primvar_075.png
        end.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPointInstancerVariant
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPointInstancerVariant.py test.usda"
    IMAGE_DIFF_COMPARE
        variantB.png
        variantA.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

if(OPENCOLORIO_FOUND AND ${PXR_BUILD_OPENCOLORIO_PLUGIN})
    pxr_register_test(testUsdviewColorManagement
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewColorManagement.py test.usda"
        IMAGE_DIFF_COMPARE
            colorCorrectionOCIO_g22.png
            colorCorrectionOCIO_linear.png
            colorCorrectionSRGB.png
            colorCorrectionDisabled.png
        EXPECTED_RETURN_CODE 0
        ENV
            OCIO=test.ocio
    )
endif()
