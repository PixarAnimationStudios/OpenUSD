set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hd)

set(optionalPublicClasses "")

if (OPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_TRANSFORM_FEEDBACK)
endif()
if (OPENSUBDIV_HAS_GLSL_COMPUTE)
    add_definitions(-DOPENSUBDIV_HAS_GLSL_COMPUTE)
endif()

pxr_library(hd
    LIBRARIES
        plug
        tf
        tracelite
        vt
        work
        sdf
        garch
        hf
        glf
        pxOsd
        ${OPENSUBDIV_LIBRARIES}
        ${GLEW_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${OPENSUBDIV_INCLUDE_DIR}
        ${GLEW_INCLUDE_DIR}
        ${TBB_INCLUDE_DIRS}
        ${OPENSUBDIV_INCLUDE_DIR}

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
        changeTracker
        compExtCompInputSource
        computation
        conversions
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
        instancer
        instanceRegistry
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
        tokens
        topology
        unitTestDelegate
        unitTestHelper
        unitTestNullRenderDelegate
        unitTestNullRenderPass
        vertexAdjacency
        vtBufferSource
        vtExtractor

    PUBLIC_HEADERS
        api.h
        enums.h
        types.h
        version.h

    PRIVATE_CLASSES
        extComputationContextInternal
        ${optionalPrivateClasses}

    RESOURCE_FILES
        plugInfo.json
)

