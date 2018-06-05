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
    
    PRIVATE_HEADERS
        declarePackageOps.h
    
    CPPFILES
        register.cpp
        prmanGeometryDecorator.cpp        
)

target_compile_definitions(${PXR_PACKAGE}
    PRIVATE

    FNATTRIBUTE_STATIC
    FNGEOLIB_STATIC
    FNGEOLIBSERVICES_STATIC
)
