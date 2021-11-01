set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hd)

set(optionalPublicClasses "")

pxr_library(hd
    LIBRARIES
        plug
        tf
        trace
        vt
        work
        sdf
        cameraUtil
        hf
        pxOsd
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        aov
        basisCurves
        basisCurvesSchema
        basisCurvesTopology
        basisCurvesTopologySchema
        binding
        bprim
        bufferArray
        bufferArrayRange
        bufferArrayRegistry
        bufferResource
        bufferSource
        bufferSpec
        camera
        cameraSchema
        categoriesSchema
        changeTracker
        command
        compExtCompInputSource
        computation
        coordSys
        coordSysBindingSchema
        dataSource
        dataSourceLegacyPrim
        dataSourceLocator
        debugCodes
        dependenciesSchema
        dependencySchema
        dirtyBitsTranslator
        dirtyList
        drawingCoord
        drawItem
        engine
        enums
        extCompCpuComputation
        extCompInputSource
        extCompPrimvarBufferSource
        extComputation
        extComputationContext
        extComputationInputComputationSchema
        extComputationOutputSchema
        extComputationPrimvarSchema
        extComputationPrimvarsSchema
        extComputationSchema
        extComputationUtils
        extentSchema
        field
        filteringSceneIndex
        flatteningSceneIndex
        flatNormals
        geomSubset
        geomSubsetSchema
        geomSubsetsSchema
        instanceCategoriesSchema
        instancedBySchema
        instancer
        instanceRegistry
        instancerTopologySchema
        instanceSchema
        legacyDisplayStyleSchema
        legacyPrimSceneIndex
        light
        lightSchema
        material
        materialBindingSchema
        materialConnectionSchema
        materialNetworkSchema
        materialNodeSchema
        materialSchema
        mesh
        meshSchema
        meshTopology
        meshTopologySchema
        meshUtil
        overlayContainerDataSource
        perfLog
        pluginRenderDelegateUniqueHandle
        points
        prefixingSceneIndex
        primDataSourceOverlayCache
        primGather
        primTypeIndex
        primvarSchema
        primvarsSchema
        purposeSchema
        renderBuffer
        renderBufferSchema
        renderDelegate
        rendererPlugin
        rendererPluginHandle
        rendererPluginRegistry
        renderIndex
        renderPass
        renderPassState
        renderThread
        repr
        resource
        resourceRegistry
        retainedDataSource
        retainedSceneIndex
        rprim
        rprimCollection
        rprimSharedData
        sceneDelegate
        sceneExtCompInputSource
        sceneIndex
        sceneIndexAdapterSceneDelegate
        sceneIndexObserver
        schema
        selection
        smoothNormals
        sortedIds
        sprim
        strategyBase
        subdivisionTagsSchema
        task
        timeSampleArray
        tokens
        topology
        types
        unitTestDelegate
        unitTestHelper
        unitTestNullRenderDelegate
        unitTestNullRenderPass
        vertexAdjacency
        visibilitySchema
        volume
        volumeFieldBindingSchema
        volumeFieldSchema
        vtBufferSource
        xformSchema

    PUBLIC_HEADERS
        api.h
        dataSourceTypeDefs.h
        driver.h
        version.h

    PRIVATE_CLASSES
        extComputationContextInternal
        ${optionalPrivateClasses}

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

pxr_build_test(testHdDirtyList
    LIBRARIES
        hd

    CPPFILES
        testenv/testHdDirtyList.cpp
)

pxr_register_test(testHdDirtyList
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdDirtyList"
)


pxr_build_test(testHdExtComputationUtils
    LIBRARIES
        hd

    CPPFILES
        testenv/testHdExtComputationUtils.cpp
)

pxr_register_test(testHdExtComputationUtils
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdExtComputationUtils"
)

pxr_build_test(testHdSceneIndex
    LIBRARIES
        hd

    CPPFILES
        testenv/testHdSceneIndex.cpp
)

pxr_register_test(testHdSceneIndex
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdSceneIndex"
)

pxr_build_test(testHdTimeSampleArray
    LIBRARIES
        hd

    CPPFILES
        testenv/testHdTimeSampleArray.cpp
)

pxr_register_test(testHdTimeSampleArray
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdTimeSampleArray"
)
