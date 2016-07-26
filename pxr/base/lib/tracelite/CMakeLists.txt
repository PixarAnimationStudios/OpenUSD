set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE tracelite)

pxr_shared_library(tracelite
    LIBRARIES
        arch

    PUBLIC_CLASSES
        trace
)
