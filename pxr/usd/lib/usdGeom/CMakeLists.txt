set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdGeom)

pxr_shared_library(usdGeom
    LIBRARIES
        tf
        plug
        vt
        sdf
        tracelite
        usd
        work
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_CLASSES
        debugCodes
        tokens
        bboxCache
        constraintTarget
        primvar
        xformCache
        basisCurves
        boundable
        camera
        capsule
        cone
        cube
        curves
        cylinder
        collectionAPI
        faceSetAPI
        imageable
        gprim
        mesh
        metrics
        modelAPI
        nurbsCurves
        nurbsPatch
        pointBased
        pointInstancer
        points
        scope
        sphere
        xform
        xformable
        xformOp
        xformCommonAPI
        
    PUBLIC_HEADERS
        api.h

    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapBBoxCache.cpp
        wrapBasisCurves.cpp
        wrapBoundable.cpp
        wrapCamera.cpp
        wrapConstraintTarget.cpp
        wrapCapsule.cpp
        wrapCollectionAPI.cpp
        wrapCone.cpp
        wrapCube.cpp
        wrapCurves.cpp
        wrapCylinder.cpp
        wrapFaceSetAPI.cpp
        wrapGprim.cpp
        wrapImageable.cpp
        wrapMesh.cpp
        wrapMetrics.cpp
        wrapModelAPI.cpp
        wrapNurbsCurves.cpp
        wrapNurbsPatch.cpp
        wrapPointBased.cpp
        wrapPointInstancer.cpp
        wrapPoints.cpp
        wrapPrimvar.cpp
        wrapScope.cpp
        wrapSphere.cpp
        wrapTokens.cpp
        wrapXform.cpp
        wrapXformCache.cpp
        wrapXformable.cpp
        wrapXformOp.cpp
        wrapXformCommonAPI.cpp

    PYTHON_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
)
