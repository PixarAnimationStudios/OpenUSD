set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE work)

pxr_library(work
    LIBRARIES
        tf
        trace
        ${TBB_tbb_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        arenaDispatcher
        detachedTask
        dispatcher
        filter
        loops
        reduce
        singularTask
        sort
        threadLimits
        utils

    PUBLIC_HEADERS
        api.h
        
    PYTHON_CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapThreadLimits.cpp

    PYMODULE_FILES
        __init__.py
)

pxr_build_test(testWorkDispatcher
    LIBRARIES
        work
    CPPFILES
        testenv/testWorkDispatcher.cpp
) 
pxr_build_test(testWorkFilter
    LIBRARIES
        work
    CPPFILES
        testenv/testWorkFilter.cpp
)
pxr_build_test(testWorkLoops
    LIBRARIES
        work
    CPPFILES
        testenv/testWorkLoops.cpp
)
pxr_build_test(testWorkReduce
    LIBRARIES
        work
    CPPFILES
        testenv/testWorkReduce.cpp
) 
pxr_build_test(testWorkSort
    LIBRARIES
        work
    CPPFILES
        testenv/testWorkSort.cpp
)
pxr_build_test(testWorkThreadLimits
    LIBRARIES
        work
    CPPFILES
        testenv/testWorkThreadLimits.cpp
) 
pxr_register_test(testWorkDispatcher
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkDispatcher"
)
pxr_register_test(testWorkFilter
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkFilter"
)
pxr_register_test(testWorkLoops
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkLoops"
)
pxr_register_test(testWorkReduce
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkReduce"
)
pxr_register_test(testWorkSort
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkSort"
)
pxr_register_test(testWorkThreadLimitsDefault
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits"
)
pxr_register_test(testWorkThreadLimits1
    ENV PXR_WORK_THREAD_LIMIT=1
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits"
)
pxr_register_test(testWorkThreadLimits3
    ENV PXR_WORK_THREAD_LIMIT=3
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits"
)
pxr_register_test(testWorkThreadLimitsRawTBBMax
    RUN_SERIAL
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits --rawtbb"
)
pxr_register_test(testWorkThreadLimitsRawTBB2
    RUN_SERIAL
    ENV PXR_WORK_THREAD_LIMIT=2
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits --rawtbb"
)
