set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE usdShaders)

pxr_plugin(usdShaders
    LIBRARIES
        ar
        ndr
        sdr
        usdShade

    PRIVATE_CLASSES
        discoveryPlugin

    PRIVATE_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/shaderDefs.usda
        shaders/previewSurface.glslfx
        shaders/uvTexture.glslfx
        shaders/primvarReader.glslfx
)
