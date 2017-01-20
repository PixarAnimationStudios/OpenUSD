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
        basisCurvesComputations
        basisCurvesShaderKey
        basisCurvesTopology
        meshShaderKey
        meshTopology 
        pointsShaderKey
        quadrangulate
        subdivision
        subdivision3
        triangulate
)
