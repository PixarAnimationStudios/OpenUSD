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

pxr_register_test(testUsdviewAlive
    PRE_COMMAND "${PYTHON_EXECUTABLE} testUsdviewAliveSetup.py"
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testAlive.py test.usda"
    TESTENV testenv/testUsdviewAlive
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_1.py --norender --detachLayers root.usd"
    TESTENV testenv/testUsdviewDetachedLayers
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_2.py --norender --detachLayersInclude 'foo,with spaces' root.usd"
    TESTENV testenv/testUsdviewDetachedLayers
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_3.py --norender --detachLayers --detachLayersExclude 'foo,with spaces' root.usd"
    TESTENV testenv/testUsdviewDetachedLayers
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDetachedLayers4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testDetachedLayers_4.py --norender root.usd"
    TESTENV testenv/testUsdviewDetachedLayers
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewFileArguments1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usda"
    TESTENV testenv/testUsdviewFileArguments
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usd"
    TESTENV testenv/testUsdviewFileArguments
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testValidFileArg.py test.usdc"
    TESTENV testenv/testUsdviewFileArguments
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py test.py"
    TESTENV testenv/testUsdviewFileArguments
    STDERR_REDIRECT py_test_out
    DIFF_COMPARE py_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py invalidSyntax.usda"
    TESTENV testenv/testUsdviewFileArguments
    CLEAN_OUTPUT "(?:[A-Za-z]:)?/.*(?=invalidSyntax)"
    STDERR_REDIRECT invalidSyntax_test_out
    DIFF_COMPARE invalidSyntax_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments6
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py test.txt"
    TESTENV testenv/testUsdviewFileArguments
    STDERR_REDIRECT txt_test_out
    DIFF_COMPARE txt_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewFileArguments7
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testInvalidFileArg.py missing"
    TESTENV testenv/testUsdviewFileArguments
    STDERR_REDIRECT missing_test_out
    DIFF_COMPARE missing_test_out
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper0
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback.py"
    TESTENV testenv/testUsdviewWrapper
    STDOUT_REDIRECT valid_output_no_file
    DIFF_COMPARE valid_output_no_file
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback.py test.usda"
    TESTENV testenv/testUsdviewWrapper
    STDOUT_REDIRECT valid_output
    DIFF_COMPARE valid_output
    EXPECTED_RETURN_CODE 0
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_1.py test.usda"
    TESTENV testenv/testUsdviewWrapper
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_2.py test.usda"
    TESTENV testenv/testUsdviewWrapper
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testCallback_Invalid_3.py test.usda"
    TESTENV testenv/testUsdviewWrapper
    EXPECTED_RETURN_CODE 1
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewWrapper5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview test.usda"
    TESTENV testenv/testUsdviewWrapper
    EXPECTED_RETURN_CODE 2
    ENV
        PXR_USDVIEW_SUPPRESS_STATE_SAVING=1
)

pxr_register_test(testUsdviewPrimPathBar
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimPathBar.py test.usda"
    TESTENV testenv/testUsdviewPrimPathBar
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPrimSearch
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimSearch.py test.usda"
    TESTENV testenv/testUsdviewPrimSearch
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPropertySearch
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPropertySearch.py test.usda"
    TESTENV testenv/testUsdviewPropertySearch
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewMetadatatabSelect
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewMetadatatabSelect.py test.usda"
    TESTENV testenv/testUsdviewMetadatatabSelect
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewTimeSamples
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewTimeSamples.py test.usda"
    TESTENV testenv/testUsdviewTimeSamples
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewNavigationKeys
    RUN_SERIAL
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewNavigationKeys.py test.usda"
    TESTENV testenv/testUsdviewNavigationKeys
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewNoPlugins
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewNoPlugins.py --noplugins test.usda"
    TESTENV testenv/testUsdviewNoPlugins
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewInterpreterNoRender
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInterpreterNoRender.py --norender test.usda"
    TESTENV testenv/testUsdviewInterpreterNoRender
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_alive
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_alive.py test.usda"
    TESTENV testenv/testUsdviewLoadPlugins
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_multipleContainers
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_multipleContainers.py test.usda"
    TESTENV testenv/testUsdviewLoadPlugins
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_missingContainer
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_missingContainer.py test.usda"
    TESTENV testenv/testUsdviewLoadPlugins
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_duplicateCommand
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_duplicateCommand.py test.usda"
    TESTENV testenv/testUsdviewLoadPlugins
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewLoadPlugins_chained
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLoadPlugins_chained.py test.usda"
    TESTENV testenv/testUsdviewLoadPlugins
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewUpAxis_yUp
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewUpAxis_yUp.py testYUp.usda"
    TESTENV testenv/testUsdviewUpAxis
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewDefaultFontFamily
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewDefaultFontFamily.py test.usda"
    TESTENV testenv/testUsdviewDefaultFontFamily
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewPrimTreeExpansion
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewPrimTreeExpansion.py test.usda"
    TESTENV testenv/testUsdviewPrimTreeExpansion
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewAnimatedBounds
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewAnimatedBounds.py test.usda"
    TESTENV testenv/testUsdviewAnimatedBounds
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewMeshBounds
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewMeshBounds.py meshBounds.usda"
    TESTENV testenv/testUsdviewMeshBounds
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
    TESTENV testenv/testUsdviewFreeCamera
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
    TESTENV testenv/testUsdviewFreeCameraAspect
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
    TESTENV testenv/testUsdviewSelectionHighlighting
    IMAGE_DIFF_COMPARE
        sel_highlight.png
        sel_highlight_none.png
        sel_highlight_double.png
        # UsdImaging 2.0 does not yet support selection highlighting of individual instances.
        # sel_highlight_instance.png
        sel_highlight_color.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewSelectionHighlighting_Delegate
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSelectionHighlighting.py test.usda"
    TESTENV testenv/testUsdviewSelectionHighlighting
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
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=0
)

pxr_register_test(testUsdviewRenderMode
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewRenderMode.py test.usda"
    TESTENV testenv/testUsdviewRenderMode
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
    TESTENV testenv/testUsdviewLights
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
    TESTENV testenv/testUsdviewLights
    IMAGE_DIFF_COMPARE
        domeYup.png
        bothLightsYup.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewSceneLights
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSceneLights.py test.usda"
    TESTENV testenv/testUsdviewSceneLights
    IMAGE_DIFF_COMPARE
        camera_noScene_Storm.png
        camera_scene_Storm.png
        domeVis_noScene_Storm.png
        domeVis_scene_Storm.png
        domeInvis_noScene_Storm.png
        domeInvis_scene_Storm.png
        sceneLights_Storm.png
        sceneLights_edit_Storm.png
        noLights_Storm.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)

pxr_register_test(testUsdviewSceneLights_Delegate
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSceneLights.py test.usda"
    TESTENV testenv/testUsdviewSceneLights
    IMAGE_DIFF_COMPARE
        camera_noScene_Storm.png
        camera_scene_Storm.png
        domeVis_noScene_Storm.png
        domeVis_scene_Storm.png
        domeInvis_noScene_Storm.png
        domeInvis_scene_Storm.png
        sceneLights_Storm.png
        sceneLights_edit_Storm.png
        noLights_Storm.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=0
)


pxr_register_test(testUsdviewSceneMaterials
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewSceneMaterials.py test.usda"
    TESTENV testenv/testUsdviewSceneMaterials
    IMAGE_DIFF_COMPARE
        enabledSceneMaterials_Storm.png
        disabledSceneMaterials_Storm.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)

pxr_register_test(testUsdviewLightVisibility
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewLightVisibility.py test.usda"
    TESTENV testenv/testUsdviewLightVisibility
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
    TESTENV testenv/testUsdviewLightVisibility
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
    TESTENV testenv/testUsdviewBackgroundColor
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
    TESTENV testenv/testUsdviewComplexity
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
    TESTENV testenv/testUsdviewFieldOfView
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
    TESTENV testenv/testUsdviewClippingPlanes
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
    TESTENV testenv/testUsdviewDefaultMaterial
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
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --camera /camera --renderer Storm --testScript testUsdviewCameraMaskMode.py test.usda"
    TESTENV testenv/testUsdviewCameraMaskMode
    IMAGE_DIFF_COMPARE
        none_storm.png
        outline_storm.png
        partial_storm.png
        full_storm.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

if (EMBREE_FOUND AND ${PXR_BUILD_EMBREE_PLUGIN})
    pxr_register_test(testUsdviewCameraMaskMode_Embree
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --camera /camera --renderer Embree --testScript testUsdviewCameraMaskMode.py test.usda"
        TESTENV testenv/testUsdviewCameraMaskMode
        IMAGE_DIFF_COMPARE
            none_embree.png
            outline_embree.png
            partial_embree.png
            full_embree.png
        FAIL 0.05
        FAIL_PERCENT 0.03
        PERCEPTUAL
        EXPECTED_RETURN_CODE 0
    )
endif()

pxr_register_test(testUsdviewFrameSelection
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewFrameSelection.py test.usda"
    TESTENV testenv/testUsdviewFrameSelection
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
    TESTENV testenv/testUsdviewReloadReopen
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
    TESTENV testenv/testUsdviewMaterialEdits
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
    TESTENV testenv/testUsdviewUpAxis
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
    TESTENV testenv/testUsdviewDeactivate
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
    TESTENV testenv/testUsdviewSkinning
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
    TESTENV testenv/testUsdviewSkinning
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
    TESTENV testenv/testUsdviewInfGeom
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
#    TESTENV testenv/testUsdviewShaderEdits
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
    TESTENV testenv/testUsdviewPrimvarEdits
    IMAGE_DIFF_COMPARE
        start.png
        add_fallback_primvar_smooth.png
        add_fallback_primvar_flat.png
        remove_fallback_primvar_flat.png
        remove_fallback_primvar_wireOnSurf.png
        add_unused_primvars.png
        remove_unused_primvars.png
        add_primvars_for_varname_edit.png
        primvar_reader_varname_edit.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewOIT
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewOIT.py test.usda"
    TESTENV testenv/testUsdviewOIT
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
    TESTENV testenv/testUsdviewAdditive
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
    TESTENV testenv/testUsdviewVariantSelection
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
    TESTENV testenv/testUsdviewPointWidths
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
    TESTENV testenv/testUsdviewPointInstancerVariant
    IMAGE_DIFF_COMPARE
        variantB.png
        variantA.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdviewExpressionVariableEdits_Delegate
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewExpressionVariableEdits.py test.usda"
    TESTENV testenv/testUsdviewExpressionVariableEdits
    IMAGE_DIFF_COMPARE
        start.png
        step_1.png
        step_2.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=0
)

pxr_register_test(testUsdviewExpressionVariableEdits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewExpressionVariableEdits.py test.usda"
    TESTENV testenv/testUsdviewExpressionVariableEdits
    IMAGE_DIFF_COMPARE
        start.png
        step_1.png
        step_2.png
    FAIL 0.05
    FAIL_PERCENT 0.03
    PERCEPTUAL
    EXPECTED_RETURN_CODE 0
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=1
)

pxr_register_test(testUsdviewInstancingEdits
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInstancingEdits.py test.usda"
    TESTENV testenv/testUsdviewInstancingEdits
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        instanceWithParent.png
        instanceWithParentDeactivated.png
        completeResyncWithNativeInstances1.png
        completeResyncWithNativeInstances2.png
    FAIL 0.04
    FAIL_PERCENT 0.1
    WARN 0.02
    WARN_PERCENT 0.2
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=false
)

pxr_register_test(testUsdviewInstancingEdits_SceneIndex
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewInstancingEdits.py test.usda"
    TESTENV testenv/testUsdviewInstancingEdits
    EXPECTED_RETURN_CODE 0
    IMAGE_DIFF_COMPARE
        instanceWithParent.png
        instanceWithParentDeactivated.png
        completeResyncWithNativeInstances1.png
        completeResyncWithNativeInstances2.png
        instancerInvisible.png
        instancerVisible.png
    FAIL 0.04
    FAIL_PERCENT 0.1
    WARN 0.02
    WARN_PERCENT 0.2
    ENV
        USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
)

if (EMBREE_FOUND AND ${PXR_BUILD_EMBREE_PLUGIN})
    pxr_register_test(testUsdviewInstancingEdits_SceneIndex_Embree
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --renderer Embree --testScript testUsdviewInstancingEditsEmbree.py test.usda"
        TESTENV testenv/testUsdviewInstancingEdits
        EXPECTED_RETURN_CODE 0
        IMAGE_DIFF_COMPARE
            instancerInvisibleEmbree.png
            instancerVisibleEmbree.png
        FAIL 0.04
        FAIL_PERCENT 0.1
        WARN 0.02
        WARN_PERCENT 0.2
        ENV
            USDIMAGINGGL_ENGINE_ENABLE_SCENE_INDEX=true
    )
endif()

if(OPENCOLORIO_FOUND AND ${PXR_BUILD_OPENCOLORIO_PLUGIN})
    pxr_register_test(testUsdviewColorManagement
        PYTHON
        COMMAND "${CMAKE_INSTALL_PREFIX}/bin/testusdview --testScript testUsdviewColorManagement.py test.usda"
        TESTENV testenv/testUsdviewColorManagement
        IMAGE_DIFF_COMPARE
            colorCorrectionOCIO_g22.png
            colorCorrectionOCIO_linear.png
            colorCorrectionSRGB.png
            colorCorrectionDisabled.png
        FAIL 0.02
        FAIL_PERCENT 0.01
        WARN 0.01
        WARN_PERCENT 0.005
        EXPECTED_RETURN_CODE 0
        ENV
            OCIO=test.ocio
    )
endif()
