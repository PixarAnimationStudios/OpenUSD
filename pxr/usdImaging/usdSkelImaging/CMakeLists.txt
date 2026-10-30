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
        xformResolver
    
    PRIVATE_CLASSES
        dataSourcePrimvar

    PUBLIC_HEADERS
        api.h
        dataSourceUtils.h

    RESOURCE_FILES
        plugInfo.json
        shaders/skinning.glslfx
)

# Build tests
pxr_build_test(testUsdSkelImagingBlendShapes
    LIBRARIES
        usdSkelImaging
        usdImaging
        usdSkel
        usd
        sdf
        hd
        tf
        vt
        gf
    CPPFILES
        testenv/testUsdSkelImagingBlendShapes.cpp
)

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

pxr_build_test(testUsdSkelImagingComputations
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
        testenv/testUsdSkelImagingComputations.cpp
)

pxr_build_test(testUsdSkelImagingNiAggregationNonDescendantPaths
    LIBRARIES
        usdImaging
        usdSkel
        usd
        sdf
        hd
        tf
    CPPFILES
        testenv/testUsdSkelImagingNiAggregationNonDescendantPaths.cpp
)

# Register tests only when they are supported.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testUsdSkelImagingBlendShapes
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingBlendShapes"
        EXPECTED_RETURN_CODE 0
        TESTENV testenv/testUsdSkelImagingBlendShapes
        ENV
            TF_FATAL_VERIFY=0
            HD_ENABLE_DEFERRED_SKINNING=0
    )

    pxr_register_test(testUsdSkelImagingBlendShapes_Deferred
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingBlendShapes"
        EXPECTED_RETURN_CODE 0
        TESTENV testenv/testUsdSkelImagingBlendShapes
        ENV
            TF_FATAL_VERIFY=0
            HD_ENABLE_DEFERRED_SKINNING=1
    )

    pxr_register_test(testUsdSkelImagingChanges
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingChanges"
        EXPECTED_RETURN_CODE 0
        TESTENV testenv/testUsdSkelImagingChanges
        ENV
            TF_FATAL_VERIFY=0
    )

    pxr_register_test(testUsdSkelImagingComputations
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingComputations"
        EXPECTED_RETURN_CODE 0
        TESTENV testenv/testUsdSkelImagingComputations
        ENV
            TF_FATAL_VERIFY=0
            USDSKELIMAGING_ENABLE_NORMAL_COMPUTATIONS=1
    )

    pxr_register_test(testUsdSkelImagingNiAggregationNonDescendantPaths
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdSkelImagingNiAggregationNonDescendantPaths"
        EXPECTED_RETURN_CODE 0
        TESTENV testenv/testUsdSkelImagingNiAggregationNonDescendantPaths
        ENV
            TF_FATAL_VERIFY=0
            USD_ASSIGN_PROTOTYPES_DETERMINISTICALLY=1
    )
endif()