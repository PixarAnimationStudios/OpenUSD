set(PXR_PREFIX pxr/usd)
set(PXR_PACKAGE ar)

if(PXR_USE_AR_2)
    set(AR_VERSION 2)
else()
    set(AR_VERSION 1)
endif()

configure_file(
    ar.h.in 
    ${CMAKE_BINARY_DIR}/include/pxr/usd/ar/ar.h)
install(
    FILES ${CMAKE_BINARY_DIR}/include/pxr/usd/ar/ar.h
    DESTINATION include/pxr/usd/ar
)

set(versionedHeaders "")
set(versionedCPPFiles "")
set(versionedPrivateHeaders "")
set(versionedPythonHeaders "")
set(versionedPythonCPPFiles "")
set(versionedPymoduleCPPFiles "")

if (AR_VERSION EQUAL 1)
    list(APPEND versionedHeaders 
         defaultResolver_v1.h
         resolver_v1.h
         resolverContext_v1.h
    )
    list(APPEND versionedCPPFiles
         defaultResolver_v1.cpp
         resolver_v1.cpp
         resolverContext_v1.cpp
    )
    list(APPEND versionedPythonHeaders
         pyResolverContext_v1.h
    )
    list(APPEND versionedPythonCPPFiles
         pyResolverContext_v1.cpp
    )
    list(APPEND versionedPymoduleCPPFiles
         wrapDefaultResolver_v1.cpp
         wrapResolver_v1.cpp
         wrapResolverContext_v1.cpp
    )
elseif (AR_VERSION EQUAL 2)
    list(APPEND versionedHeaders 
         defaultResolver_v2.h
         filesystemWritableAsset.h
         resolver_v2.h
         resolverContext_v2.h
         writableAsset.h
    )
    list(APPEND versionedCPPFiles
         defaultResolver_v2.cpp
         filesystemWritableAsset.cpp
         resolver_v2.cpp
         resolverContext_v2.cpp
         writableAsset.cpp
    )
    list(APPEND versionedPrivateHeaders
         testenv/TestArURIResolver_plugin.h
    )
    list(APPEND versionedPythonHeaders
         pyResolverContext_v2.h
    )
    list(APPEND versionedPythonCPPFiles
         pyResolverContext_v2.cpp
    )
    list(APPEND versionedPymoduleCPPFiles
         wrapDefaultResolver_v2.cpp
         wrapResolver_v2.cpp
         wrapResolverContext_v2.cpp
    )
endif()

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
        api.h
        asset.h
        assetInfo.h
        defaultResolver.h
        defaultResolverContext.h
        filesystemAsset.h
        definePackageResolver.h
        defineResolver.h
        defineResolverContext.h
        packageResolver.h
        packageUtils.h
        resolvedPath.h
        resolver.h
        resolverContext.h
        resolverContextBinder.h
        resolverScopedCache.h
        threadLocalScopedCache.h
        ${versionedHeaders}

    PYTHON_PUBLIC_HEADERS
        pyResolverContext.h
        ${versionedPythonHeaders}

    PRIVATE_HEADERS
        debugCodes.h
        ${versionedPrivateHeaders}

    CPPFILES
        asset.cpp
        assetInfo.cpp
        filesystemAsset.cpp
        packageResolver.cpp
        packageUtils.cpp
        resolvedPath.cpp
        resolverContextBinder.cpp
        resolverScopedCache.cpp
        debugCodes.cpp
        defaultResolverContext.cpp
        ${versionedCPPFiles}

    PYTHON_CPPFILES
        moduleDeps.cpp
        ${versionedPythonCPPFiles}

    PYMODULE_CPPFILES
        module.cpp
        wrapDefaultResolverContext.cpp
        wrapPackageUtils.cpp
        wrapResolvedPath.cpp
        wrapResolverContextBinder.cpp
        wrapResolverScopedCache.cpp
        ${versionedPymoduleCPPFiles}

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

if (AR_VERSION EQUAL 2)
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

pxr_register_test(testArResolverContext
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArResolverContext"
)

pxr_register_test(testArResolverContext_CPP
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArResolverContext_CPP"
)
