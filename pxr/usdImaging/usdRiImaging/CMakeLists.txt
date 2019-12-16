set(PXR_PREFIX pxr/usdRiImaging)
set(PXR_PACKAGE usdRiImaging)

pxr_library(usdRiImaging
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
        usdVol
        ar
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        pxrBarnLightFilterAdapter
        pxrIntMultLightFilterAdapter
        pxrRodLightFilterAdapter

    PUBLIC_HEADERS
        api.h
        unitTestHelper.h
        version.h

    RESOURCE_FILES
        plugInfo.json
)

