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
        basisCurvesTopology
        binding
        bprim
        bufferArray
        bufferArrayRange
        bufferArrayRegistry
        bufferResource
        bufferSource
        bufferSpec
        camera
        changeTracker
        compExtCompInputSource
        computation
        coordSys
        debugCodes
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
        extComputationUtils
        field
        flatNormals
        geomSubset
        instancer
        instanceRegistry
        light
        material
        mesh
        meshTopology
        meshUtil
        perfLog
        pluginRenderDelegateUniqueHandle
        points
        primGather
        primTypeIndex
        renderBuffer
        renderDelegate
        renderIndex
        renderPass
        renderPassState
        rendererPlugin
        rendererPluginHandle
        rendererPluginRegistry
        renderThread
        repr
        resource
        resourceRegistry
        rprim
        rprimCollection
        rprimSharedData
        sceneDelegate
        sceneExtCompInputSource
        selection
        smoothNormals
        sortedIds                   
        sprim
        strategyBase
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
	volume
        vtBufferSource

    PUBLIC_HEADERS
        api.h
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

pxr_build_test(testHdExtComputationUtils
    LIBRARIES
        hd

    CPPFILES
        testenv/testHdExtComputationUtils.cpp
)

pxr_register_test(testHdExtComputationUtils
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testHdExtComputationUtils"
)
