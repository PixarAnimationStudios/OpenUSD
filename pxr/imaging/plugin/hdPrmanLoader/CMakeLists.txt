set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdPrmanLoader)

pxr_plugin(hdPrmanLoader
   LIBRARIES
        arch
        plug
        tf
        hd

    PUBLIC_CLASSES
        rendererPlugin

    RESOURCE_FILES
        plugInfo.json

    DISABLE_PRECOMPILED_HEADERS
)

