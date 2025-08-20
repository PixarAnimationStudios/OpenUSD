set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdSkelImaging)

pxr_library(usdSkelImaging
    LIBRARIES
        hio
        hd
        usdImaging
        usdSkel

    PUBLIC_CLASSES
        animationAdapter
        animationSchema
        bindingAPIAdapter
        bindingSchema
        blendShapeAdapter
        blendShapeData
        blendShapeSchema
        dataSourceAnimationPrim
        dataSourceBindingAPI
        dataSourceBlendShapePrim
        dataSourceResolvedExtComputationPrim
        dataSourceResolvedPointsBasedPrim
        dataSourceResolvedSkeletonPrim
        dataSourceSkeletonPrim
        extComputations
        inbetweenShapeSchema
        jointInfluencesData
        package
        pointsResolvingSceneIndex
        resolvedSkeletonSchema
        resolvingSceneIndexPlugin
        skelData
        skeletonResolvingSceneIndex
        skeletonAdapter
        skeletonSchema
        skelGuideData
        skelRootAdapter
        tokens
        utils

    PUBLIC_HEADERS
        api.h
        dataSourceUtils.h

    RESOURCE_FILES
        plugInfo.json
        shaders/skinning.glslfx
)

# Build tests
pxr_build_test(testUsdSkelImagingChanges
    LIBRARIES
        usdImaging
        usdShade
        usdSkel
        usd
        sdf
        hd
        tf
    CPPFILES
        testenv/testUsdSkelImagingChanges.cpp
)

# Install (copy test assets and baselines)
pxr_install_test_dir(
    SRC testenv/testUsdSkelImagingChanges
    DEST testUsdSkelImagingChanges
)

# Register tests only when they are supported.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdSkelImagingChanges
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingChanges"
        EXPECTED_RETURN_CODE 0
        ENV
        TF_FATAL_VERIFY=0
    )
endif()