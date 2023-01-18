# It's important to not include the wrong math.h here.
set(CMAKE_INCLUDE_CURRENT_DIR OFF)

set(PXR_PREFIX pxr/base)
set(PXR_PACKAGE arch)

set(PLATFORM_LIBS ${M_LIB})
if(WIN32)
    set(PLATFORM_LIBS Ws2_32.lib Dbghelp.lib ${PLATFORM_LIBS})
endif()

pxr_library(arch
    LIBRARIES
        ${CMAKE_DL_LIBS}
        ${PLATFORM_LIBS}

    PUBLIC_CLASSES
        align
        attributes
        daemon
        debugger
        demangle
        env
        error
        errno
        fileSystem
        function
        hash
        library
        mallocHook
        regex
        stackTrace
        symbols
        systemInfo
        threads
        timing
        virtualMemory
        vsnprintf

    PUBLIC_HEADERS
        api.h
        buildMode.h
        defines.h
        export.h
        functionLite.h
        hints.h
        inttypes.h
        math.h
        pragmas.h

    PRIVATE_HEADERS
        testArchAbi.h
        testArchUtil.h

    CPPFILES
        assumptions.cpp
        initConfig.cpp

    DOXYGEN_FILES
        overview.dox
)

pxr_build_test_shared_lib(testArchAbiPlugin
    CPPFILES
        testenv/testArchAbiPlugin.cpp
)
pxr_build_test(testArchAbi
    LIBRARIES
        arch
        ${CMAKE_DL_LIBS}
    CPPFILES
        testenv/testArchAbi.cpp
)
pxr_build_test(testArchAttributes
    LIBRARIES
        arch
    CPPFILES
        testenv/testAttributes.cpp
    )
pxr_build_test(testArchDemangle
    LIBRARIES
        arch
    CPPFILES
        testenv/testDemangle.cpp
)
pxr_build_test(testArchError
    LIBRARIES
        arch
    CPPFILES
        testenv/testArchUtil.cpp
        testenv/testError.cpp
)
pxr_build_test(testArchErrno
    LIBRARIES
        arch
    CPPFILES
        testenv/testErrno.cpp
)
pxr_build_test(testArchFileSystem
    LIBRARIES
        arch
    CPPFILES
        testenv/testFileSystem.cpp
)
pxr_build_test(testArchFunction
    LIBRARIES
        arch
    CPPFILES
        testenv/testFunction.cpp
)
pxr_build_test(testArchMath
    LIBRARIES
        arch
    CPPFILES
        testenv/testMath.cpp
)
pxr_build_test(testArchPRead
    LIBRARIES
        arch
    CPPFILES
        testenv/testArchPRead.cpp
)
pxr_build_test(testArchStackTrace
    LIBRARIES
        arch
    CPPFILES
        testenv/testArchUtil.cpp
        testenv/testStackTrace.cpp
)
pxr_build_test(testArchSymbols
    LIBRARIES
        arch
    CPPFILES
        testenv/testSymbols.cpp
)
pxr_build_test(testArchSystemInfo
    LIBRARIES
        arch
    CPPFILES
        testenv/testSystemInfo.cpp
)
pxr_build_test(testArchThreads
    LIBRARIES
        arch
    CPPFILES
        testenv/testThreads.cpp
)
pxr_build_test(testArchTiming
    LIBRARIES
        arch
    CPPFILES
        testenv/testTiming.cpp
)
pxr_build_test(testArchVsnprintf
    LIBRARIES
        arch
    CPPFILES
        testenv/testVsnprintf.cpp
)

# On windows, in debug mode, we need to copy testArchStaceTrace.pdb into the
# testing folder, so the test can find it (and print out the right stack),
# even if the build or test folder has been moved (ie, when the baked-in path in
# executable, where it also looks for the pdb, doesn't work)

if (PXR_BUILD_TESTS AND WIN32)
    # This effectively creates a test-env dir (similar to pxr_install_test_dir),
    # and adds a single file, testArchStackTrace.pdb, to it.  The test-env dir
    # ensures that the .pdb is copied into the temporary test dir, which is
    # the PWD when the tests are run.  The PWD (and some subdirs) is the only
    # place that .pdbs will be searched for, other than the baked-in absolute
    # path in the windows binary
    install(
        FILES $<TARGET_PDB_FILE:testArchStackTrace>
        DESTINATION tests/ctest/testArchStackTrace
        CONFIGURATIONS Debug RelWithDebInfo
    )
endif()


pxr_register_test(testArchAbi
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchAbi"
)
pxr_register_test(testArchAttributes
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchAttributes"
)
pxr_register_test(testArchDemangle
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchDemangle"
)
pxr_register_test(testArchError
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchError"
)
pxr_register_test(testArchErrno
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchErrno"
)
pxr_register_test(testArchFileSystem
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchFileSystem"
)
pxr_register_test(testArchFunction
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchFunction"
)
pxr_register_test(testArchStackTrace
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchStackTrace"
)
pxr_register_test(testArchSymbols
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchSymbols"
)
pxr_register_test(testArchSystemInfo
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchSystemInfo"
)
pxr_register_test(testArchTiming
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchTiming"
)
pxr_register_test(testArchThreads
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchThreads"
)
pxr_register_test(testArchVsnprintf
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArchVsnprintf"
)
