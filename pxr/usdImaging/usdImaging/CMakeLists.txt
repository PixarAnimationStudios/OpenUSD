set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImaging)

set(optionalLibraries "")
set(optionalIncludeDirs "")
set(optionalPublicClasses "")
set(optionalPublicHeaders "")
if (PXR_ENABLE_TEXT_SUPPORT)
    list(APPEND optionalPublicClasses simpleTextAdapter markupTextAdapter text textRegistry textRenderer textRendererRegistry textRun markupParser markupParserRegistry)
    list(APPEND optionalPublicHeaders markupText.h textBlock.h textColor.h textLine.h textParagraph.h textRawGlyph.h textStyle.h textStyleChange.h)
endif()

pxr_library(usdImaging
    LIBRARIES
        gf
        tf
        plug
        trace
        vt
        work
        geomUtil
        hd
        hdar
        hio
        pxOsd
        sdf
        usd
        usdGeom
        usdLux
        usdRender
        usdShade
        usdText
        usdVol
        ar
        ${TBB_tbb_LIBRARY}
        ${optionalLibraries}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        ${optionalIncludeDirs}

    PUBLIC_CLASSES
        adapterRegistry
        collectionCache
        collectionMaterialBindingSchema
        collectionMaterialBindingsSchema
        directMaterialBindingSchema
        directMaterialBindingsSchema
        dataSourceAttribute
        dataSourceAttributeColorSpace
        dataSourceBasisCurves
        dataSourceCamera
        dataSourceGprim
        dataSourceMaterial
        dataSourceMesh
        dataSourceNurbsCurves
        dataSourceNurbsPatch
        dataSourcePointInstancer
        dataSourcePoints
        dataSourcePrim
        dataSourcePrimvars
        dataSourceRelationship
        dataSourceRenderPrims
        dataSourceStage
        dataSourceStageGlobals
        dataSourceTetMesh
        dataSourceUsdPrimInfo
        dataSourceVolume
        debugCodes
        delegate
        drawModeAdapter
        drawModeSceneIndex
        extentResolvingSceneIndex
        extentsHintSchema
        geomModelSchema
        indexProxy
        materialParamUtils
        modelSchema
        primvarUtils
        resolvedAttributeCache
        instancerContext
        stageSceneIndex
        selectionSceneIndex
        tokens
        apiSchemaAdapter
        basisCurvesAdapter
        capsuleAdapter
        cameraAdapter
        collectionAPIAdapter
        coneAdapter
        coordSysAdapter
        coordSysAPIAdapter
        cubeAdapter
        cylinderAdapter
        cylinderLightAdapter
        diskLightAdapter
        distantLightAdapter
        domeLightAdapter
        domeLight_1Adapter
        fieldAdapter
        flattenedDataSourceProviders
        flattenedGeomModelDataSourceProvider
        flattenedDirectMaterialBindingsDataSourceProvider
        geometryLightAdapter
        geomModelAPIAdapter
        gprimAdapter
        hermiteCurvesAdapter
        implicitSurfaceMeshUtils
        instanceablePrimAdapter
        instanceAdapter
        lightAdapter
        lightAPIAdapter
        lightFilterAdapter
        materialAdapter
        materialBindingAPIAdapter
        materialBindingsResolvingSceneIndex
        meshAdapter
        niPrototypePropagatingSceneIndex
        nurbsCurvesAdapter
        nurbsPatchAdapter
        piPrototypePropagatingSceneIndex
        planeAdapter
        pluginLightAdapter
        pluginLightFilterAdapter
        pointsAdapter
        pointInstancerAdapter
        portalLightAdapter
        primAdapter
        primvarDescCache
        rootOverridesSceneIndex
        unloadedDrawModeSceneIndex
        usdPrimInfoSchema
        usdRenderProductSchema
        usdRenderSettingsSchema
        usdRenderVarSchema
        rectLightAdapter
        renderProductAdapter
        renderSettingsAdapter
        renderSettingsFlatteningSceneIndex
        renderVarAdapter
        representedByAncestorPrimAdapter
        rerootingSceneIndex
        sceneIndices
        sphereAdapter
        sphereLightAdapter
        tetMeshAdapter
        textureUtils
        volumeAdapter
        ${optionalPublicClasses}

    PRIVATE_CLASSES
        adapterManager
        drawModeStandin
        niInstanceAggregationSceneIndex
        niPrototypePruningSceneIndex
        niPrototypeSceneIndex
        piPrototypeSceneIndex

    PUBLIC_HEADERS
        api.h
        dataSourceSchemaBased.h
        types.h
        unitTestHelper.h
        version.h
        ${optionalPublicHeaders}

    PRIVATE_HEADERS
        dataSourceImplicits-Impl.h

    RESOURCE_FILES
        plugInfo.json
)

