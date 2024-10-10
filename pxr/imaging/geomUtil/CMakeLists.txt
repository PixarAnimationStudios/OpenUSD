set(PXR_PREFIX pxr/imaging)
set(PXR_PACKAGE geomUtil)

pxr_library(geomUtil
    LIBRARIES
        arch
        gf
        tf
        vt
        pxOsd

    PUBLIC_CLASSES
        capsuleMeshGenerator
        coneMeshGenerator
        cuboidMeshGenerator
        cylinderMeshGenerator
        diskMeshGenerator
        meshGeneratorBase
        planeMeshGenerator
        sphereMeshGenerator
        tokens

    PYTHON_CPPFILES
        moduleDeps.cpp
        
    PUBLIC_HEADERS
        api.h

    PYMODULE_CPPFILES
        module.cpp
        wrapCapsuleMeshGenerator.cpp
        wrapConeMeshGenerator.cpp
        wrapCuboidMeshGenerator.cpp
        wrapCylinderMeshGenerator.cpp
        wrapDiskMeshGenerator.cpp
        wrapPlaneMeshGenerator.cpp
        wrapSphereMeshGenerator.cpp
        wrapTokens.cpp

    PYMODULE_FILES
        __init__.py
)

pxr_build_test(testMeshGenerators
   LIBRARIES
        gf
        tf
        vt
        geomUtil
        pxOsd
   CPPFILES
        testenv/testMeshGenerators.cpp
)

pxr_install_test_dir(
    SRC testenv/testMeshGenerators
    DEST testMeshGenerators
)

pxr_register_test(testMeshGenerators
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testMeshGenerators"
    DIFF_COMPARE generatedMeshes_closed.txt generatedMeshes_open.txt
)
