set(PXR_PACKAGE vtKatana)

pxr_shared_library(${PXR_PACKAGE}
    LIBRARIES
        gf
        tf
        vt
        sdf
        katanaPluginApi

    PUBLIC_CLASSES
        array
        traits
        value

    PRIVATE_CLASSES
        internalTraits

    PRIVATE_HEADERS
        internalToVt.h
        internalFromVt.h
)

