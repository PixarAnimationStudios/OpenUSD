set(PXR_PREFIX "")
set(PXR_PACKAGE rmanDiscovery)

pxr_plugin(rmanDiscovery
    LIBRARIES
        gf
        plug
        tf
        vt
        ar
        ndr
        sdr

    PRIVATE_CLASSES
        rmanDiscovery

    RESOURCE_FILES
        plugInfo.json
)
