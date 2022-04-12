set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdImaging)

pxr_library(usdImaging
    LIBRARIES
        gf
        tf
        plug
        trace
        vt
        work
        hd
        pxOsd
        sdf
        usd
        usdGeom
        usdLux
        usdRender
        usdShade
        usdVol
        ar
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        adapterRegistry
        collectionCache
        dataSourceAttribute
        dataSourceBasisCurves
        dataSourceCamera
        dataSourceGprim
        dataSourceMesh
        dataSourcePoints
        dataSourcePrim
        dataSourcePrimvars
        dataSourceRelationship
        dataSourceStageGlobals
        dataSourceVolume
        debugCodes
        delegate
        indexProxy
        materialParamUtils
        resolvedAttributeCache
        instancerContext
        stageSceneIndex
        tokens
        basisCurvesAdapter
        capsuleAdapter
        cameraAdapter
        coneAdapter
        coordSysAdapter
        cubeAdapter
        cylinderAdapter
        cylinderLightAdapter
        diskLightAdapter
        distantLightAdapter
        domeLightAdapter
        fieldAdapter
        geometryLightAdapter
        gprimAdapter
        hermiteCurvesAdapter
        implicitSurfaceMeshUtils
        instanceAdapter
        lightAdapter
        lightFilterAdapter
        materialAdapter
        meshAdapter
        nurbsCurvesAdapter
        nurbsPatchAdapter
        pluginLightAdapter
        pluginLightFilterAdapter
        pointsAdapter
        pointInstancerAdapter
        portalLightAdapter
        primAdapter
        primvarDescCache
        rectLightAdapter
        renderSettingsAdapter
        sampleFilterAdapter
        sphereAdapter
        sphereLightAdapter
        textureUtils
        volumeAdapter

    PUBLIC_HEADERS
        api.h
        unitTestHelper.h
        version.h

    PRIVATE_HEADERS
        dataSourceImplicits-Impl.h

    RESOURCE_FILES
        plugInfo.json
)

