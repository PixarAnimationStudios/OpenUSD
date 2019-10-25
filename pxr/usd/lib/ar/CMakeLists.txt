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

    PYTHON_PUBLIC_HEADERS
        pyResolverContext.h

    PRIVATE_HEADERS
        debugCodes.h

    CPPFILES
        asset.cpp
        assetInfo.cpp
        filesystemAsset.cpp
        packageResolver.cpp
        packageUtils.cpp
        resolver.cpp
        resolverContext.cpp
        resolverContextBinder.cpp
        resolverScopedCache.cpp
        debugCodes.cpp
        defaultResolver.cpp
        defaultResolverContext.cpp

    PYTHON_CPPFILES
        pyResolverContext.cpp
        moduleDeps.cpp

    PYMODULE_CPPFILES
        module.cpp
        wrapDefaultResolver.cpp
        wrapDefaultResolverContext.cpp
        wrapPackageUtils.cpp
        wrapResolver.cpp
        wrapResolverContext.cpp
        wrapResolverContextBinder.cpp
        wrapResolverScopedCache.cpp

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
