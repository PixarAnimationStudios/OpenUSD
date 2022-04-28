set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE ar)

pxr_library(ar
    LIBRARIES
        arch
        tf
        plug
        vt
        ${Boost_PYTHON_LIBRARY}

    INCLUDE_DIRS
        ${Boost_INCLUDE_DIRS}

    PUBLIC_HEADERS
        ar.h
        api.h
        asset.h
        assetInfo.h
        defaultResolver.h
        defaultResolverContext.h
        definePackageResolver.h
        defineResolver.h
        defineResolverContext.h
        filesystemAsset.h
        filesystemWritableAsset.h
        notice.h
        packageResolver.h
        packageUtils.h
        resolvedPath.h
        resolver.h
        resolverContext.h
        resolverContextBinder.h
        resolverScopedCache.h
        resolver.h
        threadLocalScopedCache.h
        timestamp.h
        writableAsset.h

    PYTHON_PUBLIC_HEADERS
        pyResolverContext.h

    PRIVATE_HEADERS
        debugCodes.h
        testenv/TestArURIResolver_plugin.h

    CPPFILES
        assetInfo.cpp
        asset.cpp
        debugCodes.cpp
        defaultResolverContext.cpp
        defaultResolver.cpp
        filesystemAsset.cpp
        filesystemWritableAsset.cpp
        notice.cpp
        packageResolver.cpp
        packageUtils.cpp
        resolvedPath.cpp
        resolverContextBinder.cpp
        resolverContext.cpp
        resolverScopedCache.cpp
        resolver.cpp
        timestamp.cpp
        writableAsset.cpp

    PYTHON_CPPFILES
        moduleDeps.cpp
        pyResolverContext.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapDefaultResolverContext.cpp
        wrapDefaultResolver.cpp
        wrapNotice.cpp
        wrapPackageUtils.cpp
        wrapResolvedPath.cpp
        wrapResolverContextBinder.cpp
        wrapResolverContext.cpp
        wrapResolverScopedCache.cpp
        wrapResolver.cpp
        wrapTimestamp.cpp

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json

    DOXYGEN_FILES
        overview.dox
)

pxr_test_scripts(
    testenv/testArAdvancedAPI.py
    testenv/testArDefaultResolver.py
    testenv/testArPackageUtils.py
    testenv/testArResolvedPath.py
    testenv/testArResolverContext.py
    testenv/testArTimestamp.py
)

pxr_build_test_shared_lib(TestArAdvancedAPI
    CREATE_FRAMEWORK
    INSTALL_PREFIX ArPlugins
    LIBRARIES
        tf
        ar
    CPPFILES
        testenv/TestArAdvancedAPI_plugin.cpp
) 

pxr_build_test_shared_lib(TestArPackageResolver
    CREATE_FRAMEWORK
    INSTALL_PREFIX ArPlugins
    LIBRARIES
        tf
        ar
    CPPFILES
        testenv/TestArPackageResolver_plugin.cpp
) 

pxr_build_test_shared_lib(TestArOptionalImplementation
    CREATE_FRAMEWORK
    INSTALL_PREFIX ArPlugins
    LIBRARIES
        tf
        ar
    CPPFILES
        testenv/TestArOptionalImplementation_plugin.cpp
) 

pxr_test_scripts(
    testenv/testArOptionalImplementation.py
)

pxr_install_test_dir(
    SRC testenv/testArOptionalImplementation
    DEST testArOptionalImplementation_1
)

pxr_install_test_dir(
    SRC testenv/testArOptionalImplementation
    DEST testArOptionalImplementation_2
)

pxr_install_test_dir(
    SRC testenv/testArOptionalImplementation
    DEST testArOptionalImplementation_3
)

pxr_install_test_dir(
    SRC testenv/testArOptionalImplementation
    DEST testArOptionalImplementation_4
)

pxr_install_test_dir(
    SRC testenv/testArOptionalImplementation
    DEST testArOptionalImplementation_5
)

pxr_register_test(testArOptionalImplementation_1
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArOptionalImplementation _TestResolver"
    STDOUT_REDIRECT base.txt
    DIFF_COMPARE base.txt
)

pxr_register_test(testArOptionalImplementation_2
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArOptionalImplementation _TestResolverWithContextMethods"
    STDOUT_REDIRECT resolver_with_context.txt
    DIFF_COMPARE resolver_with_context.txt
)

pxr_register_test(testArOptionalImplementation_3
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArOptionalImplementation _TestDerivedResolverWithContextMethods"
    STDOUT_REDIRECT derived_resolver_with_context.txt
    DIFF_COMPARE derived_resolver_with_context.txt
)

pxr_register_test(testArOptionalImplementation_4
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArOptionalImplementation _TestResolverWithCacheMethods"
    STDOUT_REDIRECT resolver_with_cache.txt
    DIFF_COMPARE resolver_with_cache.txt
)

pxr_register_test(testArOptionalImplementation_5
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArOptionalImplementation _TestDerivedResolverWithCacheMethods"
    STDOUT_REDIRECT derived_resolver_with_cache.txt
    DIFF_COMPARE derived_resolver_with_cache.txt
)

# This test relies on a plugin built specifically for this test.
# The build system currently doesn't support this for static library
# builds, so we only enable this test on shared library builds.
if (BUILD_SHARED_LIBS)
    pxr_build_test_shared_lib(TestArURIResolver
        CREATE_FRAMEWORK
        INSTALL_PREFIX ArPlugins
        LIBRARIES
            tf
            ar
        CPPFILES
            testenv/TestArURIResolver_plugin.cpp
    ) 

    pxr_build_test(testArURIResolver_CPP
        LIBRARIES
            arch
            tf
            ar
        CPPFILES
            testenv/testArURIResolver.cpp
    )

    pxr_register_test(testArURIResolver_CPP
        COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArURIResolver_CPP"
    )
endif()

pxr_build_test(testArDefaultResolver_CPP
    LIBRARIES
        arch
        tf
        ar
    CPPFILES
        testenv/testArDefaultResolver.cpp
) 

pxr_build_test(testArNotice_CPP
    LIBRARIES
        arch
        tf
        ar
    CPPFILES
        testenv/testArNotice.cpp
) 

pxr_register_test(testArNotice_CPP
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArNotice_CPP"
)

pxr_build_test(testArResolverContext_CPP
    LIBRARIES
        arch
        tf
        ar
    CPPFILES
        testenv/testArResolverContext.cpp
) 

pxr_register_test(testArAdvancedAPI
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArAdvancedAPI"
)

pxr_register_test(testArDefaultResolver
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArDefaultResolver"
)

pxr_register_test(testArDefaultResolver_CPP
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArDefaultResolver_CPP"
)

pxr_register_test(testArPackageUtils
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArPackageUtils"
)

pxr_register_test(testArResolvedPath
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArResolvedPath"
)

pxr_register_test(testArTimestamp
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArTimestamp"
)

pxr_register_test(testArResolverContext
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArResolverContext"
)

pxr_register_test(testArResolverContext_CPP
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArResolverContext_CPP"
)
