set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE esfUsd)

pxr_library(esfUsd
    LIBRARIES
        arch
        esf
        tf
        sdf
        usd
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        sceneAdapter
        stageData

    PRIVATE_CLASSES
        attribute
        attributeQuery
        object
        prim
        property
        relationship
        stage

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        README.md        
)

#
# Tests
#

# Build test plugins

pxr_build_test(testEsfUsdConnectionTables
    LIBRARIES
        esf
        esfUsd
        sdf
        tf
        usd
        usdGeom
    CPPFILES
        testenv/testEsfUsdConnectionTables.cpp
)

pxr_build_test(testEsfUsdConnectionTablesStochastic
    LIBRARIES
        arch
        esf
        esfUsd
        sdf
        tf
        trace
        usd
        usdGeom
        work
    CPPFILES
        testenv/testEsfUsdConnectionTablesStochastic.cpp
)

pxr_build_test(testEsfUsdSceneObjects
    LIBRARIES
        esf
        esfUsd
        usdGeom
    CPPFILES
        testenv/testEsfUsdSceneObjects.cpp
)

# Build tests

pxr_register_test(testEsfUsdConnectionTables
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfUsdConnectionTables"
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
)

pxr_register_test(testEsfUsdConnectionTables_Threading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfUsdConnectionTablesStochastic"
             --numStages 50 --treeDepth 5 --branchingFactor 5
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
)

pxr_register_test(testEsfUsdConnectionTables_Perf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfUsdConnectionTablesStochastic"
             --numStages 10 --treeDepth 5 --branchingFactor 5 --trace
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
)

pxr_register_test(testEsfUsdConnectionTables_Perf_SingleThreaded
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfUsdConnectionTablesStochastic"
             --numStages 10 --treeDepth 5 --branchingFactor 5 --trace
             --numThreads 1
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
)

pxr_register_test(testEsfUsdConnectionTables_Mem
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfUsdConnectionTablesStochastic"
             --numStages 10 --treeDepth 5 --branchingFactor 5 --memory
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
)

pxr_register_test(testEsfUsdSceneObjects
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEsfUsdSceneObjects"
    EXPECTED_RETURN_CODE 0
)
