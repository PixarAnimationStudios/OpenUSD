set(PXR_PACKAGE Hydra)

pxr_plugin(${PXR_PACKAGE}

    LIBRARIES
        ar 
        gf 
        js 
        kind 
        plug 
        sdf 
        tf
        usd
        usdGeom
        usdRi
        usdShade
        usdUtils
        usdImaging
        usdImagingGL
        gusd

    INCLUDE_DIRS
        ${HOUDINI_INCLUDE_DIRS}

    PRIVATE_CLASSES
        GR_PrimHydra
        GT_PrimHydra

    CPPFILES
        USDtoRE.cpp
        plugin.cpp
)
