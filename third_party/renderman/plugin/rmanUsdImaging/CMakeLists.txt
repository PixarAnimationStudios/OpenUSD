set(PXR_PREFIX "")
set(PXR_PACKAGE rmanUsdImaging)

pxr_plugin(rmanUsdImaging
    LIBRARIES
        ar
        arch
        hd
        js
        plug
        sdf
        tf
        usd
        usdGeom
        usdLux
        usdVol
        usdImaging
        vt

    PRIVATE_CLASSES
        legacyMeshLightAdapter
        legacyVolumeLightAdapter
        envDayLightAdapter
        renderTerminalTimeSamplingAdapter

    RESOURCE_FILES
        plugInfo.json
)
