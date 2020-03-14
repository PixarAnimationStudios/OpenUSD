set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hio)

pxr_library(hio
    LIBRARIES
        arch
        js
        tf
        vt
        trace
        ar
        hf

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        glslfx
        glslfxConfig

    PRIVATE_CLASSES
        debugCodes

    PUBLIC_HEADERS
        api.h

    CPPFILES
        dictionary.cpp

)
