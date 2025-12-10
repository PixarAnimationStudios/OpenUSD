set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE exec)

pxr_library(exec
    LIBRARIES
        ef
        esf
        tf
        trace
        ts
        sdf
        usd
        vdf
        vt
        TBB::tbb

    PUBLIC_HEADERS
        api.h
        providerResolution.h
        registerSchema.h
        request.h
        types.h
        valueExtractorFunction.h

    PUBLIC_CLASSES
        builtinComputations
        cacheView
        computationBuilders
        debugCodes
        requestImpl
        system
        systemChangeProcessor
        systemDiagnostics
        typeRegistry
        valueKey

    PRIVATE_HEADERS
        compilationState.h
        inputKey.h
        invalidationResult.h
        timeChangeInvalidationResult.h

    PRIVATE_CLASSES
        attributeInputNode
        builtinAttributeComputations
        builtinObjectComputations
        builtinStageComputations
        callbackNode
        compilationTask
        compiledLeafNodeCache
        compiledOutputCache
        compiler
        compilerTaskSync
        compilerTaskSyncBase
        computationDefinition
        constantValueNode
        cycleDetectingTask
        definitionRegistry
        inputRecompilationTask
        inputResolver
        inputResolvingCompilationTask
        leafCompilationTask
        metadataInputNode
        nodeRecompilationInfo
        nodeRecompilationInfoTable
        outputKey
        outputProvidingCompilationTask
        pluginComputationDefinition
        pluginData
        privateBuiltinComputations
        program
        registrationBarrier
        requestTracker
        runtime
        taskCycleDetector
        uncompilationRuleSet
        uncompilationTable
        uncompilationTarget
        uncompiler
        valueExtractor

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        README.md 
        docs/executionSystemDesign.md
        docs/images/executionSystemDesignEngineArchitecture.drawio.svg       
)

#
# Tests
#

# Build test plugins

pxr_build_test_shared_lib(TestExecPluginComputation
    INSTALL_PREFIX ExecPlugins
    LIBRARIES
        tf
        exec
        
    CPPFILES
        testenv/TestExecPluginComputation.cpp
)

pxr_build_test_shared_lib(TestExecConflictingPluginRegistration1
    INSTALL_PREFIX ExecPlugins
    LIBRARIES
        tf
        exec
        
    CPPFILES
        testenv/TestExecConflictingPluginRegistration1.cpp
)

pxr_build_test_shared_lib(TestExecConflictingPluginRegistration2
    INSTALL_PREFIX ExecPlugins
    LIBRARIES
        tf
        exec
        
    CPPFILES
        testenv/TestExecConflictingPluginRegistration2.cpp
)

# Build tests

pxr_build_test(testExecComputationRegistration
    LIBRARIES
        ef
        esf
        esfUsd
        exec
        plug
        sdf
        tf
        usd
    CPPFILES
        testenv/testExecComputationRegistration.cpp
)

# Tests that rely on plugins aren't currently supported in static builds.
if (BUILD_SHARED_LIBS)
    pxr_register_test(testExecComputationRegistration
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecComputationRegistration"
        TESTENV testenv/testExecComputationRegistration
        EXPECTED_RETURN_CODE 0
    )
endif()

pxr_build_test(testExecInputResolver
    LIBRARIES
        ef
        esf
        esfUsd
        exec
        plug
        sdf
        tf
        usd
    CPPFILES
        testenv/testExecInputResolver.cpp
)

pxr_register_test(testExecInputResolver
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecInputResolver"
    TESTENV testenv/testExecInputResolver
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecTypeRegistration
    LIBRARIES
        gf
        tf
        exec
    CPPFILES
        testenv/testExecTypeRegistration.cpp
)

pxr_register_test(testExecTypeRegistration
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecTypeRegistration"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testExecUncompilationTable
    LIBRARIES
        exec
        work
    CPPFILES
        testenv/testExecUncompilationTable.cpp
)

pxr_register_test(testExecUncompilationTable
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testExecUncompilationTable"
    EXPECTED_RETURN_CODE 0
)
