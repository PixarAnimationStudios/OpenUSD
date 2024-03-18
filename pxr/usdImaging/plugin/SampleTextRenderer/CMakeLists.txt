set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE SampleTextRenderer)

pxr_plugin(SampleTextRenderer
    LIBRARIES
        ar
        ndr
        sdr
        usdShade
        usdImaging

    PRIVATE_CLASSES
        discoveryPlugin

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        shaders/shaderDefs.usda
        shaders/SampleTextRendererSurface.glslfx

    DOXYGEN_FILES
        overview.dox

    CPPFILES
        SampleTextRenderer.cpp
)
