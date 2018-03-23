set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hd)

set(optionalPublicClasses "")

pxr_library(hd
    LIBRARIES
        plug
        tf
        tracelite
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
        debugCodes
        dirtyList
        drawingCoord
        drawItem
        engine
        extCompCpuComputation
        extCompInputSource
        extCompPrimvarBufferSource
        extComputation
        extComputationContext
        geomSubset
        instancer
        instanceRegistry
        light
        material
        materialParam
        mesh
        meshTopology
        meshUtil
        patchIndex
        perfLog
        points
        primGather
        primTypeIndex
        renderDelegate
        renderIndex
        renderPass
        renderPassState
        repr
        resource
        resourceRegistry
        rprim
        rprimCollection
        rprimSharedData
        sceneDelegate
        sceneExtCompInputSource
        smoothNormals
        sortedIds                   
        sprim
        strategyBase
        task
        texture
        textureResource
        timeSampleArray
        tokens
        topology
        types
        unitTestDelegate
        unitTestHelper
        unitTestNullRenderDelegate
        unitTestNullRenderPass
        vertexAdjacency
        vtBufferSource

    PUBLIC_HEADERS
        api.h
        enums.h
        version.h

    PRIVATE_CLASSES
        extComputationContextInternal
        ${optionalPrivateClasses}

    RESOURCE_FILES
        plugInfo.json
)

