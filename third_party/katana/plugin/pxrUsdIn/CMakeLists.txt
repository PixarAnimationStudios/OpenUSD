set(PXR_PACKAGE pxrUsdIn)

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        usdKatana
        katanaOpApi
        katanaPluginApi
        usd
        usdLux
        usdUtils

    CPPFILES
        pxrUsdIn.cpp
)
