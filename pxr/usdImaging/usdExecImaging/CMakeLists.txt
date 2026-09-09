set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdExecImaging)

# usdExecImaging requires exec.
if(NOT PXR_BUILD_EXEC)
    message(STATUS
        "Skipping building usdExecImaging because PXR_BUILD_EXEC is OFF")
    return()
endif()

pxr_library(usdExecImaging
    LIBRARIES
        execGeom
        execIr
        execUsd
        hd
        sdf
        tf
        trace
        usd
        usdGeom
        usdImaging
        vt

    PUBLIC_CLASSES
        computedDataSource
        primAdapterInterface
        requestAccessorInterface
        requestBuilderInterface

    PUBLIC_HEADERS
        api.h
        valueKey.h

    PRIVATE_CLASSES
        adapterRegistry
        debugCodes
        geomXformablePrimAdapter
        irXformablePrimAdapter
        request
        requestBuilder
        stageSceneIndex

    PRIVATE_HEADERS
        valueKeyMap.h

    RESOURCE_FILES
        plugInfo.json
)
