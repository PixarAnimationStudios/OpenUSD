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

pxr_install_test_dir(
    SRC testenv/testExecUsdAttributeComputations
    DEST testExecUsdAttributeComputations
)

pxr_register_test(testExecUsdAttributeComputations
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdAttributeComputations"
    EXPECTED_RETURN_CODE 0
    TESTENV testExecUsdAttributeComputations
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

pxr_install_test_dir(
    SRC testenv/testExecUsdBasicCompilation
    DEST testExecUsdBasicCompilation
)

pxr_register_test(testExecUsdBasicCompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdBasicCompilation"
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFile.py testCompiler.dot testCompiler.out"
    DIFF_COMPARE testCompiler.out
    TESTENV testExecUsdBasicCompilation
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

pxr_install_test_dir(
    SRC testenv/testExecUsdCallbacks
    DEST testExecUsdCallbacks
)

pxr_register_test(testExecUsdCallbacks
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdCallbacks"
    EXPECTED_RETURN_CODE 0
    TESTENV testExecUsdCallbacks
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

pxr_install_test_dir(
    SRC testenv/testExecUsdConnectionTargetedObjects
    DEST testExecUsdConnectionTargetedObjects
)

pxr_register_test(testExecUsdConnectionTargetedObjects
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdConnectionTargetedObjects"
    EXPECTED_RETURN_CODE 0
    TESTENV testExecUsdConnectionTargetedObjects
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

pxr_install_test_dir(
    SRC testenv/testExecUsdDispatchedComputations
    DEST testExecUsdDispatchedComputations
)

pxr_register_test(testExecUsdDispatchedComputations
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdDispatchedComputations"
    EXPECTED_RETURN_CODE 0
    TESTENV testExecUsdDispatchedComputations
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

pxr_install_test_dir(
    SRC testenv/testExecUsdMetadataInput
    DEST testExecUsdMetadataInput
)

pxr_register_test(testExecUsdMetadataInput
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdMetadataInput"
    EXPECTED_RETURN_CODE 0
    TESTENV testExecUsdMetadataInput
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

pxr_install_test_dir(
    SRC testenv/testExecUsdCycleDetection
    DEST testExecUsdCycleDetection
)

function(pxr_register_test_ExecUsdCycleDetection TESTNAME)
    pxr_register_test(testExecUsdCycleDetection_${TESTNAME}
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdCycleDetection ${TESTNAME}"
        EXPECTED_RETURN_CODE 134
        TESTENV testExecUsdCycleDetection
        ENV TF_FATAL_VERIFY=0
    )
endfunction()

pxr_register_test_ExecUsdCycleDetection(CyclicComputation)
pxr_register_test_ExecUsdCycleDetection(CyclicComputationPair)
pxr_register_test_ExecUsdCycleDetection(CyclicRelationshipComputation)
pxr_register_test_ExecUsdCycleDetection(LargeCyclicRelationshipComputation)
pxr_register_test_ExecUsdCycleDetection(CyclicAncestorComputation)

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

pxr_install_test_dir(
    SRC testenv/testExecUsdRecompilation
    DEST testExecUsdRecompilation
)

pxr_register_test(testExecUsdRecompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRecompilation"
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

pxr_install_test_dir(
    SRC testenv/testExecUsdRequest
    DEST testExecUsdRequest
)

pxr_register_test(testExecUsdRequest
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRequest"
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

pxr_install_test_dir(
    SRC testenv/testExecUsdRequestInvalidation
    DEST testExecUsdRequestInvalidation
)

pxr_register_test(testExecUsdRequestInvalidation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdRequestInvalidation"
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

pxr_install_test_dir(
    SRC testenv/testExecUsdUncompilation
    DEST testExecUsdUncompilation
)

pxr_register_test(testExecUsdUncompilation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUsdUncompilation"
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
