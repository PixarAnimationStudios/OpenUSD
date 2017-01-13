set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdSt)

set(optionalPublicClasses "")

pxr_shared_library(hdSt
    LIBRARIES
        hd
        tf
        tracelite

    PUBLIC_CLASSES
        basisCurves
        mesh
        points
)
