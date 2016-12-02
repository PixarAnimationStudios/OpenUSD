set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE gal)

set(optionalPublicClasses "")

pxr_shared_library(gal
    LIBRARIES
        hf
        plug
        tf
        tracelite

    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        delegate
        delegateRegistry
)
