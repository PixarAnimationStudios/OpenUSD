set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE execUsd)

pxr_library(execUsd
    LIBRARIES
        esf
        esfUsd
        exec
        tf
        trace
        sdf
        usd
        
    PUBLIC_HEADERS
        api.h

    PUBLIC_CLASSES
        cacheView
        request
        system
        valueKey

    PRIVATE_HEADERS
        visitValueKey.h

    PRIVATE_CLASSES
        requestImpl

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        README.md        
        docs/overview.md
        docs/tutorial1ComputingValues.md
        docs/tutorial2DefiningComputations.md
)

pxr_build_test(testExecUsdAttributeComputations
    LIBRARIES
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdAttributeComputations.cpp
)

pxr_register_test(testExecUsdAttributeComputations
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdAttributeComputations"
    TESTENV testenv/testExecUsdAttributeComputations
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdAttributeValueInput
    LIBRARIES
        tf
        exec
        execUsd
        plug
        sdf
        vdf
        usd
    CPPFILES
        testenv/testExecUsdAttributeValueInput.cpp
)

pxr_register_test(testExecUsdAttributeValueInput
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdAttributeValueInput"
    EXPECTED_RETURN_CODE 0
    TESTENV testenv/testExecUsdAttributeValueInput
)

pxr_build_test(testExecUsdBasicCompilation
    LIBRARIES
        gf
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdBasicCompilation.cpp
)

pxr_register_test(testExecUsdBasicCompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdBasicCompilation"
    TESTENV testenv/testExecUsdBasicCompilation
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFiles.py testCompiler.dot"
    DIFF_COMPARE testCompiler.out
)

pxr_build_test(testExecUsdCallbacks
    LIBRARIES
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdCallbacks.cpp
)

pxr_register_test(testExecUsdCallbacks
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdCallbacks"
    TESTENV testenv/testExecUsdCallbacks
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdConnectionTargetedObjects
    LIBRARIES
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdConnectionTargetedObjects.cpp
)

pxr_register_test(testExecUsdConnectionTargetedObjects
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdConnectionTargetedObjects"
    TESTENV testenv/testExecUsdConnectionTargetedObjects
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdConstantInput
    LIBRARIES
        tf
        esf
        exec
        execUsd
        plug
        sdf
        vdf
        usd
    CPPFILES
        testenv/testExecUsdConstantInput.cpp
)

pxr_register_test(testExecUsdConstantInput
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdConstantInput"
    TESTENV testenv/testExecUsdConstantInput
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFiles.py testConstantInput.dot"
    DIFF_COMPARE testConstantInput.out
)

pxr_build_test(testExecUsdDispatchedComputations
    LIBRARIES
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdDispatchedComputations.cpp
)

pxr_register_test(testExecUsdDispatchedComputations
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdDispatchedComputations"
    TESTENV testenv/testExecUsdDispatchedComputations
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdMetadataInput
    LIBRARIES
        tf
        esf
        exec
        execUsd
        vdf
        usdGeom
    CPPFILES
        testenv/testExecUsdMetadataInput.cpp
)

pxr_register_test(testExecUsdMetadataInput
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdMetadataInput"
    TESTENV testenv/testExecUsdMetadataInput
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdCycleDetection
    LIBRARIES
        tf
        plug
        exec
        execUsd
        vdf
        sdf
        usd
    CPPFILES
        testenv/testExecUsdCycleDetection.cpp
)

function(pxr_register_test_ExecUsdCycleDetection TESTNAME)
    pxr_register_test(testExecUsdCycleDetection_${TESTNAME}
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdCycleDetection ${TESTNAME}"
        EXPECTED_RETURN_CODE 134
        TESTENV testenv/testExecUsdCycleDetection
        ENV TF_FATAL_VERIFY=0
    )
endfunction()

pxr_register_test_ExecUsdCycleDetection(CyclicComputation)
pxr_register_test_ExecUsdCycleDetection(CyclicComputationPair)
pxr_register_test_ExecUsdCycleDetection(CyclicRelationshipComputation)
pxr_register_test_ExecUsdCycleDetection(LargeCyclicRelationshipComputation)
pxr_register_test_ExecUsdCycleDetection(CyclicAncestorComputation)
pxr_register_test_ExecUsdCycleDetection(CyclicRelationshipComputationAfterRecompile)
pxr_register_test_ExecUsdCycleDetection(CyclicRelationshipComputationFigure8)

pxr_build_test(testExecUsdRecompilation
    LIBRARIES
        exec
        execUsd
        plug
        sdf
        tf
        usd
        vdf
    CPPFILES
        testenv/testExecUsdRecompilation.cpp
)

pxr_register_test(testExecUsdRecompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRecompilation"
    TESTENV testenv/testExecUsdRecompilation
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFiles.py                    \
        TestRecompileDisconnectedAttributeInput-1.dot                          \
        TestRecompileDisconnectedAttributeInput-2.dot                          \
        TestRecompileDisconnectedAttributeInput-3.dot                          \
        TestRecompileMultipleRequests-1.dot                                    \
        TestRecompileMultipleRequests-2.dot                                    \
        TestRecompileMultipleRequests-3.dot                                    \
        TestRecompileDeletedPrim-1.dot                                         \
        TestRecompileDeletedPrim-2.dot                                         \
        TestRecompileDeletedPrim-3.dot                                         \
        TestRecompileResyncedPrim-1.dot                                        \
        TestRecompileResyncedPrim-2.dot                                        \
        TestRecompileResyncedPrim-3.dot                                        \
        TestRecompileChangedRelationshipTargets-1.dot                          \
        TestRecompileChangedRelationshipTargets-2.dot                          \
        TestRecompileChangedRelationshipTargets-3.dot                          \
        TestRecompileChangedRelationshipTargets-4.dot                          \
        TestRecompileChangedRelationshipTargets-5.dot                          \
        TestRecompileAfterChangingOldRelationshipTarget-1.dot                  \
        TestRecompileAfterChangingOldRelationshipTarget-2.dot                  \
        TestRecompileAfterChangingOldRelationshipTarget-3.dot                  \
        TestRecompileAfterChangingOldRelationshipTarget-4.dot                  \
        TestRecompileDuplicateInputNames-1.dot                                 \
        TestRecompileDuplicateInputNames-2.dot                                 \
        TestRecompileDuplicateInputNames-3.dot                                 \
        "
    DIFF_COMPARE TestRecompileDisconnectedAttributeInput-1.out
    DIFF_COMPARE TestRecompileDisconnectedAttributeInput-2.out
    DIFF_COMPARE TestRecompileDisconnectedAttributeInput-3.out
    DIFF_COMPARE TestRecompileMultipleRequests-1.out
    DIFF_COMPARE TestRecompileMultipleRequests-2.out
    DIFF_COMPARE TestRecompileMultipleRequests-3.out
    DIFF_COMPARE TestRecompileDeletedPrim-1.out
    DIFF_COMPARE TestRecompileDeletedPrim-2.out
    DIFF_COMPARE TestRecompileDeletedPrim-3.out
    DIFF_COMPARE TestRecompileResyncedPrim-1.out
    DIFF_COMPARE TestRecompileResyncedPrim-2.out
    DIFF_COMPARE TestRecompileResyncedPrim-3.out
    DIFF_COMPARE TestRecompileChangedRelationshipTargets-1.out
    DIFF_COMPARE TestRecompileChangedRelationshipTargets-2.out
    DIFF_COMPARE TestRecompileChangedRelationshipTargets-3.out
    DIFF_COMPARE TestRecompileChangedRelationshipTargets-4.out
    DIFF_COMPARE TestRecompileChangedRelationshipTargets-5.out
    DIFF_COMPARE TestRecompileAfterChangingOldRelationshipTarget-1.out
    DIFF_COMPARE TestRecompileAfterChangingOldRelationshipTarget-2.out
    DIFF_COMPARE TestRecompileAfterChangingOldRelationshipTarget-3.out
    DIFF_COMPARE TestRecompileAfterChangingOldRelationshipTarget-4.out
    DIFF_COMPARE TestRecompileDuplicateInputNames-1.out
    DIFF_COMPARE TestRecompileDuplicateInputNames-2.out
    DIFF_COMPARE TestRecompileDuplicateInputNames-3.out
)

pxr_build_test(testExecUsdRequest
    LIBRARIES
        gf
        plug
        tf
        work
        esf
        exec
        execUsd
        vdf
    CPPFILES
        testenv/testExecUsdRequest.cpp
)

pxr_register_test(testExecUsdRequest
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRequest"
    TESTENV testenv/testExecUsdRequest
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdRequestExpiration
    LIBRARIES
        tf
        ef
        esf
        exec
        execUsd
        vdf
    CPPFILES
        testenv/testExecUsdRequestExpiration.cpp
)

pxr_register_test(testExecUsdRequestExpiration
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRequestExpiration"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdRequestInvalidation
    LIBRARIES
        gf
        plug
        tf
        ef
        esf
        exec
        execUsd
        vdf
    CPPFILES
        testenv/testExecUsdRequestInvalidation.cpp
)

pxr_register_test(testExecUsdRequestInvalidation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRequestInvalidation"
    TESTENV testenv/testExecUsdRequestInvalidation
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUsdUncompilation
    LIBRARIES
        exec
        execUsd
        plug
        tf
        sdf
        usd
    CPPFILES
        testenv/testExecUsdUncompilation.cpp
)

pxr_register_test(testExecUsdUncompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdUncompilation"
    TESTENV testenv/testExecUsdUncompilation
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFiles.py                    \
        TestUncompileAttributeInput-compiled.dot                               \
        TestUncompileAttributeInput-uncompiled.dot                             \
        TestUncompileConstantComputation-compiled.dot                          \
        TestUncompileConstantComputation-uncompiled.dot                        \
        TestUncompileNamespaceAncestorInput-compiled.dot                       \
        TestUncompileNamespaceAncestorInput-uncompiled.dot                     \
        TestUncompileRecursiveResync-compiled.dot                              \
        TestUncompileRecursiveResync-uncompiled.dot                            \
        "
    DIFF_COMPARE TestUncompileAttributeInput-compiled.out
    DIFF_COMPARE TestUncompileAttributeInput-uncompiled.out
    DIFF_COMPARE TestUncompileConstantComputation-compiled.out
    DIFF_COMPARE TestUncompileConstantComputation-uncompiled.out
    DIFF_COMPARE TestUncompileNamespaceAncestorInput-compiled.out
    DIFF_COMPARE TestUncompileNamespaceAncestorInput-uncompiled.out
    DIFF_COMPARE TestUncompileRecursiveResync-compiled.out
    DIFF_COMPARE TestUncompileRecursiveResync-uncompiled.out
)

pxr_build_test(testExecUsdRecompilation_Perf
    LIBRARIES
        arch
        tf
        trace
        plug
        exec
        execUsd
        sdf
        usd
    CPPFILES
        testenv/testExecUsdRecompilation_Perf.cpp
)

pxr_register_test(testExecUsdRecompilation_Perf_TouchRel
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRecompilation_Perf       \
            TouchRel --branchingFactor 5 --treeDepth 7 --rel /Root/Prim1.rel"
    TESTENV testenv/testExecUsdRecompilation_Perf
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
    RUN_SERIAL
)

pxr_register_test(testExecUsdRecompilation_Perf_AddTarget
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRecompilation_Perf       \
            AddTarget --branchingFactor 5 --treeDepth 7 --rel /Root/Prim1.rel  \
            --target /Root/Prim2"
    TESTENV testenv/testExecUsdRecompilation_Perf
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
    RUN_SERIAL
)

pxr_register_test(testExecUsdRecompilation_Perf_AddTargetToNewHierarchy
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRecompilation_Perf       \
            AddTargetToNewHierarchy --branchingFactor 5 --treeDepth 7          \
            --rel /Root/Prim1.rel"
    TESTENV testenv/testExecUsdRecompilation_Perf
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
    RUN_SERIAL
)

pxr_register_test(testExecUsdRecompilation_Perf_AddTargetToConnectedHierarchy
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRecompilation_Perf       \
            AddTargetToConnectedHierarchy --branchingFactor 5 --treeDepth 7    \
            --rel /Root/Prim1.rel --target /Root/Prim2"
    TESTENV testenv/testExecUsdRecompilation_Perf
    EXPECTED_RETURN_CODE 0
    ENV TF_FATAL_VERIFY=1
    RUN_SERIAL
)