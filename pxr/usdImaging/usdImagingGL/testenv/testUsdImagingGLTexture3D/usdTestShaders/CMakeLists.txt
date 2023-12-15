set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdTestShaders)

pxr_plugin(usdTestShaders
    LIBRARIES
        ar
        ndr
        sdr
        usdShade

    PRIVATE_CLASSES
        discoveryPlugin

    RESOURCE_FILES
        plugInfo.json
        shaders/shaderDefs.usda
        shaders/uvwTexture.glslfx
        shaders/sampleTexture3D.glslfx
)
