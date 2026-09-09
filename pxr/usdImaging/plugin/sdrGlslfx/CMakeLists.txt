set(PXR_PREFIX pxr/usdImaging)
set(PXR_PACKAGE sdrGlslfx)

pxr_plugin(sdrGlslfx
    LIBRARIES
        ar
        sdr
        hio

    PRIVATE_CLASSES
        parserPlugin

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)
