set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE usdRiPxrImaging)

pxr_library(usdRiPxrImaging
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
        usdRi
        usdImaging
        usdVol
        ar

    PUBLIC_CLASSES
        projectionSchema
        pxrAovLightAdapter
        pxrCameraAPIAdapter
        pxrCameraProjectionAdapter
        pxrCameraProjectionAPIAdapter
        pxrDisplayFilterAdapter
        pxrEnergyFilterAdapter
        pxrIntegratorAdapter
        pxrVolumeAPIAdapter
        pxrSampleFilterAdapter
        pxrVolumeFilterPluginBaseAdapter
        tokens

    PRIVATE_CLASSES
        pxrRenderTerminalHelper

    PRIVATE_HEADERS
        dataSourcePxrRenderTerminalPrims.h

    PUBLIC_HEADERS
        api.h
        version.h

    RESOURCE_FILES
        plugInfo.json
)

if(TARGET usdRiPxrImaging)
    target_include_directories(${PXR_PACKAGE}
        PRIVATE
            "${PROJECT_BINARY_DIR}/include/pxr/imaging"
    )
endif()

