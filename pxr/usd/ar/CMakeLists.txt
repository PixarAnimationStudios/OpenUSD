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
set(versionedPythonHeaders "")
set(versionedPythonCPPFiles "")
set(versionedPymoduleCPPFiles "")

if (AR_VERSION EQUAL 1)
    list(APPEND versionedHeaders 
         resolver_v1.h
         resolverContext_v1.h
    )
    list(APPEND versionedCPPFiles
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
         wrapResolver_v1.cpp
         wrapResolverContext_v1.cpp
    )
elseif (AR_VERSION EQUAL 2)
    list(APPEND versionedHeaders 
         resolver_v2.h
         resolverContext_v2.h
    )
    list(APPEND versionedCPPFiles
         resolver_v2.cpp
         resolverContext_v2.cpp
    )
    list(APPEND versionedPythonHeaders
         pyResolverContext_v2.h
    )
    list(APPEND versionedPythonCPPFiles
         pyResolverContext_v2.cpp
    )
    list(APPEND versionedPymoduleCPPFiles
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

    CPPFILES
        asset.cpp
        assetInfo.cpp
        filesystemAsset.cpp
        packageResolver.cpp
        packageUtils.cpp
        resolverContextBinder.cpp
        resolverScopedCache.cpp
        debugCodes.cpp
        defaultResolver.cpp
        defaultResolverContext.cpp
        ${versionedCPPFiles}

    PYTHON_CPPFILES
        moduleDeps.cpp
        ${versionedPythonCPPFiles}

    PYMODULE_CPPFILES
        module.cpp
        wrapDefaultResolver.cpp
        wrapDefaultResolverContext.cpp
        wrapPackageUtils.cpp
        wrapResolverContextBinder.cpp
        wrapResolverScopedCache.cpp
        ${versionedPymoduleCPPFiles}

    PYMODULE_FILES
        __init__.py

    RESOURCE_FILES
        plugInfo.json
)

pxr_test_scripts(
    testenv/testArAdvancedAPI.py
    testenv/testArDefaultResolver.py
    testenv/testArPackageUtils.py
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

pxr_build_test(testArDefaultResolver_CPP
    LIBRARIES
        arch
        tf
        ar
    CPPFILES
        testenv/testArDefaultResolver.cpp
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
