set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE sdrGlslfx)

pxr_plugin(sdrGlslfx
    LIBRARIES
        ar
        ndr
        sdr
        hio

    PRIVATE_CLASSES
        parserPlugin

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)
