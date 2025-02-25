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
pxr_build_test(testUsdSkelImagingSkeletonAdapter
    LIBRARIES
        usdImaging
        usdShade
        usdGeom
        usdSkel
        usd
        sdf
        hd
        tf
        gf
        arch
    CPPFILES
        testenv/testUsdSkelImagingSkeletonAdapter.cpp
)

# Install (copy test assets and baselines)
pxr_install_test_dir(
    SRC testenv/testUsdSkelImagingSkeletonAdapter
    DEST testUsdSkelImagingSkeletonAdapter
)

# Register tests
pxr_register_test(testUsdSkelImagingSkeletonAdapter
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingSkeletonAdapter"
    EXPECTED_RETURN_CODE 0
)
