set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdParticleField)

pxr_plugin(${PXR_PACKAGE}
    LIBRARIES
        hd
        tf
        usdVol

    PUBLIC_CLASSES
        debugCodes
        hd3DGaussianSplat
        renderBuffer
        renderDelegate
        renderer
        rendererPlugin
        renderPass
        gsRenderer

    PUBLIC_HEADERS
        renderParam.h

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)
