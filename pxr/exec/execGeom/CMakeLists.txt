set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execGeom)

pxr_library(execGeom
    LIBRARIES
        gf
        tf
        execUsd
        usdGeom
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        tokens 

    PRIVATE_HEADERS

    PRIVATE_CLASSES

    CPPFILES
        xformable.cpp

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        README.md
)

#
# Tests
#

# Test plugins and the tests that rely on them aren't currently supported in
# static builds.
if (BUILD_SHARED_LIBS)
    pxr_build_test(testExecGeomXformable
        LIBRARIES
            tf
            trace
            esf
            exec
            execUsd
            sdf
            usd
            usdGeom
        CPPFILES
            testenv/testExecGeomXformable.cpp
    )

    pxr_register_test(testExecGeomXformable
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecGeomXformable"
        EXPECTED_RETURN_CODE 0
        POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFile.py testCompiler.dot testCompiler.out"
        DIFF_COMPARE testCompiler.out
        TESTENV testExecGeomXformable
    )

    pxr_install_test_dir(
        SRC testenv/testExecGeomXformable
        DEST testExecGeomXformable
    )

    pxr_build_test(testExecGeomXformable_Perf
        LIBRARIES
            tf
            trace
            esf
            exec
            execUsd
            sdf
            usd
            usdGeom
        CPPFILES
            testenv/testExecGeomXformable_Perf.cpp
    )

    pxr_register_test(testExecGeomXformable_Perf
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecGeomXformable_Perf
                 --branchingFactor 5 --treeDepth 7 --recompile"
        EXPECTED_RETURN_CODE 0
        ENV TF_FATAL_VERIFY=1
    )

    pxr_register_test(testExecGeomXformable_Perf_SingleThreaded
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecGeomXformable_Perf
                 --branchingFactor 5 --treeDepth 7 --numThreads 1 --recompile"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testExecGeomXformable_Perf_Chain
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecGeomXformable_Perf
                 --branchingFactor 1 --treeDepth 19531"
        EXPECTED_RETURN_CODE 0
    )

    pxr_register_test(testExecGeomXformable_Perf_Wide
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecGeomXformable_Perf
                 --branchingFactor 19530 --treeDepth 2"
        EXPECTED_RETURN_CODE 0
        ENV TF_FATAL_VERIFY=1
    )

    pxr_register_test(testExecGeomXformable_Perf_Large
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecGeomXformable_Perf
                 --branchingFactor 5 --treeDepth 10"
        EXPECTED_RETURN_CODE 0
    )
endif()
