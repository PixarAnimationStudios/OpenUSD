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
        usdShade
        usdVol
        ar
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        adapterRegistry
        collectionCache
        debugCodes
        delegate
        indexProxy
        resolvedAttributeCache
        instancerContext
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
        primAdapter
        primvarDescCache
        rectLightAdapter
        sphereAdapter
        sphereLightAdapter
        textureUtils
        volumeAdapter

    PRIVATE_CLASSES
        materialParamUtils

    PUBLIC_HEADERS
        api.h
        unitTestHelper.h
        version.h

    RESOURCE_FILES
        plugInfo.json
)

