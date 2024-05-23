set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdsi)

pxr_library(hdsi
    LIBRARIES
        plug
        tf
        trace
        vt
        work
        sdf
        cameraUtil
        geomUtil
        hf
        hd
        pxOsd

    PUBLIC_CLASSES
        coordSysPrimSceneIndex
        extComputationPrimvarPruningSceneIndex
        implicitSurfaceSceneIndex
        legacyDisplayStyleOverrideSceneIndex
        materialBindingResolvingSceneIndex
        materialPrimvarTransferSceneIndex
        nurbsApproximatingSceneIndex
        pinnedCurveExpandingSceneIndex
        primManagingSceneIndexObserver
        primTypeNoticeBatchingSceneIndex
        primTypePruningSceneIndex
        renderSettingsFilteringSceneIndex
        sceneGlobalsSceneIndex
        tetMeshConversionSceneIndex
        utils

    PUBLIC_HEADERS
        api.h
        version.h
)

pxr_build_test(testHdsiPinnedCurveExpandingSceneIndex
   LIBRARIES
       hd
       hdsi
       tf

   CPPFILES
       testenv/testHdsiPinnedCurveExpandingSceneIndex.cpp
)
pxr_register_test(testHdsiPinnedCurveExpandingSceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdsiPinnedCurveExpandingSceneIndex"
)
