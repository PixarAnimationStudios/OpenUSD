SET(CPPFILES
        debugCodes.cpp
        tokens.cpp
        hd3DGaussianSplat.cpp
        renderBuffer.cpp
        renderDelegate.cpp
        renderer.cpp
        rendererPlugin.cpp
        renderPass.cpp

        gsRenderer.cpp
)

set(PXR_PACKAGE hdParticleField)

pxr_plugin(hdParticleField

        CPPFILES
        ${CPPFILES}

        LIBRARIES
        arch
        js
        plug
        tf
        sdf
        vt
        gf
        hd
        hf
        usdGeom
        usdLightField
        usdImaging

        RESOURCE_FILES
        plugInfo.json
)
