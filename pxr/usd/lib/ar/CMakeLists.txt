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
        assetInfo.h
        defaultResolver.h
        defaultResolverContext.h
        resolver.h
        resolverContext.h
        resolverContextBinder.h
        resolverScopedCache.h
        defineResolver.h
        defineResolverContext.h

    PYTHON_PUBLIC_HEADERS
        pyResolverContext.h

    PRIVATE_HEADERS
        debugCodes.h

    CPPFILES
        assetInfo.cpp
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
    testenv/testArDefaultResolver.py
)

pxr_register_test(testArDefaultResolver
    PYTHON
    COMMAND "${CMAKE_INSTALL_PREFIX}/tests/testArDefaultResolver"
)
