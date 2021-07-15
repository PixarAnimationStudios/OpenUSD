set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdPhysics)

pxr_library(usdPhysics
    LIBRARIES
        js
        tf
        plug
        vt
        sdf
        trace
        usd
        usdGeom
        work
        ${Boost_PYTHON_LIBRARY}
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        articulationRootAPI
        collisionAPI
        collisionGroup
        distanceJoint
        driveAPI
        filteredPairsAPI
        fixedJoint
        joint
        limitAPI
        massAPI
        materialAPI
        meshCollisionAPI
        metrics
        prismaticJoint
        revoluteJoint
        rigidBodyAPI
        scene
        sphericalJoint
        tokens        
        
    PUBLIC_HEADERS
        api.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapArticulationRootAPI.cpp
        wrapCollisionAPI.cpp
        wrapCollisionGroup.cpp
        wrapDistanceJoint.cpp
        wrapDriveAPI.cpp
        wrapFilteredPairsAPI.cpp
        wrapFixedJoint.cpp
        wrapJoint.cpp
        wrapLimitAPI.cpp
        wrapMassAPI.cpp
        wrapMaterialAPI.cpp
        wrapMeshCollisionAPI.cpp
        wrapMetrics.cpp
        wrapPrismaticJoint.cpp
        wrapRevoluteJoint.cpp
        wrapRigidBodyAPI.cpp
        wrapScene.cpp
        wrapSphericalJoint.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
        generatedSchema.usda
        schema.usda:usdPhysics/schema.usda

    DOXYGEN_FILES
        overview.dox        
        examples/usdPhysicsBoxOnBox.usda
        examples/usdPhysicsBoxOnQuad.usda
        examples/usdPhysicsDistanceJoint.usda
        examples/usdPhysicsGroupFiltering.usda
        examples/usdPhysicsJoints.usda
        examples/usdPhysicsPairFiltering.usda
        examples/usdPhysicsSpheresWithMaterial.usda
)

