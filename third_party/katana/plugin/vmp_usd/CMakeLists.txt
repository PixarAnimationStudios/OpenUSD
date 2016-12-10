set(PXR_PACKAGE vmp_usd)

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        arch
        tf
        work
        usd
        usdUtils
        glf
        usdImaging
        usdImagingGL
        katanaPluginApi
        usdKatana

    CPPFILES
        debugCodes.cpp
        usdVMP.cpp
)

