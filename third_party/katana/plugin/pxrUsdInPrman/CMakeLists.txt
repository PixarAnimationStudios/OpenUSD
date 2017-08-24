set(PXR_PACKAGE pxrUsdInPrman)

pxr_plugin(${PXR_PACKAGE}
    KATANA_PLUGIN
    LIBRARIES
        katanaAttrfncApi
        katanaOpApi
        katanaPluginApi
        kind
        usd
        usdGeom
        usdKatana
    
    CPPFILES
        register.cpp
        prmanGeometryDecorator.cpp        
)
