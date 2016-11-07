set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hf)

set(optionalPublicClasses "")

pxr_shared_library(hf
    LIBRARIES
        plug
        tf
        tracelite

    PUBLIC_HEADERS
        pluginDelegateDesc.h
        perfLog.h

    PUBLIC_CLASSES
        pluginDelegateBase
        pluginDelegateRegistry
        
    PRIVATE_CLASSES
        pluginDelegateEntry
)
