set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE work)

if (NOT PXR_WORK_IMPL OR PXR_WORK_IMPL STREQUAL "workTBB")
    set(use_tbb TRUE)
else()
    set(use_tbb FALSE)
endif()

if (use_tbb)
    set(work_impl_libraries
        TBB::tbb
    )
    set(work_impl_classes
        workTBB/detachedTask_impl
        workTBB/dispatcher_impl
        workTBB/isolatingDispatcher_impl
        workTBB/taskGraph_impl
        workTBB/threadLimits_impl
    )
    set(work_impl_headers
        workTBB/impl.h
        workTBB/loops_impl.h
        workTBB/reduce_impl.h
        workTBB/sort_impl.h
        workTBB/withScopedParallelism_impl.h
    )   

    set(PXR_WORK_IMPL_HEADER [["pxr/base/work/workTBB/impl.h"]])
else()
    set(work_impl_package "${PXR_WORK_IMPL}")
    set(work_impl_target "${PXR_WORK_IMPL}::${PXR_WORK_IMPL}")
    find_package("${work_impl_package}" CONFIG REQUIRED)

    set(work_impl_libraries "${work_impl_target}")
    set(work_impl_classes "")
    set(work_impl_headers "")

    set(PXR_WORK_IMPL_HEADER "<${PXR_WORK_IMPL}/impl.h>")

    # Set additional cache variables primarily used when generating
    # USD's package config file.
    set(PXR_WORK_IMPL_PACKAGE "${PXR_WORK_IMPL}"
        CACHE INTERNAL
        "Package providing custom work implementation, e.g. 'workExample'")
    set(PXR_WORK_IMPL_CONFIG_DIR_VAR "${PXR_WORK_IMPL}_DIR"
        CACHE INTERNAL
        "Variable for package config file location, e.g. 'workExample_DIR'")
    set(PXR_WORK_IMPL_CONFIG_DIR "${${PXR_WORK_IMPL_CONFIG_DIR_VAR}}"
        CACHE INTERNAL
        "Directory for package config file, e.g. '/path/to/config/dir'")
endif()

configure_file(impl.h.in
    "${PROJECT_BINARY_DIR}/include/pxr/base/work/impl.h"
)
install(FILES
    "${PROJECT_BINARY_DIR}/include/pxr/base/work/impl.h"
    DESTINATION "include/pxr/base/work"
)

pxr_library(work
    LIBRARIES
        tf
        trace
        # libWork still depends on tbb for concurrent containers even
        # when using a custom implementation.
        TBB::tbb
        ${work_impl_target}

    PUBLIC_CLASSES
        dispatcher
        taskGraph
        taskGraph_defaultImpl
        threadLimits
        utils
        ${work_impl_classes}

    PUBLIC_HEADERS
        api.h
        detachedTask.h
        isolatingDispatcher.h
        loops.h
        reduce.h
        singularTask.h
        sort.h
        withScopedParallelism.h
        zeroAllocator.h
        ${work_impl_headers}

    PYMODULE_CPPFILES
        module.cpp
        wrapThreadLimits.cpp

    PYMODULE_FILES
        __init__.py

    DOXYGEN_FILES
        overview.dox
)

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

if (use_tbb)
    pxr_build_test(testWorkThreadLimitsTBB
        LIBRARIES
            work
        CPPFILES
            workTBB/testenv/testWorkThreadLimitsTBB.cpp
    )
    pxr_register_test(testWorkThreadLimitsTBBDefault
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimitsTBB"
    )
    pxr_register_test(testWorkThreadLimitsTBB1
        ENV PXR_WORK_THREAD_LIMIT=1
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimitsTBB"
    )
    pxr_register_test(testWorkThreadLimitsTBB3
        ENV PXR_WORK_THREAD_LIMIT=3
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimitsTBB"
    )
    pxr_register_test(testWorkThreadLimitsRawTBBMax
        RUN_SERIAL
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimitsTBB --rawtbb"
    )
    pxr_register_test(testWorkThreadLimitsRawTBB2
        RUN_SERIAL
        ENV PXR_WORK_THREAD_LIMIT=2
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testWorkThreadLimitsTBB --rawtbb"
    )
endif()
