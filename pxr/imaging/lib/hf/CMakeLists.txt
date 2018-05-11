set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hf)

set(optionalPublicClasses "")

pxr_library(hf
    LIBRARIES
        plug
        tf
        trace

    PUBLIC_HEADERS
        api.h
        diagnostic.h
        pluginDesc.h
        perfLog.h

    PUBLIC_CLASSES
        pluginBase
        pluginRegistry
        
    PRIVATE_CLASSES
        pluginEntry
)
