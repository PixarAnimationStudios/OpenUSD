set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdar)

set(optionalPublicClasses "")

pxr_library(hdar
    LIBRARIES
        hd
        ar

    INCLUDE_DIRS

    PUBLIC_CLASSES
        systemSchema

    PUBLIC_HEADERS
        api.h

    PRIVATE_CLASSES
        ${optionalPrivateClasses}

    RESOURCE_FILES

    DOXYGEN_FILES
        overview.dox
)
