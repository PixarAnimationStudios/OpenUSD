set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdsi)

pxr_library(hdsi
    LIBRARIES
        plug
        tf
        trace
        vt
        work
        sdf
        cameraUtil
        hf
        hd
        pxOsd

    PUBLIC_CLASSES
        implicitSurfaceSceneIndex

    PUBLIC_HEADERS
        api.h
)
