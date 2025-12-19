SET(CPPFILES
        debugCodes.cpp
        tokens.cpp
        imaging/dataSourceParticleField.cpp
        imaging/particleFieldAdapter.cpp
        renderDelegate/hd3DGaussianSplat.cpp
        renderDelegate/renderBuffer.cpp
        renderDelegate/renderDelegate.cpp
        renderDelegate/renderer.cpp
        renderDelegate/rendererPlugin.cpp
        renderDelegate/renderPass.cpp

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
