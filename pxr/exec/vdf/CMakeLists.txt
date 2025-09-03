set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE vdf)

pxr_library(vdf
    LIBRARIES
        arch
        gf
        tf
        trace
        vt
        work
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}
        
    PUBLIC_HEADERS
        api.h
        boxedContainer.h
        boxedContainerTraits.h
        connectorMap.h
        countingIterator.h
        parallelExecutorDataManager.h
        parallelExecutorDataManagerInterface.h
        parallelExecutorEngineBase.h
        parallelSpeculationExecutorEngine.h
        dataManagerBasedExecutor.h
        dataManagerBasedSubExecutor.h
        dataManagerFacade.h
        defaultInitAllocator.h
        estimateSize.h
        execNodeDebugName.h
        executorDataManager.h
        executorDataManagerInterface.h
        executorFactory.h
        executorFactoryBase.h
        fixedSizePolymorphicHolder.h
        forEachCommonType.h
        indexedData.h
        indexedDataIterator.h
        indexedWeights.h
        inputValuesPointer.h
        iteratorRange.h
        iterators.h
        lruCache.h
        maskMemoizer.h
        maskedIterator.h
        parallelExecutorDataManager.h
        parallelExecutorDataManagerInterface.h
        parallelExecutorEngineBase.h
        parallelSpeculationExecutorEngine.h
        poolChainIndex.h
        pullBasedExecutorEngine.h
        readIterator.h
        readIteratorRange.h
        readWriteAccessor.h
        readWriteIterator.h
        readWriteIteratorRange.h
        requiredInputsPredicate.h
        scheduleNode.h
        scheduleTasks.h
        speculationExecutor.h
        speculationExecutorEngine.h
        subrangeView.h
        traits.h
        typedVector.h
        vectorAccessor.h
        weightedIterator.h

    PUBLIC_CLASSES
        allocateBoxedValue
        compressedIndexMapping
        connection
        connectorSpecs
        context
        datalessExecutor
        dataManagerHashTable
        dataManagerVector
        debugCodes
        error
        evaluationState
        executionStats
        executionStatsProcessor
        executionTypeRegistry
        executorInterface
        executorBufferData
        executorDataVector
        executorErrorLogger
        executorInvalidationData
        executorInvalidator
        executorObserver
        extensibleNode
        fixedSizeHolder
        grapher
        grapherOptions
        indexedWeightsOperand
        input
        inputAndOutputSpecs
        inputSpec
        inputVector
        isolatedSubnetwork
        iterator
        mask
        maskedOutput
        maskedOutputVector
        network
        networkStats
        networkUtil
        node
        nodeProcessInvalidationInterface
        nodeSet
        object
        output
        outputSpec
        parallelDataManagerVector
        parallelExecutorDataVector
        parallelExecutorEngine
        parallelTaskSync
        parallelTaskWaitlist
        rawValueAccessor
        request
        rootNode
        schedule
        scheduler
        simpleExecutor
        smblData
        sparseInputPathFinder
        sparseInputTraverser
        sparseOutputTraverser
        sparseVectorizedInputTraverser
        sparseVectorizedOutputTraverser
        speculationExecutorBase
        speculationNode
        testUtils
        tokens
        typeDispatchTable
        types
        vector
        vectorData
        vectorDataTyped
        vectorImpl_Boxed
        vectorImpl_Compressed
        vectorImpl_Contiguous
        vectorImpl_Dispatch
        vectorImpl_Empty
        vectorImpl_Shared
        vectorImpl_Single
        vectorSubrangeAccessor

    PRIVATE_HEADERS
        dataManagerAllocator.h

    PRIVATE_CLASSES
        dotGrapher
        dynamicTopologicalSorter
        inputAndOutputSpecsRegistry
        maskRegistry
        poolChainIndexer
        scheduleInvalidator

    DOXYGEN_FILES
        README.md
)

pxr_install_test_dir(
    SRC testenv/testVdfEdit
    DEST testVdfEdit
)

pxr_install_test_dir(
    SRC testenv/testVdfGrapher
    DEST testVdfGrapher
)

pxr_install_test_dir(
    SRC testenv/testVdfIndexedWeights
    DEST testVdfIndexedWeights
)

pxr_install_test_dir(
    SRC testenv/testVdfIndexedWeights_Perf
    DEST testVdfIndexedWeights_Perf
)

pxr_install_test_dir(
    SRC testenv/testVdfSparseInputPathFinder
    DEST testVdfSparseInputPathFinder
)

pxr_install_test_dir(
    SRC testenv/testVdfTraversal
    DEST testVdfTraversal
)

pxr_build_test(testVdfBasic
    LIBRARIES
        vdf
        gf 
        tf
        trace
    
    CPPFILES
        testenv/testVdfBasic.cpp
)

pxr_build_test(testVdfCountingIterator
    LIBRARIES
        vdf
        tf
    
    CPPFILES
        testenv/testVdfCountingIterator.cpp
)

pxr_build_test(testVdfDefaultInitAllocator
    LIBRARIES
        vdf
        tf
    
    CPPFILES
        testenv/testVdfDefaultInitAllocator.cpp
)

pxr_build_test(testVdfDynamicTopologicalSorter
    LIBRARIES
        vdf
    
    CPPFILES
        testenv/testVdfDynamicTopologicalSorter.cpp
)

pxr_build_test(testVdfEdit
    LIBRARIES
        vdf
        gf 
        tf
        trace
    
    CPPFILES
        testenv/testVdfEdit.cpp
)

pxr_build_test(testVdfExecutionStats
    LIBRARIES
        vdf
    
    CPPFILES
        testenv/testVdfExecutionStats.cpp
)

pxr_build_test(testVdfExecutionTypeRegistry
    LIBRARIES
        vdf
    
    CPPFILES
        testenv/testVdfExecutionTypeRegistry.cpp
)

pxr_build_test(testVdfGrapher
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfGrapher.cpp
)

pxr_build_test(testVdfHardToReach
    LIBRARIES
        vdf
        tf
              
    CPPFILES
        testenv/testVdfHardToReach.cpp
)

pxr_build_test(testVdfIndexedWeights
    LIBRARIES
        vdf
        tf
        trace
    
    CPPFILES
      testenv/testVdfIndexedWeights.cpp
)

pxr_build_test(testVdfInputValuesPointer
    LIBRARIES
        vdf
        tf
        trace
              
    CPPFILES
        testenv/testVdfInputValuesPointer.cpp
)

pxr_build_test(testVdfIsolatedSubnetwork
    LIBRARIES
        vdf
        gf 
        tf

    CPPFILES
        testenv/testVdfIsolatedSubnetwork.cpp
)

pxr_build_test(testVdfIteratorRange
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfIteratorRange.cpp
)

pxr_build_test(testVdfIterators
    LIBRARIES
        vdf
        tf
        trace
    
    CPPFILES
        testenv/testVdfIterators.cpp
)

pxr_build_test(testVdfConnectorMap
    LIBRARIES
        vdf

    CPPFILES
        testenv/testVdfConnectorMap.cpp
)

pxr_build_test(testVdfMask
    LIBRARIES
        vdf

    CPPFILES
        testenv/testVdfMask.cpp
)

pxr_build_test(testVdfMaskThreading
    LIBRARIES
        vdf
        tf 
        trace 
        work

    CPPFILES
        testenv/testVdfMaskThreading.cpp
)

pxr_build_test(testVdfNetworkThreading
    LIBRARIES
        vdf
        tf 
        trace 
        work

    CPPFILES
        testenv/testVdfNetworkThreading.cpp
)

pxr_build_test(testVdfParallelExecutorDataVector
    LIBRARIES
        vdf 
        work

    CPPFILES
        testenv/testVdfParallelExecutorDataVector.cpp
)

pxr_build_test(testVdfPoolChainIndexer
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfPoolChainIndexer.cpp
)

pxr_build_test(testVdfReadWriteAccessor
    LIBRARIES
        vdf
        tf
        work

    CPPFILES
        testenv/testVdfReadWriteAccessor.cpp
)

pxr_build_test(testVdfReadWriteIterator
    LIBRARIES
        vdf
        tf
    
    CPPFILES
        testenv/testVdfReadWriteIterator.cpp
)

pxr_build_test(testVdfReorderConnections
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfReorderConnections.cpp
)

pxr_build_test(testVdfRequest
    LIBRARIES
        vdf

    CPPFILES
        testenv/testVdfRequest.cpp
)

pxr_build_test(testVdfSparseInputPathFinder
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfSparseInputPathFinder.cpp
)

pxr_build_test(testVdfSpeculation
    LIBRARIES
        vdf
        gf
        tf

    CPPFILES
        testenv/testVdfSpeculation.cpp
)

pxr_build_test(testVdfSubrangeView
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfSubrangeView.cpp
)

pxr_build_test(testVdfTraversal
    LIBRARIES
        vdf
        tf

    CPPFILES
        testenv/testVdfTraversal.cpp
)

pxr_build_test(testVdfVector
    LIBRARIES
        vdf
        tf
        trace 
        vt

    CPPFILES
        testenv/testVdfVector.cpp
)


pxr_register_test(testVdfBasic
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfBasic"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfCountingIterator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfCountingIterator"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfDefaultInitAllocator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfDefaultInitAllocator"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfVector
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfVector"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfGrapher
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfGrapher"
    EXPECTED_RETURN_CODE 0
    POST_COMMAND "${PYTHON_EXECUTABLE} normalizeDotFiles.py test.dot"
    DIFF_COMPARE test.out
    DIFF_COMPARE traverse.out
    DIFF_COMPARE network.dot
    DIFF_COMPARE subset.dot
    DIFF_COMPARE overlapping_subsets.dot
    DIFF_COMPARE displayFull.dot
    DIFF_COMPARE displayNoLabels.dot
    DIFF_COMPARE displaySummary.dot
    TESTENV testVdfGrapher
)

pxr_register_test(testVdfHardToReach
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfHardToReach"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfIndexedWeights
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfIndexedWeights -c"
    STDOUT_REDIRECT test.out
    EXPECTED_RETURN_CODE 0
    DIFF_COMPARE test.out
)

pxr_register_test(testVdfIterators
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfIterators"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfConnectorMap
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfConnectorMap"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfMask
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfMask"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfMaskThreading
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfMaskThreading"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfReorderConnections
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfReorderConnections"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfSpeculation
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfSpeculation"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfTraversal
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfTraversal"
    EXPECTED_RETURN_CODE 0
    STDOUT_REDIRECT testVdfTraversal-Run1-stdout.txt
    DIFF_COMPARE testVdfTraversal-Run1-stdout.txt
    TESTENV testVdfTraversal
)

pxr_register_test(testVdfSparseInputPathFinder
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfSparseInputPathFinder"
    EXPECTED_RETURN_CODE 0
    TESTENV testVdfSparseInputPathFinder
)

pxr_register_test(testVdfEdit
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfEdit"
    EXPECTED_RETURN_CODE 0
    POST_COMMAND
        "${PYTHON_EXECUTABLE} sortDotFiles.py \
        original.dot\
        edit_step_0.dot\
        edit_step_1.dot\
        edit_step_2.dot\
        edit_step_3.dot\
        edit_step_4.dot\
        removed_edge_0.dot\
        removed_edge_1.dot\
        removed_edge_2.dot\
        removed_edge_3.dot\
        removed_edge_4.dot\
        removed_edge_5.dot"
    DIFF_COMPARE original.dot.out
    DIFF_COMPARE edit_step_0.dot.out
    DIFF_COMPARE edit_step_1.dot.out
    DIFF_COMPARE edit_step_2.dot.out
    DIFF_COMPARE edit_step_3.dot.out
    DIFF_COMPARE edit_step_4.dot.out
    DIFF_COMPARE removed_edge_0.dot.out
    DIFF_COMPARE removed_edge_1.dot.out
    DIFF_COMPARE removed_edge_2.dot.out
    DIFF_COMPARE removed_edge_3.dot.out
    DIFF_COMPARE removed_edge_4.dot.out
    DIFF_COMPARE removed_edge_5.dot.out
    TESTENV testVdfEdit
)

pxr_register_test(testVdfExecutionTypeRegistry
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfExecutionTypeRegistry"
    EXPECTED_RETURN_CODE 0
    ENV
      TF_FATAL_VERIFY=0
)

pxr_register_test(testVdfIsolatedSubnetwork
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfIsolatedSubnetwork"
    EXPECTED_RETURN_CODE 0
)

# pxr_register_test(testVdfIndexedWeights_Perf
#     COMMAND testVdfIndexedWeights -p
#     EXPECTED_RETURN_CODE 0
#     POST_COMMAND "perfgen perfstats.raw testVdfIndexedWeights_Perf.yaml"
#     <logmetrics file="testVdfIndexedWeights_Perf.yaml"/>
#     TESTENV testVdfIndexedWeights_Perf
# )

pxr_register_test(testVdfDynamicTopologicalSorter
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfDynamicTopologicalSorter"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfParallelExecutorDataVector
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfParallelExecutorDataVector"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfPoolChainIndexer
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfPoolChainIndexer"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfExecutionStats
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfExecutionStats"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfRequest
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfRequest"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfInputValuesPointer
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfInputValuesPointer"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfIteratorRange
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfIteratorRange"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfReadWriteIterator
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfReadWriteIterator"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfReadWriteAccessor
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfReadWriteAccessor"
    EXPECTED_RETURN_CODE 0
)

pxr_register_test(testVdfSubrangeView
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testVdfSubrangeView"
    EXPECTED_RETURN_CODE 0
)
