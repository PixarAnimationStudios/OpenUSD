set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE usdPhysics)

pxr_library(usdPhysics
    INCLUDE_SCHEMA_FILES

    LIBRARIES
        tf
        plug
        vt
        sdf
        trace
        usd
        usdGeom
        usdShade
        work
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        metrics
        
    PUBLIC_HEADERS
        api.h
        massProperties.h

    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        wrapMetrics.cpp

    PYMODULE_FILES
        __init__.py

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

pxr_test_scripts(
    testenv/testUsdPhysicsMetrics.py
    testenv/testUsdPhysicsRigidBodyAPI.py
    testenv/testUsdPhysicsCollisionGroupAPI.py
)

pxr_register_test(testUsdPhysicsMetrics
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPhysicsMetrics"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdPhysicsRigidBodyAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPhysicsRigidBodyAPI"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testUsdPhysicsCollisionGroupAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testUsdPhysicsCollisionGroupAPI"
    EXPECTED_RETURN_CODE 0
)
