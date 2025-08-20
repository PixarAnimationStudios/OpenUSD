# Remove current dir from include path, or else ef/time.h replaces the system
# header <time.h>
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(PXR_PREFIX pxr/exec)
set(PXR_PACKAGE ef)

pxr_library(ef
    LIBRARIES
        vdf
        arch
        tf
        trace
        usd
        work
        TBB::tbb

    PUBLIC_HEADERS
        api.h
        executor.h
        firstValidInputValue.h
        pageCacheBasedExecutor.h
        pageCacheExecutor.h
        pageCacheSubExecutor.h
        subExecutor.h

    PUBLIC_CLASSES
        dependencyCache
        inputValueBlock
        leafNode
        leafNodeCache
        leafNodeIndexer
        loftedOutputSet
        maskedSubExecutor
        outputValueCache
        pageCache
        pageCacheCommitRequest
        pageCacheStorage
        time
        timeInputNode
        timeInterval
        vectorKey

    DOXYGEN_FILES
        README.md
)

pxr_build_test(testEfDependencyCache
    LIBRARIES
        ef
        tf
        work
    
    CPPFILES
        testenv/testEfDependencyCache.cpp
)

pxr_register_test(testEfDependencyCache
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEfDependencyCache"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testEfLeafNodeCache
    LIBRARIES
        ef
        tf
        work
    
    CPPFILES
        testenv/testEfLeafNodeCache.cpp
)

pxr_register_test(testEfLeafNodeCache
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEfLeafNodeCache"
    EXPECTED_RETURN_CODE 0
)

pxr_build_test(testEfTime
    LIBRARIES
        ef
        tf
    
    CPPFILES
        testenv/testEfTime.cpp
)

pxr_register_test(testEfTime
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testEfTime"
    EXPECTED_RETURN_CODE 0
)
