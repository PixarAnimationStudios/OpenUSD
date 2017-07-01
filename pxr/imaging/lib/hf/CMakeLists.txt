set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hf)

set(optionalPublicClasses "")

pxr_library(hf
    LIBRARIES
        plug
        tf
        tracelite

    PUBLIC_HEADERS
        api.h
        pluginDesc.h
        perfLog.h

    PUBLIC_CLASSES
        pluginBase
        pluginRegistry
        
    PRIVATE_CLASSES
        pluginEntry
)
