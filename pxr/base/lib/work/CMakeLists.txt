set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE work)

pxr_shared_library(work
    LIBRARIES
        tf
        tracelite
        ${TBB_tbb_LIBRARY}
        ${TBB_tbbmalloc_LIBRARY}

    INCLUDE_DIRS
        ${TBB_INCLUDE_DIRS}

    PUBLIC_CLASSES
        arenaDispatcher
        detachedTask
        dispatcher
        loops
        singularTask
        threadLimits
        utils

    PUBLIC_HEADERS
        api.h
        
    CPPFILES
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapThreadLimits.cpp

    PYTHON_FILES
        __init__.py
)

if(UNIX)
    pxr_build_test(testWorkDispatcher
        LIBRARIES
            work
        CPPFILES
            testenv/testWorkDispatcher.cpp
    ) 
    pxr_build_test(testWorkLoops
        LIBRARIES
            work
        CPPFILES
            testenv/testWorkLoops.cpp
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
    pxr_register_test(testWorkLoops
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkLoops"
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
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits --rawtbb"
    )
    pxr_register_test(testWorkThreadLimitsRawTBB2
        ENV PXR_WORK_THREAD_LIMIT=2
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimits --rawtbb"
    )
endif()