set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdNoodles)

if (NOT PXR_BUILD_USDVIEW)
    return()
endif()

# usdNoodles requires the external noodles core library.
# Build with --build-noodles (default) to enable it.
if(NOT DEFINED NOODLES_LIBRARY)
    message(STATUS "Skipping usdNoodles — NOODLES_LIBRARY not set (use --build-noodles)")
    return()
endif()

# Noodles core library (built as external dependency)
if(DEFINED NOODLES_INCLUDE_DIR)
    include_directories(${NOODLES_INCLUDE_DIR})
endif()
# core/api.h checks NOODLES_EXPORTS for dllexport; the pxr_library
# macro only defines USDNOODLES_EXPORTS, so we add it explicitly.
add_definitions(-DNOODLES_EXPORTS)

pxr_library(usdNoodles
    LIBRARIES
        garch
        glf
        hio
        js
        tf
        gf
        vt
        sdf
        usd
        boost
        python
        ${NOODLES_LIBRARY}

    PYMODULE_CPPFILES
        bindings/module.cpp
        bindings/wrapGraphLayout.cpp
        bindings/wrapGraphRenderer.cpp
        bindings/wrapLinkGeometry.cpp
        bindings/wrapNodeData.cpp
        bindings/wrapRenderManagers.cpp
        bindings/wrapRendering.cpp
        bindings/wrapSpatial.cpp
        bindings/wrapTextLayout.cpp
        bindings/wrapUndo.cpp
        bindings/wrapVertexCache.cpp

    PYMODULE_FILES
        __init__.py
        _qt_editor.py
        _schema_pin_names.py
        assets/__init__.py
        constants.py
        core/__init__.py
        graphView.py
        iconRenderer.py
        linkRenderer.py
        models.py
        nodeFactory.py
        nodeGraph.py
        nodeGraphBlueprint.py
        nodeGraphBpContainer.py
        nodeGraphJson.py
        nodeGraphStage.py
        nodeLibraries.py
        nodeLibs/__init__.py
        nodeLibs/registry.py
        nodeLibs/usdPrimLibrary.py
        nodeLibs/usdPrimRegistry.py
        nodeLibs/usdTypedPrimBase.py
        noodlesConfig.py
        noodlesPreferences.py
        noodlesSettings.py
        pinUtils.py
        primAuthoring.py
        render/__init__.py
        spatial/__init__.py
        testenv/fixtures/external_plugin/test_external_library.py
        testenv/headless/__init__.py
        testenv/headless/base.py
        testenv/headless/harness.py
        testenv/headless/helpers.py
        testenv/headless/screenshot.py
        testenv/testUsdNoodlesActiveFlowHighlight.py
        testenv/testUsdNoodlesAddRemoveNode.py
        testenv/testUsdNoodlesAutoLayout.py
        testenv/testUsdNoodlesBackdrop.py
        testenv/testUsdNoodlesBindingGuards.py
        testenv/testUsdNoodlesDeleteConnection.py
        testenv/testUsdNoodlesDisconnectDrag.py
        testenv/testUsdNoodlesDisplayColor.py
        testenv/testUsdNoodlesDualPins.py
        testenv/testUsdNoodlesEditTarget.py
        testenv/testUsdNoodlesExpansionState.py
        testenv/testUsdNoodlesExternalPluginE2E.py
        testenv/testUsdNoodlesGenericRelationshipFix.py
        testenv/testUsdNoodlesGraphViewMarqueeAndHover.py
        testenv/testUsdNoodlesGraphViewProjection.py
        testenv/testUsdNoodlesGraphViewRelationships.py
        testenv/testUsdNoodlesGraphViewShortcut.py
        testenv/testUsdNoodlesGridPlaceAndOverlap.py
        testenv/testUsdNoodlesHeadlessCreateMoveUndo.py
        testenv/testUsdNoodlesHeadlessDeleteIncidentLinks.py
        testenv/testUsdNoodlesHeadlessRemoveCreatesDangling.py
        testenv/testUsdNoodlesHeadlessSmoke.py
        testenv/testUsdNoodlesHotkeyPopup.py
        testenv/testUsdNoodlesIconRenderer.py
        testenv/testUsdNoodlesImportDiag.py
        testenv/testUsdNoodlesKeyBindingDialog.py
        testenv/testUsdNoodlesLayoutBindingsAndReadback.py
        testenv/testUsdNoodlesLinkGeometry.py
        testenv/testUsdNoodlesLinkRenderer.py
        testenv/testUsdNoodlesLinkRendererCacheKey.py
        testenv/testUsdNoodlesLinkSyncGatingExtras.py
        testenv/testUsdNoodlesMacMenuBar.py
        testenv/testUsdNoodlesNodeFactoryIcon.py
        testenv/testUsdNoodlesNodeGraph.py
        testenv/testUsdNoodlesNodeLibraries.py
        testenv/testUsdNoodlesNodeLibraryRegistry.py
        testenv/testUsdNoodlesPinDragInteractions.py
        testenv/testUsdNoodlesPinGroups.py
        testenv/testUsdNoodlesPrimRegistry.py
        testenv/testUsdNoodlesPropertyTooltip.py
        testenv/testUsdNoodlesRenderManagerGuards.py
        testenv/testUsdNoodlesRowDrift.py
        testenv/testUsdNoodlesSaveStage.py
        testenv/testUsdNoodlesSchemaAwarePinDiscovery.py
        testenv/testUsdNoodlesSchemaTypeName.py
        testenv/testUsdNoodlesSimple.py
        testenv/testUsdNoodlesSpatialIndex.py
        testenv/testUsdNoodlesStageReload.py
        testenv/testUsdNoodlesTextLayout.py
        testenv/testUsdNoodlesTextRenderer.py
        testenv/testUsdNoodlesUnifiedCrossSideGroup.py
        testenv/testUsdNoodlesVertexGeneration.py
        testenv/test_noodlesSettings.py
        textRenderer.py
        usdNoticeHandler.py
        utils.py
        widgets/__init__.py
        widgets/groupSticker.py
        widgets/hotkeyPopup.py
        widgets/keyBindingDialog.py
        widgets/libraryToggleMenu.py
        widgets/minimap.py
        widgets/nodeCreationHotbox.py
        widgets/selectableList.py
        widgets/statusOverlay.py
        widgets/textInputWidget.py
        widgets/widgetBase.py

    RESOURCE_FILES
        assets/body_graph.json
        assets/body_graph2.json
        assets/fonts/Poppins-Light.json
        assets/fonts/Poppins-Light.png
        assets/fonts/Poppins-Medium.json
        assets/fonts/Poppins-Medium.png
        assets/fonts/Poppins-Regular.json
        assets/fonts/Poppins-Regular.png
        assets/fonts/baskerville.json
        assets/fonts/baskerville.png
        assets/icons/box.png
        assets/icons/relationship-arrows.png
        assets/icons/row-minus.png
        assets/my_nodes.json
        assets/shaders/icon_frag.glsl
        assets/shaders/icon_vert.glsl
        assets/shaders/link_poly_frag.glsl
        assets/shaders/link_poly_vert.glsl
        assets/shaders/msdf_frag.glsl
        assets/shaders/msdf_vert.glsl
        assets/shaders/node_frag.glsl
        assets/shaders/node_vert.glsl
        assets/test_single_node.json
        plugInfo.json

)
