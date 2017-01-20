set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE hdSt)

pxr_shared_library(hdSt
    LIBRARIES
        hd
        tf
        tracelite

    PUBLIC_CLASSES
        basisCurves
        mesh
        points

    PRIVATE_CLASSES
        basisCurvesTopology
        meshTopology    
        quadrangulate
        subdivision
        subdivision3
        triangulate
)
