set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE HdUsdWriter)
set(optionalPublicClasses "")

pxr_plugin(HdUsdWriter
    DISABLE_PRECOMPILED_HEADERS

    LIBRARIES
        hd
        usdImaging
        usdLux
        usdShade
        usdSkel
        usdVol
        usdVolImaging

    PUBLIC_CLASSES
        camera
        curves
        instancer
        light
        material
        mesh
        openvdbasset
        points
        renderPass        
        renderDelegate
        rendererPlugin
        volume
        
    PUBLIC_HEADERS
        api.h        
        pointBased.h
        rprim.h
        utils.h   

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)
